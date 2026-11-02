#include "SolidWorldChart.h"
#include "SolidBridges.h"
#include "SolidRoads.h"
#include "SolidBiome.h"
#include "SolidCore1.h"
#include "SolidLair.h"
#include "Events/SolidEncounters.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "Game/SolidGameMode.h"

const TCHAR* USolidWorldChart::ShippedPath()
{
	return TEXT("/Game/World/WorldChart");
}

namespace
{
	/**
	 * The values the place tool selected. Position is not one of them.
	 * A town matches its name. A stronghold and a site match their size.
	 * A lair mouth matches its lair and its end. A note matches its text.
	 * An encounter matches its name. A bridge matches its name.
	 */
	bool SamePlacedMark(const FSolidWorldChartMark& Existing, const FSolidWorldChartMark& Incoming)
	{
		if (Existing.Kind != Incoming.Kind)
		{
			return false;
		}
		switch (Incoming.Kind)
		{
		case ESolidWorldChartMarkKind::Town:
			return Existing.Name.Equals(Incoming.Name, ESearchCase::IgnoreCase);
		case ESolidWorldChartMarkKind::Stronghold:
		case ESolidWorldChartMarkKind::StrongholdSite:
			return Existing.StrongholdSize == Incoming.StrongholdSize;
		case ESolidWorldChartMarkKind::LairEntrance:
		case ESolidWorldChartMarkKind::LairExit:
			if (!Existing.LairId.IsEmpty() && !Incoming.LairId.IsEmpty())
			{
				return Existing.LairId.Equals(Incoming.LairId, ESearchCase::IgnoreCase);
			}
			return Existing.Name.Equals(Incoming.Name, ESearchCase::IgnoreCase);
		case ESolidWorldChartMarkKind::Custom:
			return Existing.Name.Equals(Incoming.Name, ESearchCase::CaseSensitive);
		case ESolidWorldChartMarkKind::Encounter:
		case ESolidWorldChartMarkKind::Bridge:
			return Existing.Name.Equals(Incoming.Name, ESearchCase::IgnoreCase);
		default:
			return false;
		}
	}
}

void USolidWorldChart::PushUndo()
{
	FSnapshot Snapshot;
	Snapshot.Points = Points;
	Snapshot.Marks = Marks;
	UndoStack.Add(MoveTemp(Snapshot));
}

