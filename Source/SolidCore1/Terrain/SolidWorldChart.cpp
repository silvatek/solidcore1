#include "SolidWorldChart.h"
#include "SolidCore1.h"
#include "SolidLair.h"
#include "SolidStronghold.h"
#include "SolidTerrainMap.h"
#include "SolidTowns.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	ESolidWorldChartMarkKind ChartMarkKind(const ESolidWorldBang Kind)
	{
		switch (Kind)
		{
		case ESolidWorldBang::Stronghold:
			return ESolidWorldChartMarkKind::Stronghold;
		case ESolidWorldBang::StrongholdSite:
			return ESolidWorldChartMarkKind::StrongholdSite;
		case ESolidWorldBang::LairEntrance:
			return ESolidWorldChartMarkKind::LairEntrance;
		case ESolidWorldBang::LairExit:
			return ESolidWorldChartMarkKind::LairExit;
		case ESolidWorldBang::Town:
		default:
			return ESolidWorldChartMarkKind::Town;
		}
	}

	FString ChartRoadEndName(const SolidRoad::FEndpoint& End)
	{
		if (End.bStronghold)
		{
			return End.StrongholdName;
		}
		return SolidTowns::NameFor(End.LocationId);
	}
}

const TCHAR* USolidWorldChart::ShippedPath()
{
	return TEXT("/Game/World/WorldChart");
}

void USolidWorldChart::DefaultWorldBounds(FVector2D& OutMin, FVector2D& OutMax)
{
	const USolidTerrainMap* Terrain = GetDefault<USolidTerrainMap>();
	OutMin = Terrain->GetWorldMinXY();
	OutMax = Terrain->GetWorldMaxXY();
}

const TCHAR* USolidWorldChart::RoadTypeName(const ESolidWorldChartRoadType Type)
{
	switch (Type)
	{
	case ESolidWorldChartRoadType::Track:
		return TEXT("track");
	case ESolidWorldChartRoadType::Road:
		return TEXT("road");
	case ESolidWorldChartRoadType::Highway:
		return TEXT("highway");
	case ESolidWorldChartRoadType::Trail:
	default:
		return TEXT("trail");
	}
}

ESolidWorldChartRoadType USolidWorldChart::ChartRoadType(const SolidRoad::EType Type)
{
	switch (Type)
	{
	case SolidRoad::EType::Track:
		return ESolidWorldChartRoadType::Track;
	case SolidRoad::EType::Road:
		return ESolidWorldChartRoadType::Road;
	case SolidRoad::EType::Highway:
		return ESolidWorldChartRoadType::Highway;
	case SolidRoad::EType::Trail:
	default:
		return ESolidWorldChartRoadType::Trail;
	}
}

bool USolidWorldChart::ImportFromString(const FString& Text)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!Map->LoadFromString(Text))
	{
		return false;
	}

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	DefaultWorldBounds(WorldMin, WorldMax);

	TArray<ESolidBiome> NewCells;
	NewCells.SetNum(USolidWorldMap::MapWidth * USolidWorldMap::MapHeight);
	TArray<FSolidWorldChartMark> NewMarks;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const int32 Index = Row * USolidWorldMap::MapWidth + Col;
			NewCells[Index] = Map->GetMarkerBiomeAtCell(Col, Row);

			FSolidWorldBang Bang;
			if (!Map->FindBang(Col, Row, Bang))
			{
				continue;
			}

			FSolidWorldChartMark Mark;
			Mark.Kind = ChartMarkKind(Bang.Kind);
			Mark.Name = Bang.Name;
			Mark.StrongholdType = Bang.StrongholdType;
			Mark.StrongholdSize = Bang.StrongholdSize;
			Mark.LairId = Bang.LairId;
			FVector2D Centre = FVector2D::ZeroVector;
			if (!Map->CellToWorldXY(Bang.Cell, WorldMin, WorldMax, Centre))
			{
				continue;
			}
			Mark.PositionCm = Centre;
			NewMarks.Add(Mark);
		}
	}

	TArray<FSolidWorldChartRoad> NewRoads;
	for (const SolidRoad::FRoute& Route : Map->GetRoads())
	{
		FSolidWorldChartRoad Road;
		Road.FromName = ChartRoadEndName(Route.Definition.From);
		Road.ToName = ChartRoadEndName(Route.Definition.To);
		Road.Type = ChartRoadType(Route.Definition.Type);
		NewRoads.Add(Road);
	}

	Cells = MoveTemp(NewCells);
	Marks = MoveTemp(NewMarks);
	Roads = MoveTemp(NewRoads);
	UndoStack.Reset();
	return true;
}

