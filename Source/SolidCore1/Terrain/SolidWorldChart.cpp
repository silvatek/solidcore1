#include "SolidWorldChart.h"
#include "SolidRoads.h"
#include "SolidBiome.h"
#include "SolidCore1.h"
#include "SolidLair.h"
#include "Events/SolidEncounters.h"
#include "SolidStronghold.h"
#include "SolidTerrainMap.h"
#include "SolidTowns.h"
#include "SolidRiver.h"
#include "Game/SolidGameMode.h"

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
	if (!LoadFromString(Text))
	{
		return false;
	}

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	DefaultWorldBounds(WorldMin, WorldMax);

	TArray<ESolidBiome> NewCells;
	NewCells.SetNum(MapWidth * MapHeight);
	TArray<FSolidWorldChartMark> NewMarks;
	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const int32 Index = Row * MapWidth + Col;
			NewCells[Index] = GetMarkerBiomeAtCell(Col, Row);

			FSolidWorldBang Bang;
			if (!FindBang(Col, Row, Bang))
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
			if (!CellToWorldXY(Bang.Cell, WorldMin, WorldMax, Centre))
			{
				continue;
			}
			Mark.PositionCm = Centre;
			NewMarks.Add(Mark);
		}
	}

	TArray<FSolidWorldChartRoad> NewRoads;
	for (const SolidRoad::FRoute& Route : GetRoads())
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

namespace
{
	FString CompactName(FString Name)
	{
		Name.ReplaceInline(TEXT(" "), TEXT(""));
		return Name;
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

	/**
	 * The values the place tool selected. Position is not one of them.
	 * A town matches its name. A stronghold and a site match their size.
	 * A lair mouth matches its lair and its end. A note matches its text.
	 * An encounter matches its name.
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
			return Existing.Name.Equals(Incoming.Name, ESearchCase::IgnoreCase);
		default:
			return false;
		}
	}
}

bool USolidWorldChart::CellCentre(const FIntPoint Cell, FVector2D& OutCm)
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= USolidWorldChart::MapWidth || Cell.Y >= USolidWorldChart::MapHeight)
	{
		return false;
	}

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	DefaultWorldBounds(WorldMin, WorldMax);
	const float ExtentX = FMath::Max(WorldMax.X - WorldMin.X, 1.f);
	const float ExtentY = FMath::Max(WorldMax.Y - WorldMin.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(USolidWorldChart::MapWidth);
	const float CellH = ExtentY / static_cast<float>(USolidWorldChart::MapHeight);
	// Column 0 is west. Row 0 is north. The centre matches USolidWorldChart::CellToWorldXY.
	OutCm.X = WorldMax.X - (static_cast<float>(Cell.X) + 0.5f) * CellW;
	OutCm.Y = WorldMax.Y - (static_cast<float>(Cell.Y) + 0.5f) * CellH;
	return true;
}

FVector2D USolidWorldChart::TerrainPosition(const FVector2D& PositionCm, const FVector2D& TerrainMin)
{
	FVector2D ChartMin = FVector2D::ZeroVector;
	FVector2D ChartMax = FVector2D::ZeroVector;
	DefaultWorldBounds(ChartMin, ChartMax);
	return PositionCm + (TerrainMin - ChartMin);
}

void USolidWorldChart::PushUndo()
{
	FSnapshot Snapshot;
	Snapshot.Cells = Cells;
	Snapshot.Points = Points;
	Snapshot.Marks = Marks;
	Snapshot.Roads = Roads;
	UndoStack.Add(MoveTemp(Snapshot));
}

bool USolidWorldChart::CellIndex(const FIntPoint Cell, int32& OutIndex) const
{
	if (Cells.Num() != USolidWorldChart::MapWidth * USolidWorldChart::MapHeight
		|| Cell.X < 0 || Cell.Y < 0
		|| Cell.X >= USolidWorldChart::MapWidth || Cell.Y >= USolidWorldChart::MapHeight)
	{
		return false;
	}
	OutIndex = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
	return true;
}

bool USolidWorldChart::PaintRun(const TArray<FIntPoint>& CellsToPaint, const ESolidBiome Biome, FString& OutError)
{
	OutError.Reset();
	if (!SolidBiome::Of(Biome).Paintable())
	{
		OutError = TEXT("Swamp and road are not paint biomes.");
		return false;
	}
	if (Cells.Num() != USolidWorldChart::MapWidth * USolidWorldChart::MapHeight)
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
	if (!EnsurePoints())
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	for (const int32 Index : Indexes)
	{
		Cells[Index] = Biome;
	}
	SetPointsInCells(Indexes, Biome);
	for (const int32 Index : Indexes)
	{
		const int32 MarkIndex = MarkAtCell(FIntPoint(Index % MapWidth, Index / MapWidth));
		if (MarkIndex != INDEX_NONE)
		{
			ApplyMarkerPoint(Marks[MarkIndex].PositionCm, Marks[MarkIndex].Kind);
		}
	}
	InvalidateQuery();
	return true;
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
	if (!CellCentre(Cell, Centre) || Cells.Num() != USolidWorldChart::MapWidth * USolidWorldChart::MapHeight)
	{
		OutError = Cells.Num() == USolidWorldChart::MapWidth * USolidWorldChart::MapHeight
			? TEXT("That cell is off the map.")
			: TEXT("The chart does not cover the map.");
		return false;
	}
	if (!EnsurePoints())
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	FSolidWorldChartMark Stored = Mark;
	Stored.PositionCm = Centre;
	TArray<FVector2D> Released;
	for (int32 Index = Marks.Num() - 1; Index >= 0; --Index)
	{
		const bool bHere = FVector2D::Distance(Marks[Index].PositionCm, Centre) <= 1.f;
		if (!bHere && !SamePlacedMark(Marks[Index], Stored))
		{
			continue;
		}
		Released.Add(Marks[Index].PositionCm);
		Marks.RemoveAt(Index);
	}
	Marks.Add(Stored);
	for (const FVector2D& Position : Released)
	{
		ReleaseMarkerPoint(Position);
	}
	ApplyMarkerPoint(Stored.PositionCm, Stored.Kind);
	InvalidateQuery();
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

bool USolidWorldChart::PlaceCustom(const FIntPoint Cell, const FString& Note, FString& OutError)
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
	return PlaceMark(Cell, Mark, OutError);
}

bool USolidWorldChart::PlaceEncounter(const FIntPoint Cell, const FString& Name, FString& OutError)
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
	InvalidateQuery();
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
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::RouteForRoad(const int32 RoadIndex, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	if (!Roads.IsValidIndex(RoadIndex))
	{
		return false;
	}
	if (!EnsureQuery())
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

	if (GetRoads().IsValidIndex(ParsedIndex))
	{
		OutCells = GetRoads()[ParsedIndex].Cells;
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

bool USolidWorldChart::Undo()
{
	if (UndoStack.Num() == 0)
	{
		return false;
	}
	const FSnapshot Snapshot = UndoStack.Pop();
	Cells = Snapshot.Cells;
	Points = Snapshot.Points;
	Marks = Snapshot.Marks;
	Roads = Snapshot.Roads;
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::EnsureQuery() const
{
	if (bIsLoaded && Markers.Num() == MapWidth * MapHeight)
	{
		return true;
	}
	if (Cells.Num() != MapWidth * MapHeight)
	{
		return false;
	}
	return BuildQuery();
}

void USolidWorldChart::InvalidateQuery()
{
	bIsLoaded = false;
}

namespace
{
	bool WorldMapMarkerMatches(const TCHAR Raw, const TCHAR Expected)
	{
		return Raw == Expected || FChar::ToUpper(Raw) == Expected;
	}

	TCHAR WorldMapCanonicalMarker(const TCHAR Raw)
	{
		const TCHAR Expected[] = {
			USolidWorldChart::GrassMarker,
			USolidWorldChart::TownMarker,
			USolidWorldChart::ForestMarker,
			USolidWorldChart::RiverMarker,
			USolidWorldChart::SeaMarker,
			USolidWorldChart::DesertMarker,
			USolidWorldChart::LocationMarker,
		};
		for (const TCHAR Marker : Expected)
		{
			if (WorldMapMarkerMatches(Raw, Marker))
			{
				return Marker;
			}
		}
		return FChar::IsDigit(Raw) ? Raw : FChar::ToUpper(Raw);
	}
}

bool USolidWorldChart::IsGridMarker(const TCHAR Marker)
{
	const TCHAR Upper = FChar::ToUpper(Marker);
	if ((Upper >= TEXT('A') && Upper <= TEXT('Z')) || FChar::IsDigit(Marker))
	{
		return true;
	}
	return WorldMapMarkerMatches(Marker, GrassMarker)
		|| WorldMapMarkerMatches(Marker, TownMarker)
		|| WorldMapMarkerMatches(Marker, ForestMarker)
		|| WorldMapMarkerMatches(Marker, RiverMarker)
		|| WorldMapMarkerMatches(Marker, SeaMarker)
		|| WorldMapMarkerMatches(Marker, DesertMarker)
		|| WorldMapMarkerMatches(Marker, LocationMarker);
}

ESolidBiome USolidWorldChart::MarkerToBiome(const TCHAR Marker)
{
	if (WorldMapMarkerMatches(Marker, SeaMarker))
	{
		return ESolidBiome::Sea;
	}
	if (WorldMapMarkerMatches(Marker, GrassMarker))
	{
		return ESolidBiome::Grassland;
	}
	if (WorldMapMarkerMatches(Marker, ForestMarker))
	{
		return ESolidBiome::Forest;
	}
	if (WorldMapMarkerMatches(Marker, TownMarker))
	{
		return ESolidBiome::Town;
	}
	if (WorldMapMarkerMatches(Marker, RiverMarker))
	{
		return ESolidBiome::River;
	}
	if (WorldMapMarkerMatches(Marker, DesertMarker))
	{
		return ESolidBiome::Desert;
	}
	if (WorldMapMarkerMatches(Marker, LocationMarker))
	{
		return ESolidBiome::Grassland;
	}

	// A named town, lair, or defined stronghold upgrades this in GetBiomeAtCell.
	return FChar::ToUpper(Marker) == TEXT('M') ? ESolidBiome::Mountain : ESolidBiome::Grassland;
}

FLinearColor USolidWorldChart::DefaultColorForBiome(const ESolidBiome Biome)
{
	return SolidBiome::Of(Biome).Color();
}

namespace
{
	bool WorldMapDigits(const FString& Text)
	{
		if (Text.IsEmpty())
		{
			return false;
		}
		for (const TCHAR Char : Text)
		{
			if (!FChar::IsDigit(Char))
			{
				return false;
			}
		}
		return true;
	}

	struct FWorldMapBangRef
	{
		enum class EKind : uint8
		{
			Skip,
			Town,
			Stronghold,
			StrongholdSite,
			LairEntrance,
			LairExit,
		};

		int32 MapX = INDEX_NONE;
		int32 MapY = INDEX_NONE;
		EKind Kind = EKind::Skip;
		FString Identifier;
	};

	/**
	 * "! 8,21 = Town Iglin" — row 8, column 21, counting from 1 at the top-left.
	 * "! 9,12 = Stronghold MeagreDungeon", "! 12,12 = StrongholdSite (Humble)",
	 * "! 10,11 = LairEntrance Iglin tunnels", "! 11,40 = LairExit Iglin tunnels".
	 * Anything after # is a comment. A numeric right-hand side is ignored.
	 * Returns false when the line is not that shape.
	 * An out-of-range coordinate is that shape: MapX stays INDEX_NONE and the line is consumed.
	 */
	bool WorldMapParseBangLocation(const FString& Line, FWorldMapBangRef& Out)
	{
		Out = FWorldMapBangRef();

		FString Trimmed = Line.TrimStartAndEnd();
		int32 Hash = INDEX_NONE;
		if (Trimmed.FindChar(TEXT('#'), Hash))
		{
			Trimmed.LeftInline(Hash);
			Trimmed.TrimEndInline();
		}
		if (Trimmed.IsEmpty() || Trimmed[0] != USolidWorldChart::LocationMarker)
		{
			return false;
		}

		int32 EqIndex = INDEX_NONE;
		if (!Trimmed.FindChar(TEXT('='), EqIndex) || EqIndex < 2)
		{
			return false;
		}

		const FString Left = Trimmed.Mid(1, EqIndex - 1).TrimStartAndEnd();
		const FString Right = Trimmed.Mid(EqIndex + 1).TrimStartAndEnd();
		int32 Comma = INDEX_NONE;
		if (!Left.FindChar(TEXT(','), Comma))
		{
			return false;
		}

		const FString RowText = Left.Left(Comma).TrimStartAndEnd();
		const FString ColumnText = Left.Mid(Comma + 1).TrimStartAndEnd();
		if (!WorldMapDigits(RowText) || !WorldMapDigits(ColumnText) || Right.IsEmpty())
		{
			return false;
		}

		const int32 Row = FCString::Atoi(*RowText);
		const int32 Column = FCString::Atoi(*ColumnText);
		if (Row < 1 || Column < 1
			|| Row > USolidWorldChart::MapHeight
			|| Column > USolidWorldChart::MapWidth)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d is outside the grid; ignored."),
				Row, Column);
			return true;
		}