bool USolidWorldChart::PlaceOn(const FIntPoint TerrainPoint, const FSolidWorldChartMark& Mark, FString& OutError)
{
	OutError.Reset();
	if (TerrainPoint.X < 0 || TerrainPoint.Y < 0
		|| TerrainPoint.X >= PointAxis || TerrainPoint.Y >= PointAxis)
	{
		OutError = TEXT("That point is off the map.");
		return false;
	}
	if (!EnsurePoints())
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	FSolidWorldChartMark Stored = Mark;
	Stored.TerrainPoint = TerrainPoint;
	for (int32 Index = Marks.Num() - 1; Index >= 0; --Index)
	{
		const bool bHere = Marks[Index].TerrainPoint == Stored.TerrainPoint;
		if (!bHere && !SamePlacedMark(Marks[Index], Stored))
		{
			continue;
		}
		Marks.RemoveAt(Index);
	}
	Marks.Add(Stored);
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::PlaceTownAt(const FIntPoint TerrainPoint, const FString& Name, FString& OutError)
{
	const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Name.TrimStartAndEnd());
	if (!Town)
	{
		OutError = FString::Printf(TEXT("%s is not a town."), *Name.TrimStartAndEnd());
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::Town;
	Mark.Name = Town->Name;
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceStrongholdAt(
	const FIntPoint TerrainPoint,
	const ESolidStrongholdSize Size,
	const ESolidStrongholdType Type,
	FString& OutError)
{
	bool bSize = false;
	for (const ESolidStrongholdSize Known : SolidStronghold::Sizes)
	{
		bSize |= Known == Size;
	}
	bool bType = false;
	for (const ESolidStrongholdType Known : SolidStronghold::Types)
	{
		bType |= Known == Type;
	}
	if (!bSize || !bType)
	{
		OutError = TEXT("That size and type is not a stronghold the game has.");
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::Stronghold;
	Mark.StrongholdSize = Size;
	Mark.StrongholdType = Type;
	Mark.Name = SolidStronghold::Label(Size, Type);
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceSiteAt(const FIntPoint TerrainPoint, const ESolidStrongholdSize Size, FString& OutError)
{
	bool bKnownSize = false;
	for (const ESolidStrongholdSize Known : SolidStronghold::Sizes)
	{
		bKnownSize |= Known == Size;
	}
	if (!bKnownSize)
	{
		OutError = TEXT("That size is not a stronghold size.");
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::StrongholdSite;
	Mark.StrongholdSize = Size;
	Mark.Name = SolidStronghold::SizeName(Size);
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceLairMouthAt(
	const FIntPoint TerrainPoint,
	const FString& LairName,
	const bool bEntrance,
	FString& OutError)
{
	const SolidLair::FDefinition* Lair = SolidLair::FindByName(LairName.TrimStartAndEnd());
	if (!Lair)
	{
		OutError = FString::Printf(TEXT("%s is not a lair."), *LairName.TrimStartAndEnd());
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = bEntrance ? ESolidWorldChartMarkKind::LairEntrance : ESolidWorldChartMarkKind::LairExit;
	Mark.Name = Lair->Name;
	Mark.LairId = Lair->Id;
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceCustomAt(const FIntPoint TerrainPoint, const FString& Note, FString& OutError)
{
	FString Clean = Note;
	Clean.ReplaceInline(TEXT("\r"), TEXT(" "));
	Clean.ReplaceInline(TEXT("\n"), TEXT(" "));
	Clean.TrimStartAndEndInline();
	while (Clean.Contains(TEXT("  ")))
	{
		Clean.ReplaceInline(TEXT("  "), TEXT(" "));
	}
	if (Clean.IsEmpty())
	{
		OutError = TEXT("A note needs some text.");
		return false;
	}
	constexpr int32 MaxNote = 48;
	if (Clean.Len() > MaxNote)
	{
		OutError = TEXT("A note is at most 48 characters.");
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::Custom;
	Mark.Name = Clean;
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceEncounterAt(const FIntPoint TerrainPoint, const FString& Name, FString& OutError)
{
	const SolidEncounter::FStaticEncounter* Found = SolidEncounter::FindByName(Name);
	if (!Found || !Found->Name)
	{
		OutError = FString::Printf(TEXT("%s is not an encounter."), *Name.TrimStartAndEnd());
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::Encounter;
	Mark.Name = Found->Name;
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::PlaceBridgeAt(const FIntPoint TerrainPoint, const FString& Name, FString& OutError)
{
	const FString Clean = Name.TrimStartAndEnd();
	const SolidBridge::FBridgeMarker* Found = nullptr;
	for (const SolidBridge::FBridgeMarker& Bridge : SolidBridge::Bridges)
	{
		if (Clean.Equals(Bridge.Name, ESearchCase::IgnoreCase))
		{
			Found = &Bridge;
			break;
		}
	}
	if (!Found || !Found->Name)
	{
		OutError = FString::Printf(TEXT("%s is not a bridge."), *Clean);
		return false;
	}

	FSolidWorldChartMark Mark;
	Mark.Kind = ESolidWorldChartMarkKind::Bridge;
	Mark.Name = Found->Name;
	return PlaceOn(TerrainPoint, Mark, OutError);
}

bool USolidWorldChart::DeleteMarkAt(const FIntPoint TerrainPoint, FString& OutError)
{
	OutError.Reset();
	int32 Existing = INDEX_NONE;
	for (int32 Index = 0; Index < Marks.Num(); ++Index)
	{
		if (Marks[Index].TerrainPoint == TerrainPoint)
		{
			Existing = Index;
			break;
		}
	}
	if (Existing == INDEX_NONE)
	{
		OutError = TEXT("No mark on this point.");
		return false;
	}
	PushUndo();
	Marks.RemoveAt(Existing);
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return false;
	}
	const FSnapshot Snapshot = UndoStack.Pop();
	Points = Snapshot.Points;
	Marks = Snapshot.Marks;
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::EnsureQuery() const
{
	if (bIsLoaded)
	{
		return true;
	}
	if (Points.Num() != PointAxis * PointAxis)
	{
		return false;
	}
	return BuildQuery();
}

void USolidWorldChart::InvalidateQuery()
{
	bIsLoaded = false;
}

FLinearColor USolidWorldChart::DefaultColorForBiome(const ESolidBiome Biome)
{
	return SolidBiome::Of(Biome).Color();
}

bool USolidWorldChart::LoadDefault(const bool bForceReload)
{
	if (bIsLoaded && !bForceReload)
	{
		return true;
	}

	const USolidWorldChart* Chart = GetDefault<ASolidGameMode>()->WorldChart;
	if (!Chart)
	{
		Chart = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	}
	if (!Chart || !LoadFromChart(Chart))
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: failed to load /Game/World/WorldChart."));
		return false;
	}
	return true;
}

void USolidWorldChart::ResetLoad() const
{
	RoutedRoads.Reset();
	PointTowns.Reset();
	bIsLoaded = false;
}

bool USolidWorldChart::LoadFromChart(const USolidWorldChart* Chart)
{
	if (!Chart)
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: there is no chart to load."));
		return false;
	}
	if (Chart != this)
	{
		Marks = Chart->Marks;
		const int32 PointCount = PointAxis * PointAxis;
		Points = Chart->Points.Num() == PointCount ? Chart->Points : TArray<ESolidBiome>();
		UndoStack.Reset();
	}
	return BuildQuery();
}

bool USolidWorldChart::BuildQuery() const
{
	if (Points.Num() != PointAxis * PointAxis)
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: the chart does not cover the %d point grid."), PointAxis);
		return false;
	}

	ResetLoad();
	bIsLoaded = true;

	AssignTownPoints();

	BuildListedRoads();

	bIsLoaded = true;
	UE_LOG(LogSolid, Warning,
		TEXT("WorldChart loaded: %d point grid, roads=%d"),
		PointAxis, RoutedRoads.Num());
	return true;
}

int32 USolidWorldChart::CollectEncounterMarks(TArray<FSolidWorldChartMark>& OutMarks) const
{
	OutMarks.Reset();
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind == ESolidWorldChartMarkKind::Encounter)
		{
			OutMarks.Add(Mark);
		}
	}
	return OutMarks.Num();
}

bool USolidWorldChart::StrongholdAtTerrain(
	const int32 TerrainX,
	const int32 TerrainY,
	ESolidStrongholdType& OutType,
	ESolidStrongholdSize& OutSize) const
{
	if (TerrainX < 0 || TerrainY < 0 || TerrainX >= PointAxis || TerrainY >= PointAxis)
	{
		return false;
	}
	const FIntPoint TerrainPoint(TerrainX, TerrainY);
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.TerrainPoint != TerrainPoint)
		{
			continue;
		}
		if (Mark.Kind != ESolidWorldChartMarkKind::Stronghold)
		{
			return false;
		}
		OutType = Mark.StrongholdType;
		OutSize = Mark.StrongholdSize;
		SolidStronghold::FromToken(Mark.Name, OutType, OutSize);
		return true;
	}
	return false;
}

namespace
{
	constexpr int32 PointCount = USolidWorldChart::PointAxis * USolidWorldChart::PointAxis;

	int32 PointSlot(const int32 TerrainX, const int32 TerrainY)
	{
		return TerrainY * USolidWorldChart::PointAxis + TerrainX;
	}

	void PointAtFraction(const float EastFromWest, const float NorthFromSouth, int32& OutTerrainX, int32& OutTerrainY)
	{
		const float Span = static_cast<float>(USolidWorldChart::PointAxis - 1);
		OutTerrainX = FMath::Clamp(FMath::RoundToInt(EastFromWest * Span), 0, USolidWorldChart::PointAxis - 1);
		OutTerrainY = FMath::Clamp(FMath::RoundToInt(NorthFromSouth * Span), 0, USolidWorldChart::PointAxis - 1);
	}

	void PointAtWorld(
		const float WorldX,
		const float WorldY,
		const FVector2D& Min,
		const FVector2D& Max,
		int32& OutTerrainX,
		int32& OutTerrainY)
	{
		const float ExtentX = FMath::Max(Max.X - Min.X, 1.f);
		const float ExtentY = FMath::Max(Max.Y - Min.Y, 1.f);
		PointAtFraction((WorldX - Min.X) / ExtentX, (WorldY - Min.Y) / ExtentY, OutTerrainX, OutTerrainY);
	}

	FVector2D WorldAtPoint(const int32 TerrainX, const int32 TerrainY, const FVector2D& Min, const FVector2D& Max)
	{
		const float Span = static_cast<float>(USolidWorldChart::PointAxis - 1);
		return FVector2D(
			Min.X + static_cast<float>(TerrainX) * (Max.X - Min.X) / Span,
			Min.Y + static_cast<float>(TerrainY) * (Max.Y - Min.Y) / Span);
	}

	bool OnPoint(const FIntPoint& TerrainPoint)
	{
		return TerrainPoint.X >= 0 && TerrainPoint.Y >= 0
			&& TerrainPoint.X < USolidWorldChart::PointAxis
			&& TerrainPoint.Y < USolidWorldChart::PointAxis;
	}

	void AppendPoint(TArray<FIntPoint>& Points, TSet<int32>& Seen, const int32 TerrainX, const int32 TerrainY)
	{
		if (TerrainX < 0 || TerrainY < 0 || TerrainX >= USolidWorldChart::PointAxis || TerrainY >= USolidWorldChart::PointAxis)
		{
			return;
		}
		const int32 Slot = PointSlot(TerrainX, TerrainY);
		if (Seen.Contains(Slot))
		{
			return;
		}
		Seen.Add(Slot);
		Points.Add(FIntPoint(TerrainX, TerrainY));
	}

	void AppendDisc(TArray<FIntPoint>& Points, TSet<int32>& Seen, const int32 TerrainX, const int32 TerrainY, const int32 Radius)
	{
		const int32 Reach = FMath::Clamp(Radius, 0, USolidWorldChart::PointAxis);
		const int32 ReachSq = Reach * Reach;
		for (int32 DY = -Reach; DY <= Reach; ++DY)
		{
			for (int32 DX = -Reach; DX <= Reach; ++DX)
			{
				if (DX * DX + DY * DY <= ReachSq)
				{
					AppendPoint(Points, Seen, TerrainX + DX, TerrainY + DY);
				}
			}
		}
	}

	/** Straight run of brush stamps from one cursor point to the next. */
	void AppendRun(TArray<FIntPoint>& Points, TSet<int32>& Seen, const FIntPoint& From, const FIntPoint& To, const int32 Radius)
	{
		int32 TerrainX = From.X;
		int32 TerrainY = From.Y;
		const int32 DX = FMath::Abs(To.X - TerrainX);
		const int32 DY = FMath::Abs(To.Y - TerrainY);
		const int32 StepX = TerrainX < To.X ? 1 : -1;
		const int32 StepY = TerrainY < To.Y ? 1 : -1;
		int32 Error = DX - DY;
		while (true)
		{
			AppendDisc(Points, Seen, TerrainX, TerrainY, Radius);
			if (TerrainX == To.X && TerrainY == To.Y)
			{
				break;
			}
			const int32 Twice = Error * 2;
			if (Twice > -DY)
			{
				Error -= DY;
				TerrainX += StepX;
			}
			if (Twice < DX)
			{
				Error += DX;
				TerrainY += StepY;
			}
		}
	}
}

void USolidWorldChart::AssignTownPoints() const
{
	PointTowns.Reset();
	if (Points.Num() != PointCount)
	{
		return;
	}

	PointTowns.Init(INDEX_NONE, PointCount);
	TArray<int32> Queue;
	auto Claim = [this, &Queue](const int32 LocationId, const int32 TerrainX, const int32 TerrainY)
	{
		if (LocationId == INDEX_NONE
			|| TerrainX < 0 || TerrainY < 0
			|| TerrainX >= PointAxis || TerrainY >= PointAxis)
		{
			return;
		}
		const int32 Start = PointSlot(TerrainX, TerrainY);
		if (Points[Start] != ESolidBiome::Town || PointTowns[Start] != INDEX_NONE)
		{
			return;
		}

		PointTowns[Start] = LocationId;
		Queue.Reset();
		Queue.Add(Start);
		for (int32 Index = 0; Index < Queue.Num(); ++Index)
		{
			const int32 Slot = Queue[Index];
			const int32 OriginX = Slot % PointAxis;
			const int32 OriginY = Slot / PointAxis;
			const FIntPoint Steps[] = {
				FIntPoint(1, 0),
				FIntPoint(-1, 0),
				FIntPoint(0, 1),
				FIntPoint(0, -1),
			};
			for (const FIntPoint& Step : Steps)
			{
				const int32 NextTerrainX = OriginX + Step.X;
				const int32 NextTerrainY = OriginY + Step.Y;
				if (NextTerrainX < 0 || NextTerrainY < 0 || NextTerrainX >= PointAxis || NextTerrainY >= PointAxis)
				{
					continue;
				}
				const int32 Next = PointSlot(NextTerrainX, NextTerrainY);
				if (Points[Next] != ESolidBiome::Town || PointTowns[Next] != INDEX_NONE)
				{
					continue;
				}
				PointTowns[Next] = LocationId;
				Queue.Add(Next);
			}
		}
	};

	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::Town)
		{
			continue;
		}
		const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
		if (!Town)
		{
			continue;
		}
		Claim(Town->Id, Mark.TerrainPoint.X, Mark.TerrainPoint.Y);
	}
}

bool USolidWorldChart::EnsurePoints() const
{
	return Points.Num() == PointCount;
}

bool USolidWorldChart::TerrainPointAt(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FIntPoint& OutTerrainPoint) const
{
	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, TerrainX, TerrainY);
	OutTerrainPoint = FIntPoint(TerrainX, TerrainY);
	return true;
}

bool USolidWorldChart::TerrainPointToWorld(
	const FIntPoint TerrainPoint,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (!OnPoint(TerrainPoint))
	{
		return false;
	}
	OutWorldXY = WorldAtPoint(TerrainPoint.X, TerrainPoint.Y, WorldMinXY, WorldMaxXY);
	return true;
}

ESolidBiome USolidWorldChart::GetPointBiome(const FIntPoint TerrainPoint) const
{
	if (!EnsurePoints() || !OnPoint(TerrainPoint))
	{
		return ESolidBiome::Grassland;
	}
	return Points[PointSlot(TerrainPoint.X, TerrainPoint.Y)];
}

void USolidWorldChart::BrushPoints(const TArray<FIntPoint>& Centres, const int32 Radius, TArray<FIntPoint>& OutPoints)
{
	OutPoints.Reset();
	if (Centres.Num() == 0)
	{
		return;
	}

	TSet<int32> Seen;
	FIntPoint Previous = Centres[0];
	for (const FIntPoint& TerrainPoint : Centres)
	{
		if (!OnPoint(Previous) || !OnPoint(TerrainPoint))
		{
			Previous = TerrainPoint;
			continue;
		}
		AppendRun(OutPoints, Seen, Previous, TerrainPoint, Radius);
		Previous = TerrainPoint;
	}
}

void USolidWorldChart::PointRectangle(const FIntPoint From, const FIntPoint To, TArray<FIntPoint>& OutPoints)
{
	OutPoints.Reset();
	const int32 TerrainX0 = FMath::Min(From.X, To.X);
	const int32 TerrainX1 = FMath::Max(From.X, To.X);
	const int32 TerrainY0 = FMath::Min(From.Y, To.Y);
	const int32 TerrainY1 = FMath::Max(From.Y, To.Y);
	if (TerrainX0 < 0 || TerrainY0 < 0 || TerrainX1 >= PointAxis || TerrainY1 >= PointAxis)
	{
		return;
	}

	OutPoints.Reserve((TerrainX1 - TerrainX0 + 1) * (TerrainY1 - TerrainY0 + 1));
	for (int32 TerrainY = TerrainY0; TerrainY <= TerrainY1; ++TerrainY)
	{
		for (int32 TerrainX = TerrainX0; TerrainX <= TerrainX1; ++TerrainX)
		{
			OutPoints.Add(FIntPoint(TerrainX, TerrainY));
		}
	}
}

bool USolidWorldChart::PaintPoints(const TArray<FIntPoint>& Covered, const ESolidBiome Biome, FString& OutError)
{
	OutError.Reset();
	if (!SolidBiome::Of(Biome).Paintable())
	{
		OutError = TEXT("Swamp and road are not paint biomes.");
		return false;
	}
	if (Covered.Num() == 0)
	{
		OutError = TEXT("Paint needs a point.");
		return false;
	}
	int32 HighestSlot = 0;
	for (const FIntPoint& TerrainPoint : Covered)
	{
		if (!OnPoint(TerrainPoint))
		{
			OutError = TEXT("That point is off the map.");
			return false;
		}
		HighestSlot = FMath::Max(HighestSlot, PointSlot(TerrainPoint.X, TerrainPoint.Y));
	}
	if (Points.Num() <= HighestSlot && !EnsurePoints())
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}
	if (Points.Num() <= HighestSlot)
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	for (const FIntPoint& TerrainPoint : Covered)
	{
		Points[PointSlot(TerrainPoint.X, TerrainPoint.Y)] = Biome;
	}
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::PaintBrush(
	const TArray<FIntPoint>& Centres,
	const int32 Radius,
	const ESolidBiome Biome,
	FString& OutError)
{
	OutError.Reset();
	if (Centres.Num() == 0)
	{
		OutError = TEXT("Paint needs a point.");
		return false;
	}
	for (const FIntPoint& TerrainPoint : Centres)
	{
		if (!OnPoint(TerrainPoint))
		{
			OutError = TEXT("That point is off the map.");
			return false;
		}
	}

	TArray<FIntPoint> Covered;
	BrushPoints(Centres, Radius, Covered);
	return PaintPoints(Covered, Biome, OutError);
}

bool USolidWorldChart::SetPointRectangle(const FIntPoint From, const FIntPoint To, const ESolidBiome Biome)
{
	if (Points.Num() != PointCount || !OnPoint(From) || !OnPoint(To))
	{
		return false;
	}

	const int32 TerrainX0 = FMath::Min(From.X, To.X);
	const int32 TerrainX1 = FMath::Max(From.X, To.X);
	const int32 TerrainY0 = FMath::Min(From.Y, To.Y);
	const int32 TerrainY1 = FMath::Max(From.Y, To.Y);
	for (int32 TerrainY = TerrainY0; TerrainY <= TerrainY1; ++TerrainY)
	{
		for (int32 TerrainX = TerrainX0; TerrainX <= TerrainX1; ++TerrainX)
		{
			Points[PointSlot(TerrainX, TerrainY)] = Biome;
		}
	}
	return true;
}

void USolidWorldChart::Init(const ESolidBiome Biome)
{
	Points.Init(Biome, PointAxis * PointAxis);
	InvalidateQuery();
}

bool USolidWorldChart::PaintPointRectangle(
	const FIntPoint From,
	const FIntPoint To,
	const ESolidBiome Biome,
	FString& OutError)
{
	if (!OnPoint(From) || !OnPoint(To))
	{
		OutError = TEXT("That point is off the map.");
		return false;
	}

	TArray<FIntPoint> Covered;
	PointRectangle(From, To, Covered);
	return PaintPoints(Covered, Biome, OutError);
}

ESolidBiome USolidWorldChart::SampleBiome(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!EnsurePoints())
	{
		return ESolidBiome::Grassland;
	}

	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, TerrainX, TerrainY);
	const ESolidBiome Ground = Points[PointSlot(TerrainX, TerrainY)];

	// The road shares the grassland shelf, so this is a texture change there.
	// A stronghold keeps the shelf it names. Mountain and sea keep theirs because
	// those shelves sit at a different height. The channel itself stays water.
	bool bNamedShelf = false;
	ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
	if (StrongholdAtTerrain(TerrainX, TerrainY, HoldType, HoldSize))
	{
		ESolidBiome Named = ESolidBiome::Grassland;
		bNamedShelf = SolidStronghold::BiomeFor(FSolidStronghold(HoldType, HoldSize), Named);
	}
	if (!bNamedShelf
		&& Ground != ESolidBiome::River
		&& IsOnRoad(WorldX, WorldY, WorldMinXY, WorldMaxXY)
		&& SolidBiome::Of(Ground).HeightOffsetCm()
			== SolidBiome::Of(ESolidBiome::Road).HeightOffsetCm())
	{
		return ESolidBiome::Road;
	}
	return Ground;
}