void USolidWorldChart::ImportShippedWorldMap()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("WorldMap.txt");
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path) || !ImportFromString(Text))
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: failed to import Content/WorldMap.txt."));
		return;
	}
	Modify();
	UE_LOG(LogSolid, Warning, TEXT("WorldChart imported from Content/WorldMap.txt. Save this asset as %s."), ShippedPath());
}

namespace
{
	bool IsPaintBiome(const ESolidBiome Biome)
	{
		switch (Biome)
		{
		case ESolidBiome::Grassland:
		case ESolidBiome::Forest:
		case ESolidBiome::Mountain:
		case ESolidBiome::Town:
		case ESolidBiome::Desert:
		case ESolidBiome::Sea:
		case ESolidBiome::River:
			return true;
		default:
			return false;
		}
	}

	FString CompactName(FString Name)
	{
		Name.ReplaceInline(TEXT(" "), TEXT(""));
		return Name;
	}

	bool SameMark(const FSolidWorldChartMark& A, const FSolidWorldChartMark& B)
	{
		return A.Kind == B.Kind
			&& A.Name.Equals(B.Name, ESearchCase::IgnoreCase)
			&& FVector2D::Distance(A.PositionCm, B.PositionCm) <= 1.f
			&& A.StrongholdType == B.StrongholdType
			&& A.StrongholdSize == B.StrongholdSize
			&& A.LairId.Equals(B.LairId, ESearchCase::IgnoreCase);
	}

	bool SameRoad(const FSolidWorldChartRoad& A, const FSolidWorldChartRoad& B)
	{
		return A.Type == B.Type
			&& CompactName(A.FromName).Equals(CompactName(B.FromName), ESearchCase::IgnoreCase)
			&& CompactName(A.ToName).Equals(CompactName(B.ToName), ESearchCase::IgnoreCase);
	}

	/** Town, stronghold, site, or neither. A site is reported and is not an end. */
	enum class EChartRoadEnd : uint8
	{
		Missing,
		Town,
		Stronghold,
		Site,
	};

	EChartRoadEnd RoadEndKind(const TArray<FSolidWorldChartMark>& Marks, const FString& Name)
	{
		const FString Compact = CompactName(Name);
		if (Compact.IsEmpty())
		{
			return EChartRoadEnd::Missing;
		}

		EChartRoadEnd Found = EChartRoadEnd::Missing;
		for (const FSolidWorldChartMark& Mark : Marks)
		{
			if (Mark.Kind == ESolidWorldChartMarkKind::Town
				&& Mark.Name.Equals(Name.TrimStartAndEnd(), ESearchCase::IgnoreCase))
			{
				return EChartRoadEnd::Town;
			}
			if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold
				&& CompactName(Mark.Name).Equals(Compact, ESearchCase::IgnoreCase))
			{
				Found = EChartRoadEnd::Stronghold;
			}
			else if (Found != EChartRoadEnd::Stronghold
				&& Mark.Kind == ESolidWorldChartMarkKind::StrongholdSite
				&& Mark.Name.Equals(Name.TrimStartAndEnd(), ESearchCase::IgnoreCase))
			{
				Found = EChartRoadEnd::Site;
			}
		}
		return Found;
	}

	FString RoadEndSentence(const EChartRoadEnd Kind, const FString& Name)
	{
		const FString Trimmed = Name.TrimStartAndEnd();
		if (Kind == EChartRoadEnd::Site)
		{
			return FString::Printf(TEXT("%s is a stronghold site, not a road end."), *Trimmed);
		}
		if (Kind == EChartRoadEnd::Missing)
		{
			return FString::Printf(TEXT("%s is not a town or stronghold on the chart."), *Trimmed);
		}
		return FString();
	}
}