		Out.MapX = Column - 1;
		Out.MapY = Row - 1;

		if (FChar::IsDigit(Right[0]))
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d uses a number; name the town, stronghold, or lair."),
				Row, Column);
			Out.Kind = FWorldMapBangRef::EKind::Skip;
			return true;
		}

		FString Type;
		FString Identifier;
		if (!Right.Split(TEXT(" "), &Type, &Identifier))
		{
			Type = Right;
		}
		Type.TrimStartAndEndInline();
		Identifier.TrimStartAndEndInline();
		if (Identifier.Len() >= 2
			&& Identifier[0] == TEXT('(')
			&& Identifier[Identifier.Len() - 1] == TEXT(')'))
		{
			Identifier = Identifier.Mid(1, Identifier.Len() - 2).TrimStartAndEnd();
		}
		Out.Identifier = Identifier;
		if (Type.Equals(TEXT("Town"), ESearchCase::IgnoreCase))
		{
			Out.Kind = FWorldMapBangRef::EKind::Town;
		}
		else if (Type.Equals(TEXT("Stronghold"), ESearchCase::IgnoreCase))
		{
			Out.Kind = FWorldMapBangRef::EKind::Stronghold;
		}
		else if (Type.Equals(TEXT("StrongholdSite"), ESearchCase::IgnoreCase))
		{
			Out.Kind = FWorldMapBangRef::EKind::StrongholdSite;
		}
		else if (Type.Equals(TEXT("LairEntrance"), ESearchCase::IgnoreCase))
		{
			Out.Kind = FWorldMapBangRef::EKind::LairEntrance;
		}
		else if (Type.Equals(TEXT("LairExit"), ESearchCase::IgnoreCase))
		{
			Out.Kind = FWorldMapBangRef::EKind::LairExit;
		}
		else
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d has unknown type '%s'; ignored."),
				Row, Column, *Type);
			Out.Kind = FWorldMapBangRef::EKind::Skip;
		}
		return true;
	}

	void LocateStrongholdEnd(const TMap<int32, FSolidWorldBang>& Marks, SolidRoad::FEndpoint& End)
	{
		if (!End.bStronghold)
		{
			return;
		}
		FString Want = End.StrongholdName;
		Want.ReplaceInline(TEXT(" "), TEXT(""));
		for (const TPair<int32, FSolidWorldBang>& Pair : Marks)
		{
			const FSolidWorldBang& Mark = Pair.Value;
			if (Mark.Kind != ESolidWorldBang::Stronghold)
			{
				continue;
			}
			FString Have = Mark.Name;
			Have.ReplaceInline(TEXT(" "), TEXT(""));
			if (!Have.Equals(Want, ESearchCase::IgnoreCase))
			{
				continue;
			}
			End.Cell = Mark.Cell;
			End.bLocated = true;
			return;
		}
	}
}