int32 USolidWorldChart::SampleLocationId(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!EnsureQuery() || PointTowns.Num() != PointCount || Points.Num() != PointCount)
	{
		return INDEX_NONE;
	}

	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, TerrainX, TerrainY);
	const int32 Slot = PointSlot(TerrainX, TerrainY);
	if (Points[Slot] != ESolidBiome::Town)
	{
		return INDEX_NONE;
	}
	return PointTowns[Slot];
}

FLinearColor USolidWorldChart::GetBiomeColor(const ESolidBiome Biome) const
{
	return DefaultColorForBiome(Biome);
}

bool USolidWorldChart::FindTownMarker(const int32 TownId, FSolidWorldChartMark& OutMark) const
{
	const SolidTowns::FTownInfo* Town = SolidTowns::Find(TownId);
	if (!Town)
	{
		return false;
	}
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::Town)
		{
			continue;
		}
		if (!Mark.Name.Equals(Town->Name, ESearchCase::IgnoreCase))
		{
			continue;
		}
		OutMark = Mark;
		return true;
	}
	return false;
}

bool USolidWorldChart::FindLairMarker(
	const FString& LairId,
	const ESolidWorldChartMarkKind Kind,
	FSolidWorldChartMark& OutMark) const
{
	if (LairId.IsEmpty())
	{
		return false;
	}
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != Kind || !Mark.LairId.Equals(LairId, ESearchCase::IgnoreCase))
		{
			continue;
		}
		OutMark = Mark;
		return true;
	}
	return false;
}