bool USolidWorldChart::CellCentre(const FIntPoint Cell, FVector2D& OutCm)
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= USolidWorldMap::MapWidth || Cell.Y >= USolidWorldMap::MapHeight)
	{
		return false;
	}

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	DefaultWorldBounds(WorldMin, WorldMax);
	const float ExtentX = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
	const float ExtentY = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellH = ExtentY / static_cast<float>(USolidWorldMap::MapHeight);
	// Column 0 is east. Row 0 is north. The centre matches USolidWorldMap::CellToWorldXY.
	OutCm.X = WorldMax.X - (static_cast<float>(Cell.X) + 0.5f) * CellW;
	OutCm.Y = WorldMax.Y - (static_cast<float>(Cell.Y) + 0.5f) * CellH;
	return true;
}

void USolidWorldChart::PushUndo()
{
	FSnapshot Snapshot;
	Snapshot.Cells = Cells;
	Snapshot.Marks = Marks;
	Snapshot.Roads = Roads;
	UndoStack.Add(MoveTemp(Snapshot));
}

bool USolidWorldChart::CellIndex(const FIntPoint Cell, int32& OutIndex) const
{
	if (Cells.Num() != USolidWorldMap::MapWidth * USolidWorldMap::MapHeight
		|| Cell.X < 0 || Cell.Y < 0
		|| Cell.X >= USolidWorldMap::MapWidth || Cell.Y >= USolidWorldMap::MapHeight)
	{
		return false;
	}
	OutIndex = Cell.Y * USolidWorldMap::MapWidth + Cell.X;
	return true;
}

bool USolidWorldChart::PaintRun(const TArray<FIntPoint>& CellsToPaint, const ESolidBiome Biome, FString& OutError)
{
	OutError.Reset();
	if (!IsPaintBiome(Biome))
	{
		OutError = TEXT("Swamp and road are not paint biomes.");
		return false;
	}
	if (Cells.Num() != USolidWorldMap::MapWidth * USolidWorldMap::MapHeight)
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	TArray<int32> Indexes;
	Indexes.Reserve(CellsToPaint.Num());
	for (const FIntPoint& Cell : CellsToPaint)
	{
		int32 Index = INDEX_NONE;
		if (!CellIndex(Cell, Index))
		{
			OutError = TEXT("That cell is off the map.");
			return false;
		}
		Indexes.AddUnique(Index);
	}
	if (Indexes.Num() == 0)
	{
		OutError = TEXT("Paint needs a cell.");
		return false;
	}

	PushUndo();
	for (const int32 Index : Indexes)
	{
		Cells[Index] = Biome;
	}
	return true;
}

bool USolidWorldChart::PaintCell(const FIntPoint Cell, const ESolidBiome Biome, FString& OutError)
{
	return PaintRun({ Cell }, Biome, OutError);
}

bool USolidWorldChart::PaintRectangle(const FIntPoint From, const FIntPoint To, const ESolidBiome Biome, FString& OutError)
{
	const int32 X0 = FMath::Min(From.X, To.X);
	const int32 X1 = FMath::Max(From.X, To.X);
	const int32 Y0 = FMath::Min(From.Y, To.Y);
	const int32 Y1 = FMath::Max(From.Y, To.Y);
	TArray<FIntPoint> Covered;
	Covered.Reserve((X1 - X0 + 1) * (Y1 - Y0 + 1));
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			Covered.Add(FIntPoint(X, Y));
		}
	}
	return PaintRun(Covered, Biome, OutError);
}