bool USolidWorldChart::LoadFromString(const FString& Text)
{
	TArray<FString> Lines;
	Text.ParseIntoArrayLines(Lines, /*bCullEmpty=*/false);

	TArray<FString> GridLines;
	GridLines.Reserve(MapHeight);
	for (const FString& RawLine : Lines)
	{
		const FString Line = RawLine.TrimStartAndEnd();
		if (Line.Len() == MapWidth)
		{
			bool bAllMarkers = true;
			for (int32 i = 0; i < Line.Len(); ++i)
			{
				if (!IsGridMarker(Line[i]))
				{
					bAllMarkers = false;
					break;
				}
			}
			if (bAllMarkers)
			{
				GridLines.Add(Line);
				if (GridLines.Num() == MapHeight)
				{
					break;
				}
			}
		}
	}

	if (GridLines.Num() != MapHeight)
	{
		UE_LOG(LogSolid, Error,
			TEXT("WorldMap: expected %d grid rows of width %d, found %d."),
			MapHeight, MapWidth, GridLines.Num());
		return false;
	}

	ResetLoad();

	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		const FString& RowText = GridLines[Row];
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const TCHAR Marker = WorldMapCanonicalMarker(RowText[Col]);
			Markers[Row * MapWidth + Col] = static_cast<uint8>(Marker);
		}
	}

	TArray<SolidRoad::FDefinition> RoadDefs;
	TArray<FWorldMapBangRef> BangRefs;
	for (const FString& RawLine : Lines)
	{
		const FString Trimmed = RawLine.TrimStartAndEnd();
		SolidRoad::FDefinition RoadDef;
		FWorldMapBangRef BangRef;
		if (Trimmed.StartsWith(TEXT("Road:"), ESearchCase::IgnoreCase))
		{
			if (SolidRoad::Parse(Trimmed, RoadDef))
			{
				RoadDefs.Add(RoadDef);
			}
			else
			{
				UE_LOG(LogSolid, Warning, TEXT("WorldMap: ignored road line '%s'."), *Trimmed);
			}
		}
		else if (WorldMapParseBangLocation(Trimmed, BangRef))
		{
			BangRefs.Add(BangRef);
		}
	}

	for (const FWorldMapBangRef& Ref : BangRefs)
	{
		if (Ref.MapX == INDEX_NONE || Ref.MapY == INDEX_NONE || Ref.Kind == FWorldMapBangRef::EKind::Skip)
		{
			continue;
		}
		const int32 Index = Ref.MapY * MapWidth + Ref.MapX;
		const int32 Row = Ref.MapY + 1;
		const int32 Column = Ref.MapX + 1;
		if (static_cast<TCHAR>(Markers[Index]) != LocationMarker)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d is not an exclamation mark; ignored."),
				Row, Column);
			continue;
		}
		if (BangMarks.Contains(Index))
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d is already described; keeping it."),
				Row, Column);
			continue;
		}

		FSolidWorldBang Mark;
		Mark.Cell = FIntPoint(Ref.MapX, Ref.MapY);
		switch (Ref.Kind)
		{
		case FWorldMapBangRef::EKind::Town:
		{
			const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Ref.Identifier);
			if (!Town)
			{
				UE_LOG(LogSolid, Warning,
					TEXT("WorldMap: ! %d,%d town '%s' is unknown; ignored."),
					Row, Column, *Ref.Identifier);
				continue;
			}
			Mark.Kind = ESolidWorldBang::Town;
			Mark.LocationId = Town->Id;
			Mark.Name = Town->Name;
			BangLocationIds[Index] = Town->Id;
			FTownSeed Seed;
			Seed.LocationId = Town->Id;
			Seed.Column = Ref.MapX;
			Seed.Row = Ref.MapY;
			TownSeeds.Add(Seed);
			break;
		}
		case FWorldMapBangRef::EKind::Stronghold:
		{
			ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
			ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
			if (!SolidStronghold::FromToken(Ref.Identifier, HoldType, HoldSize))
			{
				UE_LOG(LogSolid, Warning,
					TEXT("WorldMap: ! %d,%d stronghold '%s' is unknown; ignored."),
					Row, Column, *Ref.Identifier);
				continue;
			}
			Mark.Kind = ESolidWorldBang::Stronghold;
			Mark.StrongholdType = HoldType;
			Mark.StrongholdSize = HoldSize;
			Mark.Name = SolidStronghold::Label(FSolidStronghold(HoldType, HoldSize));
			break;
		}
		case FWorldMapBangRef::EKind::StrongholdSite:
		{
			ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
			if (!SolidStronghold::SizeFromToken(Ref.Identifier, Size))
			{
				UE_LOG(LogSolid, Warning,
					TEXT("WorldMap: ! %d,%d stronghold site '%s' is unknown; ignored."),
					Row, Column, *Ref.Identifier);
				continue;
			}
			Mark.Kind = ESolidWorldBang::StrongholdSite;
			Mark.StrongholdSize = Size;
			Mark.Name = SolidStronghold::SizeName(Size);
			break;
		}
		case FWorldMapBangRef::EKind::LairEntrance:
		case FWorldMapBangRef::EKind::LairExit:
		{
			const SolidLair::FDefinition* Lair = SolidLair::FindByName(Ref.Identifier);
			if (!Lair)
			{
				UE_LOG(LogSolid, Warning,
					TEXT("WorldMap: ! %d,%d lair '%s' is unknown; ignored."),
					Row, Column, *Ref.Identifier);
				continue;
			}
			Mark.Kind = Ref.Kind == FWorldMapBangRef::EKind::LairEntrance
				? ESolidWorldBang::LairEntrance
				: ESolidWorldBang::LairExit;
			Mark.Name = Lair->Name;
			Mark.LairId = Lair->Id;
			break;
		}
		default:
			continue;
		}
		BangMarks.Add(Index, Mark);
	}

	CollectNamedTowns();

	bIsLoaded = true;
	CopyPlayOntoPoints(true, false);
	AssignTownPoints();
	for (SolidRoad::FDefinition& RoadDef : RoadDefs)
	{
		LocateStrongholdEnd(BangMarks, RoadDef.From);
		LocateStrongholdEnd(BangMarks, RoadDef.To);
	}
	BuildRoads(RoadDefs);
	UE_LOG(LogSolid, Warning,
		TEXT("WorldMap loaded: %dx%d startTownCells=%d locations=%d roads=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), Locations.Num(), SolvedRoads.Num());
	return true;
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
	Markers.SetNum(MapWidth * MapHeight);
	StartTownCells.Reset();
	Locations.Reset();
	SolvedRoads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	bHasStartTown = false;
	BangMarks.Reset();
	BangLocationIds.Init(INDEX_NONE, MapWidth * MapHeight);
	EncounterMarks.Reset();
	TownSeeds.Reset();
	PointTowns.Reset();
	bIsLoaded = false;
}

void USolidWorldChart::CollectNamedTowns() const
{
	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const int32 Id = ResolveLocationId(Col, Row);
			if (Id == INDEX_NONE)
			{
				continue;
			}
			FSolidWorldLocation& Loc = Locations.FindOrAdd(Id);
			if (Loc.Id == INDEX_NONE)
			{
				Loc.Id = Id;
				Loc.Biome = ESolidBiome::Town;
			}
			Loc.Cells.Add(FIntPoint(Col, Row));
			if (Id == 0)
			{
				StartTownCells.Add(FIntPoint(Col, Row));
			}
		}
	}
	bHasStartTown = StartTownCells.Num() > 0;

	for (TPair<int32, FSolidWorldLocation>& Pair : Locations)
	{
		Pair.Value.Name = SolidTowns::NameFor(Pair.Key);
	}
}