bool USolidWorldChart::FindStrongholdMarker(const FString& Name, FSolidWorldChartMark& OutMark) const
{
	FString Want = Name;
	Want.ReplaceInline(TEXT(" "), TEXT(""));
	if (Want.IsEmpty())
	{
		return false;
	}
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::Stronghold)
		{
			continue;
		}
		FString Have = Mark.Name;
		Have.ReplaceInline(TEXT(" "), TEXT(""));
		if (!Have.Equals(Want, ESearchCase::IgnoreCase))
		{
			continue;
		}
		OutMark = Mark;
		return true;
	}
	return false;
}

bool USolidWorldChart::FindStrongholdMarker(
	const ESolidStrongholdSize Size,
	const ESolidStrongholdType Type,
	FSolidWorldChartMark& OutMark) const
{
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::Stronghold
			|| Mark.StrongholdSize != Size
			|| Mark.StrongholdType != Type)
		{
			continue;
		}
		OutMark = Mark;
		return true;
	}
	return false;
}

bool USolidWorldChart::FindStrongholdSiteMarker(
	const ESolidStrongholdSize Size,
	FSolidWorldChartMark& OutMark) const
{
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::StrongholdSite || Mark.StrongholdSize != Size)
		{
			continue;
		}
		OutMark = Mark;
		return true;
	}
	return false;
}