bool USolidWorldChart::PlaceMark(const FIntPoint Cell, const FSolidWorldChartMark& Mark, FString& OutError)
{
	OutError.Reset();
	FVector2D Centre = FVector2D::ZeroVector;
	if (!CellCentre(Cell, Centre) || Cells.Num() != USolidWorldMap::MapWidth * USolidWorldMap::MapHeight)
	{
		OutError = Cells.Num() == USolidWorldMap::MapWidth * USolidWorldMap::MapHeight
			? TEXT("That cell is off the map.")
			: TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	const int32 Existing = MarkAtCell(Cell);
	FSolidWorldChartMark Stored = Mark;
	Stored.PositionCm = Centre;
	if (Existing == INDEX_NONE)
	{
		Marks.Add(Stored);
	}
	else
	{
		Marks[Existing] = Stored;
	}
	return true;
}

bool USolidWorldChart::PlaceTown(const FIntPoint Cell, const FString& Name, FString& OutError)
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
	return PlaceMark(Cell, Mark, OutError);
}

bool USolidWorldChart::PlaceStronghold(
	const FIntPoint Cell,
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
	return PlaceMark(Cell, Mark, OutError);
}

bool USolidWorldChart::PlaceSite(const FIntPoint Cell, const ESolidStrongholdSize Size, FString& OutError)
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
	return PlaceMark(Cell, Mark, OutError);
}

bool USolidWorldChart::PlaceLairMouth(
	const FIntPoint Cell,
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
	return PlaceMark(Cell, Mark, OutError);
}

bool USolidWorldChart::DeleteMark(const FIntPoint Cell, FString& OutError)
{
	OutError.Reset();
	const int32 Existing = MarkAtCell(Cell);
	if (Existing == INDEX_NONE)
	{
		OutError = TEXT("No mark on this cell.");
		return false;
	}
	PushUndo();
	Marks.RemoveAt(Existing);
	return true;
}

FString USolidWorldChart::RoadEndError(const FString& From, const FString& To) const
{
	TArray<FString> Sentences;
	const EChartRoadEnd FromKind = RoadEndKind(Marks, From);
	const EChartRoadEnd ToKind = RoadEndKind(Marks, To);
	if (FromKind == EChartRoadEnd::Site || FromKind == EChartRoadEnd::Missing)
	{
		Sentences.Add(RoadEndSentence(FromKind, From));
	}
	if (ToKind == EChartRoadEnd::Site || ToKind == EChartRoadEnd::Missing)
	{
		Sentences.Add(RoadEndSentence(ToKind, To));
	}
	return FString::Join(Sentences, TEXT(" "));
}

bool USolidWorldChart::AddRoad(
	const FString& From,
	const FString& To,
	const ESolidWorldChartRoadType Type,
	FString& OutError)
{
	OutError = RoadEndError(From, To);
	if (From.TrimStartAndEnd().IsEmpty() || To.TrimStartAndEnd().IsEmpty())
	{
		OutError = TEXT("A road needs two names.");
		return false;
	}

	PushUndo();
	FSolidWorldChartRoad Road;
	Road.FromName = From.TrimStartAndEnd();
	Road.ToName = To.TrimStartAndEnd();
	Road.Type = Type;
	Roads.Add(Road);
	return true;
}

bool USolidWorldChart::RouteForRoad(const int32 RoadIndex, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	if (!Roads.IsValidIndex(RoadIndex))
	{
		return false;
	}

	// A name that is not a town or a stronghold does not parse, so the strip is empty.
	auto LineFor = [](const FSolidWorldChartRoad& Road)
	{
		return FString::Printf(
			TEXT("Road: %s -> %s, %s"),
			*Road.FromName,
			*Road.ToName,
			RoadTypeName(Road.Type));
	};

	SolidRoad::FDefinition ThisRoad;
	if (!SolidRoad::Parse(LineFor(Roads[RoadIndex]), ThisRoad))
	{
		return true;
	}

	int32 ParsedIndex = 0;
	for (int32 Index = 0; Index < RoadIndex; ++Index)
	{
		SolidRoad::FDefinition Earlier;
		if (SolidRoad::Parse(LineFor(Roads[Index]), Earlier))
		{
			++ParsedIndex;
		}
	}

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!Map->LoadFromChart(this))
	{
		return false;
	}
	if (Map->GetRoads().IsValidIndex(ParsedIndex))
	{
		OutCells = Map->GetRoads()[ParsedIndex].Cells;
	}
	return true;
}