namespace
{
	/** Swamp and road are not marker biomes, so a chart cell of either loads as grassland. */
	TCHAR MarkerForChartBiome(const ESolidBiome Biome)
	{
		const FSolidBiome& Ground = SolidBiome::Of(Biome);
		return Ground.Paintable() ? Ground.Marker() : USolidWorldChart::GrassMarker;
	}
}

bool USolidWorldChart::RememberChartMark(const FSolidWorldChartMark& Mark, const FIntPoint Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= MapWidth || Cell.Y >= MapHeight)
	{
		return false;
	}
	const int32 Index = Cell.Y * MapWidth + Cell.X;
	if (BangMarks.Contains(Index))
	{
		return false;
	}

	FSolidWorldBang Bang;
	Bang.Cell = Cell;
	switch (Mark.Kind)
	{
	case ESolidWorldChartMarkKind::Town:
	{
		const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
		if (!Town)
		{
			return false;
		}
		Bang.Kind = ESolidWorldBang::Town;
		Bang.LocationId = Town->Id;
		Bang.Name = Town->Name;
		BangLocationIds[Index] = Town->Id;
		FTownSeed Seed;
		Seed.LocationId = Town->Id;
		Seed.Column = Cell.X;
		Seed.Row = Cell.Y;
		TownSeeds.Add(Seed);
		break;
	}
	case ESolidWorldChartMarkKind::Stronghold:
	{
		ESolidStrongholdType HoldType = Mark.StrongholdType;
		ESolidStrongholdSize HoldSize = Mark.StrongholdSize;
		SolidStronghold::FromToken(Mark.Name, HoldType, HoldSize);
		Bang.Kind = ESolidWorldBang::Stronghold;
		Bang.StrongholdType = HoldType;
		Bang.StrongholdSize = HoldSize;
		Bang.Name = SolidStronghold::Label(FSolidStronghold(HoldType, HoldSize));
		break;
	}
	case ESolidWorldChartMarkKind::StrongholdSite:
	{
		ESolidStrongholdSize Size = Mark.StrongholdSize;
		SolidStronghold::SizeFromToken(Mark.Name, Size);
		Bang.Kind = ESolidWorldBang::StrongholdSite;
		Bang.StrongholdSize = Size;
		Bang.Name = SolidStronghold::SizeName(Size);
		break;
	}
	case ESolidWorldChartMarkKind::LairEntrance:
	case ESolidWorldChartMarkKind::LairExit:
	{
		const SolidLair::FDefinition* Lair = SolidLair::FindByName(Mark.Name);
		if (!Lair)
		{
			return false;
		}
		Bang.Kind = Mark.Kind == ESolidWorldChartMarkKind::LairEntrance
			? ESolidWorldBang::LairEntrance
			: ESolidWorldBang::LairExit;
		Bang.Name = Lair->Name;
		Bang.LairId = Lair->Id;
		break;
	}
	case ESolidWorldChartMarkKind::Custom:
	case ESolidWorldChartMarkKind::Encounter:
		return false;
	default:
		return false;
	}

	// A town id and the text file both use !. The chart stored that cell as grassland.
	Markers[Index] = static_cast<uint8>(LocationMarker);
	BangMarks.Add(Index, Bang);
	return true;
}

bool USolidWorldChart::LoadFromChart(const USolidWorldChart* Chart)
{
	if (!Chart || Chart->Cells.Num() != MapWidth * MapHeight)
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: the chart does not cover the %dx%d map."), MapWidth, MapHeight);
		return false;
	}
	if (Chart != this)
	{
		Cells = Chart->Cells;
		Marks = Chart->Marks;
		Roads = Chart->Roads;
		const int32 PointCount = PointAxis * PointAxis;
		Points = Chart->Points.Num() == PointCount ? Chart->Points : TArray<ESolidBiome>();
		UndoStack.Reset();
	}
	return BuildQuery();
}

bool USolidWorldChart::BuildQuery() const
{
	if (Cells.Num() != MapWidth * MapHeight)
	{
		UE_LOG(LogSolid, Error, TEXT("WorldChart: the chart does not cover the %dx%d map."), MapWidth, MapHeight);
		return false;
	}

	ResetLoad();
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		Markers[Index] = static_cast<uint8>(MarkerForChartBiome(Cells[Index]));
	}

	// A mark position becomes a cell once. That conversion needs a loaded map.
	bIsLoaded = true;

	FVector2D WorldMin = FVector2D::ZeroVector;
	FVector2D WorldMax = FVector2D::ZeroVector;
	USolidWorldChart::DefaultWorldBounds(WorldMin, WorldMax);
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind == ESolidWorldChartMarkKind::Encounter)
		{
			FSolidWorldEncounterMark Found;
			Found.Name = Mark.Name;
			Found.PositionCm = Mark.PositionCm;
			EncounterMarks.Add(Found);
		}
		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!CellAtWorld(Mark.PositionCm.X, Mark.PositionCm.Y, WorldMin, WorldMax, Cell))
		{
			continue;
		}
		RememberChartMark(Mark, Cell);
	}

	CollectNamedTowns();
	CopyPlayOntoPoints(false, true);
	AssignTownPoints();

	if (SolidRoad::bRoadsFromChart)
	{
		TArray<SolidRoad::FDefinition> RoadDefs;
		auto AppendRoad = [&RoadDefs](const FString& From, const FString& To, const TCHAR* TypeName) -> SolidRoad::FDefinition*
		{
			const FString Line = FString::Printf(TEXT("Road: %s -> %s, %s"), *From, *To, TypeName);
			SolidRoad::FDefinition RoadDef;
			if (!SolidRoad::Parse(Line, RoadDef))
			{
				UE_LOG(LogSolid, Warning, TEXT("WorldChart: ignored road %s -> %s."), *From, *To);
				return nullptr;
			}
			RoadDefs.Add(RoadDef);
			return &RoadDefs.Last();
		};
		for (const FSolidWorldChartRoad& Road : Roads)
		{
			AppendRoad(Road.FromName, Road.ToName, RoadTypeName(Road.Type));
		}
		for (SolidRoad::FDefinition& RoadDef : RoadDefs)
		{
			LocateStrongholdEnd(BangMarks, RoadDef.From);
			LocateStrongholdEnd(BangMarks, RoadDef.To);
		}
		BuildRoads(RoadDefs);
	}
	else
	{
		AppendListedRoads();
	}

	bIsLoaded = true;
	UE_LOG(LogSolid, Warning,
		TEXT("WorldChart loaded: %dx%d startTownCells=%d locations=%d roads=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), Locations.Num(), SolvedRoads.Num());
	return true;
}

void USolidWorldChart::AppendListedRoads() const
{
	auto FillEnd = [](const FSolidWorldChartMark& Mark, SolidRoad::FEndpoint& Out) -> bool
	{
		Out = SolidRoad::FEndpoint();
		if (Mark.Kind == ESolidWorldChartMarkKind::Town)
		{
			const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
			if (Town == nullptr)
			{
				return false;
			}
			Out.LocationId = Town->Id;
			return true;
		}
		if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			Out.bStronghold = true;
			Out.StrongholdName = Mark.Name;
			return true;
		}
		return false;
	};

	ResetRoads();
	for (SolidRoad::FRoad Road : SolidRoad::Roads)
	{
		SolidRoad::LocateMark(Marks, Road.From);
		SolidRoad::LocateMark(Marks, Road.To);
		SolidRoad::FDefinition Definition;
		Definition.Type = Road.Type;
		if (!FillEnd(Road.From, Definition.From) || !FillEnd(Road.To, Definition.To))
		{
			UE_LOG(LogSolid, Warning, TEXT("WorldChart: ignored road %s -> %s."), *Road.From.Name, *Road.To.Name);
			continue;
		}
		if (Road.From.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			LocateStrongholdEnd(BangMarks, Definition.From);
		}
		if (Road.To.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			LocateStrongholdEnd(BangMarks, Definition.To);
		}
		BuildRoad(Definition, &Road);
	}
}