const TCHAR* USolidWorldChart::RoadTypeName(const SolidRoad::EType Type)
{
	switch (Type)
	{
	case SolidRoad::EType::Track:
		return TEXT("track");
	case SolidRoad::EType::Road:
		return TEXT("road");
	case SolidRoad::EType::Highway:
		return TEXT("highway");
	case SolidRoad::EType::Trail:
	default:
		return TEXT("trail");
	}
}

void USolidWorldChart::BuildListedRoads() const
{
	ResetRoads();
	for (SolidRoad::FSolidRoad Road : SolidRoad::Roads)
	{
		BuildRoad(&Road);
	}
}

void USolidWorldChart::ResetRoads() const
{
	RoutedRoads.Reset();
}

void USolidWorldChart::BuildRoad(SolidRoad::FSolidRoad* Road) const
{
	if (Road == nullptr)
	{
		UE_LOG(LogSolid, Warning, TEXT("WorldMap: no road."));
		return;
	}

	const bool bPlaced = Road->Locate(Marks);
	if (!bPlaced)
	{
		UE_LOG(LogSolid, Warning, TEXT("WorldChart: cannot locate both ends of %s -> %s."), *Road->From.Name, *Road->To.Name);
		return;
	}

	if (!Road->FindRoute(*this) || Road->Points.Num() <= 1)
	{
		UE_LOG(LogSolid, Warning, TEXT("WorldMap: no route for road %s -> %s."), *Road->From.Name, *Road->To.Name);
		return;
	}

	RoutedRoads.Add(*Road);
}

bool USolidWorldChart::IsOnRoad(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	const FVector2D World(WorldX, WorldY);
	for (const SolidRoad::FSolidRoad& Road : GetRoutedRoads())
	{
		if (Road.Points.Num() < 2)
		{
			continue;
		}
		const float HalfWidth = SolidRoad::WidthCm(Road.Type) * 0.5f;
		FVector2D Previous = FVector2D::ZeroVector;
		bool bHasPrevious = false;
		for (const FIntPoint& TerrainPoint : Road.Points)
		{
			FVector2D At = FVector2D::ZeroVector;
			if (!TerrainPointToWorld(TerrainPoint, WorldMinXY, WorldMaxXY, At))
			{
				bHasPrevious = false;
				continue;
			}
			if (bHasPrevious && SolidRoad::DistanceToSegmentCm(World, Previous, At) <= HalfWidth)
			{
				return true;
			}
			Previous = At;
			bHasPrevious = true;
		}
	}
	return false;
}