int32 USolidWorldChart::MarkAtCell(const FIntPoint Cell) const
{
	FVector2D Centre = FVector2D::ZeroVector;
	if (!CellCentre(Cell, Centre))
	{
		return INDEX_NONE;
	}
	for (int32 Index = 0; Index < Marks.Num(); ++Index)
	{
		if (FVector2D::Distance(Marks[Index].PositionCm, Centre) <= 1.f)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool USolidWorldChart::ReadCell(const FIntPoint Cell, FSolidWorldChartRead& Out) const
{
	Out = FSolidWorldChartRead();
	int32 Index = INDEX_NONE;
	if (!CellIndex(Cell, Index))
	{
		return false;
	}

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	if (!Map->LoadFromChart(this))
	{
		return false;
	}

	Out.bValid = true;
	Out.Painted = Cells[Index];
	Out.Resolved = Map->GetBiomeAtCell(Cell.X, Cell.Y);
	Out.bMarkOverrides = Out.Resolved != Out.Painted;
	if (const int32 MarkIndex = MarkAtCell(Cell); MarkIndex != INDEX_NONE)
	{
		Out.bHasMark = true;
		Out.Mark = Marks[MarkIndex];
	}
	const TArray<SolidRoad::FRoute>& Routes = Map->GetRoads();
	for (int32 RoadIndex = 0; RoadIndex < Routes.Num(); ++RoadIndex)
	{
		if (Routes[RoadIndex].Cells.Contains(Cell))
		{
			Out.RoadIndexes.Add(RoadIndex);
		}
	}
	return true;
}

bool USolidWorldChart::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return false;
	}
	const FSnapshot Snapshot = UndoStack.Pop();
	Cells = Snapshot.Cells;
	Marks = Snapshot.Marks;
	Roads = Snapshot.Roads;
	return true;
}

FString USolidWorldChart::Disagreement(const FString& WorldMapText) const
{
	USolidWorldChart* Other = NewObject<USolidWorldChart>();
	if (!Other->ImportFromString(WorldMapText))
	{
		return TEXT("Content/WorldMap.txt did not load.");
	}
	if (Cells.Num() != Other->Cells.Num())
	{
		return TEXT("The chart and Content/WorldMap.txt disagree.");
	}
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		if (Cells[Index] != Other->Cells[Index])
		{
			return TEXT("The chart and Content/WorldMap.txt disagree.");
		}
	}
	if (Marks.Num() != Other->Marks.Num() || Roads.Num() != Other->Roads.Num())
	{
		return TEXT("The chart and Content/WorldMap.txt disagree.");
	}

	TArray<int32> UsedMarks;
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		int32 Found = INDEX_NONE;
		for (int32 Index = 0; Index < Other->Marks.Num(); ++Index)
		{
			if (UsedMarks.Contains(Index) || !SameMark(Mark, Other->Marks[Index]))
			{
				continue;
			}
			Found = Index;
			break;
		}
		if (Found == INDEX_NONE)
		{
			return TEXT("The chart and Content/WorldMap.txt disagree.");
		}
		UsedMarks.Add(Found);
	}

	TArray<int32> UsedRoads;
	for (const FSolidWorldChartRoad& Road : Roads)
	{
		int32 Found = INDEX_NONE;
		for (int32 Index = 0; Index < Other->Roads.Num(); ++Index)
		{
			if (UsedRoads.Contains(Index) || !SameRoad(Road, Other->Roads[Index]))
			{
				continue;
			}
			Found = Index;
			break;
		}
		if (Found == INDEX_NONE)
		{
			return TEXT("The chart and Content/WorldMap.txt disagree.");
		}
		UsedRoads.Add(Found);
	}
	return FString();
}