ESolidBiome USolidWorldChart::GetMarkerBiomeAtCell(const int32 MapX, const int32 MapY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return ESolidBiome::Grassland;
	}
	const int32 X = FMath::Clamp(MapX, 0, MapWidth - 1);
	const int32 Y = FMath::Clamp(MapY, 0, MapHeight - 1);
	return MarkerToBiome(static_cast<TCHAR>(Markers[Y * MapWidth + X]));
}

ESolidBiome USolidWorldChart::GetBiomeAtCell(int32 MapX, int32 MapY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return ESolidBiome::Grassland;
	}
	MapX = FMath::Clamp(MapX, 0, MapWidth - 1);
	MapY = FMath::Clamp(MapY, 0, MapHeight - 1);
	const int32 Id = ResolveLocationId(MapX, MapY);
	if (Id != INDEX_NONE)
	{
		if (const FSolidWorldLocation* Loc = Locations.Find(Id))
		{
			return Loc->Biome;
		}
		return ESolidBiome::Town;
	}
	if (const FSolidWorldBang* Mark = BangMarks.Find(MapY * MapWidth + MapX))
	{
		if (Mark->Kind == ESolidWorldBang::Stronghold)
		{
			const FSolidStronghold Attributes(Mark->StrongholdType, Mark->StrongholdSize);
			ESolidBiome Shelf = ESolidBiome::Grassland;
			if (SolidStronghold::BiomeFor(Attributes, Shelf))
			{
				return Shelf;
			}
		}
		else if (Mark->Kind == ESolidWorldBang::LairEntrance
			|| Mark->Kind == ESolidWorldBang::LairExit)
		{
			return ESolidBiome::Mountain;
		}
	}
	const TCHAR Marker = static_cast<TCHAR>(Markers[MapY * MapWidth + MapX]);
	return MarkerToBiome(Marker);
}

bool USolidWorldChart::FindBang(const int32 MapX, const int32 MapY, FSolidWorldBang& Out) const
{
	if (!EnsureQuery() || MapX < 0 || MapY < 0 || MapX >= MapWidth || MapY >= MapHeight)
	{
		return false;
	}
	if (const FSolidWorldBang* Found = BangMarks.Find(MapY * MapWidth + MapX))
	{
		Out = *Found;
		return true;
	}
	return false;
}

bool USolidWorldChart::StrongholdAtCell(
	const int32 MapX,
	const int32 MapY,
	ESolidStrongholdType& OutType,
	ESolidStrongholdSize& OutSize) const
{
	FSolidWorldBang Mark;
	if (!FindBang(MapX, MapY, Mark) || Mark.Kind != ESolidWorldBang::Stronghold)
	{
		return false;
	}
	OutType = Mark.StrongholdType;
	OutSize = Mark.StrongholdSize;
	return true;
}

bool USolidWorldChart::StrongholdSiteAtCell(const int32 MapX, const int32 MapY, ESolidStrongholdSize& OutSize) const
{
	FSolidWorldBang Mark;
	if (!FindBang(MapX, MapY, Mark) || Mark.Kind != ESolidWorldBang::StrongholdSite)
	{
		return false;
	}
	OutSize = Mark.StrongholdSize;
	return true;
}

TCHAR USolidWorldChart::GetMarkerAtCell(int32 MapX, int32 MapY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return 0;
	}
	MapX = FMath::Clamp(MapX, 0, MapWidth - 1);
	MapY = FMath::Clamp(MapY, 0, MapHeight - 1);
	return static_cast<TCHAR>(Markers[MapY * MapWidth + MapX]);
}

bool USolidWorldChart::WorldXYToCell(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	int32& OutMapX,
	int32& OutMapY) const
{
	if (!EnsureQuery())
	{
		return false;
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	// File column 0 is the left side of the text → world west (max X).
	const float UFromWest = FMath::Clamp((WorldMaxXY.X - WorldX) / ExtentX, 0.f, 1.f);
	// File row 0 is north (max Y).
	const float VFromSouth = FMath::Clamp((WorldY - WorldMinXY.Y) / ExtentY, 0.f, 1.f);
	const float VFromNorth = 1.f - VFromSouth;

	OutMapX = FMath::Clamp(FMath::FloorToInt(UFromWest * MapWidth), 0, MapWidth - 1);
	OutMapY = FMath::Clamp(FMath::FloorToInt(VFromNorth * MapHeight), 0, MapHeight - 1);
	return true;
}

namespace
{
	bool WorldMapMarkerInCell(const USolidWorldChart& Map, const int32 MapX, const int32 MapY, TCHAR& OutMarker)
	{
		if (MapX < 0 || MapY < 0 || MapX >= USolidWorldChart::MapWidth || MapY >= USolidWorldChart::MapHeight)
		{
			return false;
		}
		OutMarker = Map.GetMarkerAtCell(MapX, MapY);
		return OutMarker != 0;
	}

	/** File column 0 is west, file row 0 is north. */
	FIntPoint WorldMapRiverSideStep(const SolidRiver::ESide Side)
	{
		switch (Side)
		{
		case SolidRiver::ESide::North: return FIntPoint(0, -1);
		case SolidRiver::ESide::East: return FIntPoint(1, 0);
		case SolidRiver::ESide::South: return FIntPoint(0, 1);
		case SolidRiver::ESide::West: return FIntPoint(-1, 0);
		default: return FIntPoint(0, -1);
		}
	}

	ESolidBiome WorldMapRiverBank(const USolidWorldChart& Map, const int32 MapX, const int32 MapY, const SolidRiver::ESide Preferred)
	{
		const int32 First = static_cast<int32>(Preferred);
		const int32 Order[4] = { First, (First + 1) & 3, (First + 3) & 3, (First + 2) & 3 };
		for (const int32 Index : Order)
		{
			const FIntPoint Step = WorldMapRiverSideStep(static_cast<SolidRiver::ESide>(Index));
			const int32 NextX = MapX + Step.X;
			const int32 NextY = MapY + Step.Y;
			TCHAR Marker = 0;
			if (!WorldMapMarkerInCell(Map, NextX, NextY, Marker))
			{
				continue;
			}
			const ESolidBiome Biome = Map.GetBiomeAtCell(NextX, NextY);
			// The channel is already water. Another river square is the next
			// length of that channel, so a bend's dry corner takes the other bank.
			// Sea is a real neighbor: a mouth uses it, and a land bank stays land.
			if (Biome == ESolidBiome::River)
			{
				continue;
			}
			return Biome;
		}
		return ESolidBiome::Grassland;
	}
}

void USolidWorldChart::BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions) const
{
	ResetRoads();

	for (const SolidRoad::FDefinition& Definition : Definitions)
	{
		BuildRoad(Definition, nullptr);
	}
}

void USolidWorldChart::ResetRoads() const
{
	SolvedRoads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	RoadSpansByCell.SetNum(MapWidth * MapHeight);
}

void USolidWorldChart::BuildRoad(const SolidRoad::FDefinition& Definition, const SolidRoad::FRoad* Road) const
{
	(void)Road;
	if (RoadSpansByCell.Num() != MapWidth * MapHeight)
	{
		RoadSpansByCell.SetNum(MapWidth * MapHeight);
	}

	SolidRoad::FRoute Route;
	Route.Definition = Definition;
	if (!SolidRoad::Solve(*this, Definition, Route.Cells))
	{
		const auto EndName = [](const SolidRoad::FEndpoint& End)
		{
			return End.bStronghold
				? End.StrongholdName
				: SolidTowns::NameFor(End.LocationId);
		};
		const FString From = EndName(Definition.From);
		const FString To = EndName(Definition.To);
		UE_LOG(LogSolid, Warning, TEXT("WorldMap: no route for road %s -> %s."), *From, *To);
	}
	SolvedRoads.Add(Route);

	if (Route.Cells.Num() == 0)
	{
		return;
	}

	const float HalfWidth = SolidRoad::WidthCm(Definition.Type) * 0.5f;
	const int32 SpanCount = Route.Cells.Num() == 1 ? 1 : Route.Cells.Num() - 1;
	for (int32 SpanIndex = 0; SpanIndex < SpanCount; ++SpanIndex)
	{
		SolidRoad::FSpan Span;
		Span.From = Route.Cells[SpanIndex];
		Span.To = Route.Cells[FMath::Min(SpanIndex + 1, Route.Cells.Num() - 1)];
		Span.HalfWidthCm = HalfWidth;
		const int32 Stored = RoadSpans.Add(Span);

		auto Touch = [this, Stored](const FIntPoint& Cell)
		{
			for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
			{
				for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
				{
					const int32 X = Cell.X + OffsetX;
					const int32 Y = Cell.Y + OffsetY;
					if (X < 0 || Y < 0 || X >= MapWidth || Y >= MapHeight)
					{
						continue;
					}
					RoadSpansByCell[Y * MapWidth + X].AddUnique(Stored);
				}
			}
		};
		Touch(Span.From);
		Touch(Span.To);
	}
}

bool USolidWorldChart::IsOnRoad(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!EnsureQuery() || RoadSpans.Num() == 0 || RoadSpansByCell.Num() != MapWidth * MapHeight)
	{
		return false;
	}

	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return false;
	}

	const FVector2D World(WorldX, WorldY);
	for (const int32 SpanIndex : RoadSpansByCell[MapY * MapWidth + MapX])
	{
		const SolidRoad::FSpan& Span = RoadSpans[SpanIndex];
		FVector2D From = FVector2D::ZeroVector;
		FVector2D To = FVector2D::ZeroVector;
		if (!CellToWorldXY(Span.From, WorldMinXY, WorldMaxXY, From)
			|| !CellToWorldXY(Span.To, WorldMinXY, WorldMaxXY, To))
		{
			continue;
		}
		if (SolidRoad::DistanceToSegmentCm(World, From, To) <= Span.HalfWidthCm)
		{
			return true;
		}
	}
	return false;
}

bool USolidWorldChart::IsMarkedTown(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return false;
	}

	const TCHAR Marker = GetMarkerAtCell(MapX, MapY);
	if (Marker == LocationMarker)
	{
		return GetLocationIdAtCell(MapX, MapY) != INDEX_NONE;
	}
	return Marker == TownMarker;
}

namespace
{
	constexpr int32 PointCount = USolidWorldChart::PointAxis * USolidWorldChart::PointAxis;

	int32 PointSlot(const int32 IX, const int32 IY)
	{
		return IY * USolidWorldChart::PointAxis + IX;
	}

	void PointAtFraction(const float EastFromWest, const float NorthFromSouth, int32& OutX, int32& OutY)
	{
		const float Span = static_cast<float>(USolidWorldChart::PointAxis - 1);
		OutX = FMath::Clamp(FMath::RoundToInt(EastFromWest * Span), 0, USolidWorldChart::PointAxis - 1);
		OutY = FMath::Clamp(FMath::RoundToInt(NorthFromSouth * Span), 0, USolidWorldChart::PointAxis - 1);
	}

	void PointAtWorld(
		const float WorldX,
		const float WorldY,
		const FVector2D& Min,
		const FVector2D& Max,
		int32& OutX,
		int32& OutY)
	{
		const float ExtentX = FMath::Max(Max.X - Min.X, 1.f);
		const float ExtentY = FMath::Max(Max.Y - Min.Y, 1.f);
		PointAtFraction((WorldX - Min.X) / ExtentX, (WorldY - Min.Y) / ExtentY, OutX, OutY);
	}

	FVector2D WorldAtPoint(const int32 IX, const int32 IY, const FVector2D& Min, const FVector2D& Max)
	{
		const float Span = static_cast<float>(USolidWorldChart::PointAxis - 1);
		return FVector2D(
			Min.X + static_cast<float>(IX) * (Max.X - Min.X) / Span,
			Min.Y + static_cast<float>(IY) * (Max.Y - Min.Y) / Span);
	}

	/** The terrain point at a cell centre. Column 0 is west, row 0 is north. */
	void PointUnderCell(const int32 Col, const int32 Row, int32& OutX, int32& OutY)
	{
		PointAtFraction(
			1.f - (static_cast<float>(Col) + 0.5f) / static_cast<float>(USolidWorldChart::MapWidth),
			1.f - (static_cast<float>(Row) + 0.5f) / static_cast<float>(USolidWorldChart::MapHeight),
			OutX,
			OutY);
	}

	bool OnPoint(const FIntPoint& Point)
	{
		return Point.X >= 0 && Point.Y >= 0
			&& Point.X < USolidWorldChart::PointAxis
			&& Point.Y < USolidWorldChart::PointAxis;
	}

	void AppendPoint(TArray<FIntPoint>& Points, TSet<int32>& Seen, const int32 X, const int32 Y)
	{
		if (X < 0 || Y < 0 || X >= USolidWorldChart::PointAxis || Y >= USolidWorldChart::PointAxis)
		{
			return;
		}
		const int32 Slot = PointSlot(X, Y);
		if (Seen.Contains(Slot))
		{
			return;
		}
		Seen.Add(Slot);
		Points.Add(FIntPoint(X, Y));
	}

	void AppendDisc(TArray<FIntPoint>& Points, TSet<int32>& Seen, const int32 CentreX, const int32 CentreY, const int32 Radius)
	{
		const int32 Reach = FMath::Clamp(Radius, 0, USolidWorldChart::PointAxis);
		const int32 ReachSq = Reach * Reach;
		for (int32 Y = CentreY - Reach; Y <= CentreY + Reach; ++Y)
		{
			for (int32 X = CentreX - Reach; X <= CentreX + Reach; ++X)
			{
				const int32 DX = X - CentreX;
				const int32 DY = Y - CentreY;
				if (DX * DX + DY * DY <= ReachSq)
				{
					AppendPoint(Points, Seen, X, Y);
				}
			}
		}
	}

	/** Straight run of brush stamps from one cursor point to the next. */
	void AppendRun(TArray<FIntPoint>& Points, TSet<int32>& Seen, const FIntPoint& From, const FIntPoint& To, const int32 Radius)
	{
		int32 X = From.X;
		int32 Y = From.Y;
		const int32 DX = FMath::Abs(To.X - X);
		const int32 DY = FMath::Abs(To.Y - Y);
		const int32 StepX = X < To.X ? 1 : -1;
		const int32 StepY = Y < To.Y ? 1 : -1;
		int32 Error = DX - DY;
		while (true)
		{
			AppendDisc(Points, Seen, X, Y, Radius);
			if (X == To.X && Y == To.Y)
			{
				break;
			}
			const int32 Twice = Error * 2;
			if (Twice > -DY)
			{
				Error -= DY;
				X += StepX;
			}
			if (Twice < DX)
			{
				Error += DX;
				Y += StepY;
			}
		}
	}
}

ESolidBiome USolidWorldChart::SampleCellPlay(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return ESolidBiome::Grassland;
	}

	TCHAR Marker = 0;
	if (!WorldMapMarkerInCell(*this, MapX, MapY, Marker) || Marker != RiverMarker)
	{
		return GetBiomeAtCell(MapX, MapY);
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(MapWidth);
	const float CellH = ExtentY / static_cast<float>(MapHeight);

	FVector2D Center = FVector2D::ZeroVector;
	if (!CellToWorldXY(FIntPoint(MapX, MapY), WorldMinXY, WorldMaxXY, Center))
	{
		return ESolidBiome::River;
	}

	const FVector2D Local(WorldX - Center.X, WorldY - Center.Y);
	SolidRiver::FLinks Links;
	TCHAR Neighbor = 0;
	Links.bNorth = WorldMapMarkerInCell(*this, MapX, MapY - 1, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bEast = WorldMapMarkerInCell(*this, MapX + 1, MapY, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bSouth = WorldMapMarkerInCell(*this, MapX, MapY + 1, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bWest = WorldMapMarkerInCell(*this, MapX - 1, MapY, Neighbor) && SolidRiver::Joins(Neighbor);
	if (SolidRiver::IsWater(Local, CellW, CellH, Links))
	{
		return ESolidBiome::River;
	}
	return WorldMapRiverBank(*this, MapX, MapY, SolidRiver::Side(Local, CellW, CellH, Links));
}

void USolidWorldChart::ApplyMarkerPoint(const FVector2D& PositionCm, const ESolidWorldChartMarkKind Kind) const
{
	if (Points.Num() != PointCount)
	{
		return;
	}

	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	DefaultWorldBounds(Min, Max);
	int32 IX = 0;
	int32 IY = 0;
	PointAtWorld(PositionCm.X, PositionCm.Y, Min, Max, IX, IY);
	Points[PointSlot(IX, IY)] = Kind == ESolidWorldChartMarkKind::Town
		? ESolidBiome::Town
		: ESolidBiome::Grassland;
}

void USolidWorldChart::ReleaseMarkerPoint(const FVector2D& PositionCm) const
{
	if (Points.Num() != PointCount)
	{
		return;
	}

	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	DefaultWorldBounds(Min, Max);
	int32 IX = 0;
	int32 IY = 0;
	PointAtWorld(PositionCm.X, PositionCm.Y, Min, Max, IX, IY);
	const FVector2D Here = WorldAtPoint(IX, IY, Min, Max);
	int32 Col = 0;
	int32 Row = 0;
	if (!WorldXYToCell(Here.X, Here.Y, Min, Max, Col, Row))
	{
		return;
	}

	const FIntPoint Steps[] = {
		FIntPoint(1, 0),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(0, -1),
		FIntPoint(2, 0),
		FIntPoint(-2, 0),
		FIntPoint(0, 2),
		FIntPoint(0, -2),
	};
	for (const FIntPoint& Step : Steps)
	{
		const int32 NextX = IX + Step.X;
		const int32 NextY = IY + Step.Y;
		if (!OnPoint(FIntPoint(NextX, NextY)))
		{
			continue;
		}
		const FVector2D World = WorldAtPoint(NextX, NextY, Min, Max);
		int32 NextCol = 0;
		int32 NextRow = 0;
		if (!WorldXYToCell(World.X, World.Y, Min, Max, NextCol, NextRow))
		{
			continue;
		}
		if (NextCol != Col || NextRow != Row)
		{
			continue;
		}
		Points[PointSlot(IX, IY)] = Points[PointSlot(NextX, NextY)];
		return;
	}
}

void USolidWorldChart::CopyPlayOntoPoints(const bool bForce, const bool bUseStoredMarks) const
{
	if (!bForce && Points.Num() == PointCount)
	{
		return;
	}

	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	DefaultWorldBounds(Min, Max);
	Points.SetNum(PointCount);
	for (int32 IY = 0; IY < PointAxis; ++IY)
	{
		for (int32 IX = 0; IX < PointAxis; ++IX)
		{
			const FVector2D World = WorldAtPoint(IX, IY, Min, Max);
			Points[PointSlot(IX, IY)] = SampleCellPlay(World.X, World.Y, Min, Max);
		}
	}

	if (bUseStoredMarks)
	{
		for (const FSolidWorldChartMark& Mark : Marks)
		{
			ApplyMarkerPoint(Mark.PositionCm, Mark.Kind);
		}
		return;
	}

	for (const TPair<int32, FSolidWorldBang>& Pair : BangMarks)
	{
		int32 IX = 0;
		int32 IY = 0;
		PointUnderCell(Pair.Value.Cell.X, Pair.Value.Cell.Y, IX, IY);
		Points[PointSlot(IX, IY)] = Pair.Value.Kind == ESolidWorldBang::Town
			? ESolidBiome::Town
			: ESolidBiome::Grassland;
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
	for (const FTownSeed& Seed : TownSeeds)
	{
		int32 IX = 0;
		int32 IY = 0;
		PointUnderCell(Seed.Column, Seed.Row, IX, IY);
		const int32 Start = PointSlot(IX, IY);
		if (Points[Start] != ESolidBiome::Town || PointTowns[Start] != INDEX_NONE)
		{
			continue;
		}

		PointTowns[Start] = Seed.LocationId;
		Queue.Reset();
		Queue.Add(Start);
		for (int32 Index = 0; Index < Queue.Num(); ++Index)
		{
			const int32 Slot = Queue[Index];
			const int32 X = Slot % PointAxis;
			const int32 Y = Slot / PointAxis;
			const FIntPoint Steps[] = {
				FIntPoint(1, 0),
				FIntPoint(-1, 0),
				FIntPoint(0, 1),
				FIntPoint(0, -1),
			};
			for (const FIntPoint& Step : Steps)
			{
				const int32 NextX = X + Step.X;
				const int32 NextY = Y + Step.Y;
				if (NextX < 0 || NextY < 0 || NextX >= PointAxis || NextY >= PointAxis)
				{
					continue;
				}
				const int32 Next = PointSlot(NextX, NextY);
				if (Points[Next] != ESolidBiome::Town || PointTowns[Next] != INDEX_NONE)
				{
					continue;
				}
				PointTowns[Next] = Seed.LocationId;
				Queue.Add(Next);
			}
		}
	}
}

bool USolidWorldChart::EnsurePoints() const
{
	if (Points.Num() == PointCount)
	{
		return true;
	}
	if (!EnsureQuery())
	{
		return false;
	}
	if (Points.Num() == PointCount)
	{
		return true;
	}
	CopyPlayOntoPoints(true, Marks.Num() > 0);
	return Points.Num() == PointCount;
}

void USolidWorldChart::SetPointsInCells(const TArray<int32>& CellIndexes, const ESolidBiome Biome) const
{
	if (Points.Num() != PointCount || CellIndexes.Num() == 0)
	{
		return;
	}

	TSet<int32> Want;
	Want.Reserve(CellIndexes.Num());
	for (const int32 Index : CellIndexes)
	{
		Want.Add(Index);
	}

	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	DefaultWorldBounds(Min, Max);
	for (int32 IY = 0; IY < PointAxis; ++IY)
	{
		for (int32 IX = 0; IX < PointAxis; ++IX)
		{
			const FVector2D World = WorldAtPoint(IX, IY, Min, Max);
			int32 Col = 0;
			int32 Row = 0;
			if (!WorldXYToCell(World.X, World.Y, Min, Max, Col, Row))
			{
				continue;
			}
			if (!Want.Contains(Row * MapWidth + Col))
			{
				continue;
			}
			Points[PointSlot(IX, IY)] = Biome;
		}
	}
}

bool USolidWorldChart::TerrainPointAt(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FIntPoint& OutPoint) const
{
	int32 IX = 0;
	int32 IY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, IX, IY);
	OutPoint = FIntPoint(IX, IY);
	return true;
}

bool USolidWorldChart::TerrainPointToWorld(
	const FIntPoint Point,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (!OnPoint(Point))
	{
		return false;
	}
	OutWorldXY = WorldAtPoint(Point.X, Point.Y, WorldMinXY, WorldMaxXY);
	return true;
}

ESolidBiome USolidWorldChart::GetPointBiome(const FIntPoint Point) const
{
	if (!EnsurePoints() || !OnPoint(Point))
	{
		return ESolidBiome::Grassland;
	}
	return Points[PointSlot(Point.X, Point.Y)];
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
	for (const FIntPoint& Centre : Centres)
	{
		if (!OnPoint(Previous) || !OnPoint(Centre))
		{
			Previous = Centre;
			continue;
		}
		AppendRun(OutPoints, Seen, Previous, Centre, Radius);
		Previous = Centre;
	}
}

void USolidWorldChart::PointRectangle(const FIntPoint From, const FIntPoint To, TArray<FIntPoint>& OutPoints)
{
	OutPoints.Reset();
	const int32 X0 = FMath::Min(From.X, To.X);
	const int32 X1 = FMath::Max(From.X, To.X);
	const int32 Y0 = FMath::Min(From.Y, To.Y);
	const int32 Y1 = FMath::Max(From.Y, To.Y);
	if (X0 < 0 || Y0 < 0 || X1 >= PointAxis || Y1 >= PointAxis)
	{
		return;
	}

	OutPoints.Reserve((X1 - X0 + 1) * (Y1 - Y0 + 1));
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			OutPoints.Add(FIntPoint(X, Y));
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
	if (Cells.Num() != MapWidth * MapHeight)
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}
	if (Covered.Num() == 0)
	{
		OutError = TEXT("Paint needs a point.");
		return false;
	}
	for (const FIntPoint& Point : Covered)
	{
		if (!OnPoint(Point))
		{
			OutError = TEXT("That point is off the map.");
			return false;
		}
	}
	if (!EnsurePoints())
	{
		OutError = TEXT("The chart does not cover the map.");
		return false;
	}

	PushUndo();
	TSet<int32> Painted;
	Painted.Reserve(Covered.Num());
	for (const FIntPoint& Point : Covered)
	{
		const int32 Slot = PointSlot(Point.X, Point.Y);
		Points[Slot] = Biome;
		Painted.Add(Slot);
	}

	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	DefaultWorldBounds(Min, Max);
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		int32 IX = 0;
		int32 IY = 0;
		PointAtWorld(Mark.PositionCm.X, Mark.PositionCm.Y, Min, Max, IX, IY);
		if (Painted.Contains(PointSlot(IX, IY)))
		{
			ApplyMarkerPoint(Mark.PositionCm, Mark.Kind);
		}
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
	for (const FIntPoint& Centre : Centres)
	{
		if (!OnPoint(Centre))
		{
			OutError = TEXT("That point is off the map.");
			return false;
		}
	}

	TArray<FIntPoint> Covered;
	BrushPoints(Centres, Radius, Covered);
	return PaintPoints(Covered, Biome, OutError);
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

ESolidBiome USolidWorldChart::GetPointBiomeAtCell(const int32 MapX, const int32 MapY) const
{
	if (!EnsurePoints())
	{
		return GetBiomeAtCell(MapX, MapY);
	}

	const int32 X = FMath::Clamp(MapX, 0, MapWidth - 1);
	const int32 Y = FMath::Clamp(MapY, 0, MapHeight - 1);
	int32 IX = 0;
	int32 IY = 0;
	PointUnderCell(X, Y, IX, IY);
	return Points[PointSlot(IX, IY)];
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

	int32 IX = 0;
	int32 IY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, IX, IY);
	const ESolidBiome Ground = Points[PointSlot(IX, IY)];

	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return Ground;
	}

	// The road shares the grassland shelf, so this is a texture change there.
	// A stronghold keeps the shelf it names. Mountain and sea keep theirs because
	// those shelves sit at a different height. The channel itself stays water.
	bool bNamedShelf = false;
	ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
	if (StrongholdAtCell(MapX, MapY, HoldType, HoldSize))
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

int32 USolidWorldChart::GetLocationIdAtCell(const int32 MapX, const int32 MapY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return INDEX_NONE;
	}

	const int32 X = FMath::Clamp(MapX, 0, MapWidth - 1);
	const int32 Y = FMath::Clamp(MapY, 0, MapHeight - 1);
	return ResolveLocationId(X, Y);
}

int32 USolidWorldChart::ResolveLocationId(const int32 MapX, const int32 MapY) const
{
	if (Markers.Num() != MapWidth * MapHeight
		|| MapX < 0 || MapY < 0 || MapX >= MapWidth || MapY >= MapHeight)
	{
		return INDEX_NONE;
	}

	if (static_cast<TCHAR>(Markers[MapY * MapWidth + MapX]) != LocationMarker)
	{
		return INDEX_NONE;
	}
	const int32 Index = MapY * MapWidth + MapX;
	return BangLocationIds.IsValidIndex(Index) ? BangLocationIds[Index] : INDEX_NONE;
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

	int32 IX = 0;
	int32 IY = 0;
	PointAtWorld(WorldX, WorldY, WorldMinXY, WorldMaxXY, IX, IY);
	const int32 Slot = PointSlot(IX, IY);
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

bool USolidWorldChart::CellsToWorldXY(
	const TArray<FIntPoint>& InCells,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (InCells.Num() == 0)
	{
		return false;
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(MapWidth);
	const float CellH = ExtentY / static_cast<float>(MapHeight);

	FVector2D Sum = FVector2D::ZeroVector;
	for (const FIntPoint& Cell : InCells)
	{
		// Column 0 = west = max X (matches SampleBiome).
		const float X = WorldMaxXY.X - (static_cast<float>(Cell.X) + 0.5f) * CellW;
		// Row 0 = north = max Y.
		const float Y = WorldMaxXY.Y - (static_cast<float>(Cell.Y) + 0.5f) * CellH;
		Sum += FVector2D(X, Y);
	}
	OutWorldXY = Sum / static_cast<float>(InCells.Num());
	return true;
}

bool USolidWorldChart::GetStartTownWorldXY(
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (!EnsureQuery() || !bHasStartTown || StartTownCells.Num() == 0)
	{
		return false;
	}
	return CellsToWorldXY(StartTownCells, WorldMinXY, WorldMaxXY, OutWorldXY);
}

bool USolidWorldChart::FindLocation(const int32 Id, FSolidWorldLocation& OutLocation) const
{
	EnsureQuery();
	if (const FSolidWorldLocation* Found = Locations.Find(Id))
	{
		OutLocation = *Found;
		return true;
	}
	return false;
}

void USolidWorldChart::CollectLocations(TArray<FSolidWorldLocation>& OutLocations) const
{
	EnsureQuery();
	OutLocations.Reset();
	Locations.GenerateValueArray(OutLocations);
	OutLocations.Sort([](const FSolidWorldLocation& A, const FSolidWorldLocation& B)
	{
		return A.Id < B.Id;
	});
}

bool USolidWorldChart::CellToWorldXY(
	const FIntPoint Cell,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	const TArray<FIntPoint> OneCell = { Cell };
	return CellsToWorldXY(OneCell, WorldMinXY, WorldMaxXY, OutWorldXY);
}

bool USolidWorldChart::CellAtWorld(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FIntPoint& OutCell) const
{
	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return false;
	}
	OutCell = FIntPoint(MapX, MapY);
	return true;
}

bool USolidWorldChart::GetLocationWorldXY(
	const int32 Id,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	EnsureQuery();
	const FSolidWorldLocation* Found = Locations.Find(Id);
	if (!Found)
	{
		return false;
	}
	return CellsToWorldXY(Found->Cells, WorldMinXY, WorldMaxXY, OutWorldXY);
}
