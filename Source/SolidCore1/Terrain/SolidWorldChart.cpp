#include "SolidWorldChart.h"
#include "SolidBridges.h"
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
	InvalidateQuery();
	return true;
}

bool USolidWorldChart::PaintRectangle(const FIntPoint FromCell, const FIntPoint ToCell, const ESolidBiome Biome, FString& OutError)
{
	const int32 CellX0 = FMath::Min(FromCell.X, ToCell.X);
	const int32 CellX1 = FMath::Max(FromCell.X, ToCell.X);
	const int32 CellY0 = FMath::Min(FromCell.Y, ToCell.Y);
	const int32 CellY1 = FMath::Max(FromCell.Y, ToCell.Y);
	TArray<FIntPoint> Covered;
	Covered.Reserve((CellX1 - CellX0 + 1) * (CellY1 - CellY0 + 1));
	for (int32 CellY = CellY0; CellY <= CellY1; ++CellY)
	{
		for (int32 CellX = CellX0; CellX <= CellX1; ++CellX)
		{
			Covered.Add(FIntPoint(CellX, CellY));
		}
	}
	return PaintRun(Covered, Biome, OutError);
}

bool USolidWorldChart::PlaceMark(const FIntPoint Cell, const FSolidWorldChartMark& Mark, FString& OutError)
{
	OutError.Reset();
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= MapWidth || Cell.Y >= MapHeight
		|| Cells.Num() != USolidWorldChart::MapWidth * USolidWorldChart::MapHeight)
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
	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointUnderCell(Cell.X, Cell.Y, TerrainX, TerrainY);
	Stored.TerrainPoint = FIntPoint(TerrainX, TerrainY);
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

bool USolidWorldChart::PlaceBridge(const FIntPoint Cell, const FString& Name, FString& OutError)
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

int32 USolidWorldChart::MarkAtCell(const FIntPoint Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= MapWidth || Cell.Y >= MapHeight)
	{
		return INDEX_NONE;
	}
	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointUnderCell(Cell.X, Cell.Y, TerrainX, TerrainY);
	const FIntPoint TerrainPoint(TerrainX, TerrainY);
	for (int32 Index = 0; Index < Marks.Num(); ++Index)
	{
		if (Marks[Index].TerrainPoint == TerrainPoint)
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
			SolidBiome::Of(ESolidBiome::Grassland).Marker(),
			SolidBiome::Of(ESolidBiome::Town).Marker(),
			SolidBiome::Of(ESolidBiome::Forest).Marker(),
			SolidBiome::Of(ESolidBiome::River).Marker(),
			SolidBiome::Of(ESolidBiome::Sea).Marker(),
			SolidBiome::Of(ESolidBiome::Desert).Marker(),
			USolidWorldChart::DeprecatedLocationMarker,
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
	return WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Grassland).Marker())
		|| WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Town).Marker())
		|| WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Forest).Marker())
		|| WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::River).Marker())
		|| WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Sea).Marker())
		|| WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Desert).Marker())
		|| WorldMapMarkerMatches(Marker, DeprecatedLocationMarker);
}

ESolidBiome USolidWorldChart::MarkerToBiome(const TCHAR Marker)
{
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Sea).Marker()))
	{
		return ESolidBiome::Sea;
	}
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Grassland).Marker()))
	{
		return ESolidBiome::Grassland;
	}
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Forest).Marker()))
	{
		return ESolidBiome::Forest;
	}
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Town).Marker()))
	{
		return ESolidBiome::Town;
	}
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::River).Marker()))
	{
		return ESolidBiome::River;
	}
	if (WorldMapMarkerMatches(Marker, SolidBiome::Of(ESolidBiome::Desert).Marker()))
	{
		return ESolidBiome::Desert;
	}
	if (WorldMapMarkerMatches(Marker, DeprecatedLocationMarker))
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

		int32 CellX = INDEX_NONE;
		int32 CellY = INDEX_NONE;
		EKind Kind = EKind::Skip;
		FString Identifier;
	};

	/**
	 * "! 8,21 = Town Iglin" — row 8, column 21, counting from 1 at the top-left.
	 * "! 9,12 = Stronghold MeagreDungeon", "! 12,12 = StrongholdSite (Humble)",
	 * "! 10,11 = LairEntrance Iglin tunnels", "! 11,40 = LairExit Iglin tunnels".
	 * Anything after # is a comment. A numeric right-hand side is ignored.
	 * Returns false when the line is not that shape.
	 * An out-of-range coordinate is that shape: CellX stays INDEX_NONE and the line is consumed.
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
		if (Trimmed.IsEmpty() || Trimmed[0] != USolidWorldChart::DeprecatedLocationMarker)
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

		Out.CellX = Column - 1;
		Out.CellY = Row - 1;

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

	for (int32 CellY = 0; CellY < MapHeight; ++CellY)
	{
		const FString& RowText = GridLines[CellY];
		for (int32 CellX = 0; CellX < MapWidth; ++CellX)
		{
			const TCHAR Marker = WorldMapCanonicalMarker(RowText[CellX]);
			Markers[CellY * MapWidth + CellX] = static_cast<uint8>(Marker);
		}
	}

	TArray<FWorldMapBangRef> BangRefs;
	for (const FString& RawLine : Lines)
	{
		const FString Trimmed = RawLine.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("Road:"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		FWorldMapBangRef BangRef;
		if (WorldMapParseBangLocation(Trimmed, BangRef))
		{
			BangRefs.Add(BangRef);
		}
	}

	for (const FWorldMapBangRef& Ref : BangRefs)
	{
		if (Ref.CellX == INDEX_NONE || Ref.CellY == INDEX_NONE || Ref.Kind != FWorldMapBangRef::EKind::Town)
		{
			continue;
		}
		const int32 Index = Ref.CellY * MapWidth + Ref.CellX;
		const int32 Row = Ref.CellY + 1;
		const int32 Column = Ref.CellX + 1;
		if (static_cast<TCHAR>(Markers[Index]) != DeprecatedLocationMarker)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d is not an exclamation mark; ignored."),
				Row, Column);
			continue;
		}
		if (BangLocationIds[Index] != INDEX_NONE)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d is already described; keeping it."),
				Row, Column);
			continue;
		}

		const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Ref.Identifier);
		if (!Town)
		{
			UE_LOG(LogSolid, Warning,
				TEXT("WorldMap: ! %d,%d town '%s' is unknown; ignored."),
				Row, Column, *Ref.Identifier);
			continue;
		}
		BangLocationIds[Index] = Town->Id;
		FTownSeed Seed;
		Seed.LocationId = Town->Id;
		Seed.CellX = Ref.CellX;
		Seed.CellY = Ref.CellY;
		TownSeeds.Add(Seed);
	}

	CollectNamedTowns();

	bIsLoaded = true;
	CopyPlayOntoPoints(true);
	AssignTownPoints();
	UE_LOG(LogSolid, Warning,
		TEXT("WorldMap loaded: %dx%d startTownCells=%d roads=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), SolvedRoads.Num());
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
	SolvedRoads.Reset();
	RoutedRoads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	bHasStartTown = false;
	BangLocationIds.Init(INDEX_NONE, MapWidth * MapHeight);
	EncounterMarks.Reset();
	TownSeeds.Reset();
	PointTowns.Reset();
	bIsLoaded = false;
}

void USolidWorldChart::CollectNamedTowns() const
{
	for (int32 CellY = 0; CellY < MapHeight; ++CellY)
	{
		for (int32 CellX = 0; CellX < MapWidth; ++CellX)
		{
			if (ResolveLocationId(CellX, CellY) == 0)
			{
				StartTownCells.Add(FIntPoint(CellX, CellY));
			}
		}
	}
	bHasStartTown = StartTownCells.Num() > 0;
}

namespace
{
	/** Swamp and road are not marker biomes, so a chart cell of either loads as grassland. */
	TCHAR MarkerForChartBiome(const ESolidBiome Biome)
	{
		const FSolidBiome& Ground = SolidBiome::Of(Biome);
		return Ground.Paintable() ? Ground.Marker() : SolidBiome::Of(ESolidBiome::Grassland).Marker();
	}
}

bool USolidWorldChart::RememberChartMark(const FSolidWorldChartMark& Mark, const FIntPoint Cell) const
{
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= MapWidth || Cell.Y >= MapHeight)
	{
		return false;
	}
	const int32 Index = Cell.Y * MapWidth + Cell.X;
	if (static_cast<TCHAR>(Markers[Index]) == DeprecatedLocationMarker)
	{
		return false;
	}

	switch (Mark.Kind)
	{
	case ESolidWorldChartMarkKind::Town:
	{
		const SolidTowns::FTownInfo* Town = SolidTowns::FindByName(Mark.Name);
		if (!Town)
		{
			return false;
		}
		BangLocationIds[Index] = Town->Id;
		FTownSeed Seed;
		Seed.LocationId = Town->Id;
		Seed.CellX = Cell.X;
		Seed.CellY = Cell.Y;
		TownSeeds.Add(Seed);
		break;
	}
	case ESolidWorldChartMarkKind::Stronghold:
	case ESolidWorldChartMarkKind::StrongholdSite:
		break;
	case ESolidWorldChartMarkKind::LairEntrance:
	case ESolidWorldChartMarkKind::LairExit:
		if (!SolidLair::FindByName(Mark.Name))
		{
			return false;
		}
		break;
	case ESolidWorldChartMarkKind::Custom:
	case ESolidWorldChartMarkKind::Encounter:
	case ESolidWorldChartMarkKind::Bridge:
		return false;
	default:
		return false;
	}

	// A town id and the text file both use !. The chart stored that cell as grassland.
	Markers[Index] = static_cast<uint8>(DeprecatedLocationMarker);
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

	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind == ESolidWorldChartMarkKind::Encounter)
		{
			FSolidWorldEncounterMark Found;
			Found.Name = Mark.Name;
			Found.TerrainPoint = Mark.TerrainPoint;
			EncounterMarks.Add(Found);
		}
		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!CellAtTerrainPoint(Mark.TerrainPoint, Cell))
		{
			continue;
		}
		RememberChartMark(Mark, Cell);
	}

	CollectNamedTowns();
	CopyPlayOntoPoints(false);
	AssignTownPoints();

	BuildListedRoads();

	bIsLoaded = true;
	UE_LOG(LogSolid, Warning,
		TEXT("WorldChart loaded: %dx%d startTownCells=%d roads=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), SolvedRoads.Num());
	return true;
}

ESolidBiome USolidWorldChart::GetMarkerBiomeAtCell(int32 CellX, int32 CellY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return ESolidBiome::Grassland;
	}
	CellX = FMath::Clamp(CellX, 0, MapWidth - 1);
	CellY = FMath::Clamp(CellY, 0, MapHeight - 1);
	return MarkerToBiome(static_cast<TCHAR>(Markers[CellY * MapWidth + CellX]));
}

ESolidBiome USolidWorldChart::GetBiomeAtCell(int32 CellX, int32 CellY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return ESolidBiome::Grassland;
	}
	CellX = FMath::Clamp(CellX, 0, MapWidth - 1);
	CellY = FMath::Clamp(CellY, 0, MapHeight - 1);
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		FIntPoint Cell;
		if (!CellAtTerrainPoint(Mark.TerrainPoint, Cell) || Cell.X != CellX || Cell.Y != CellY)
		{
			continue;
		}
		if (Mark.Kind == ESolidWorldChartMarkKind::Town)
		{
			if (SolidTowns::FindByName(Mark.Name))
			{
				return ESolidBiome::Town;
			}
		}
		else if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold)
		{
			ESolidStrongholdType HoldType = Mark.StrongholdType;
			ESolidStrongholdSize HoldSize = Mark.StrongholdSize;
			SolidStronghold::FromToken(Mark.Name, HoldType, HoldSize);
			ESolidBiome Shelf = ESolidBiome::Grassland;
			if (SolidStronghold::BiomeFor(FSolidStronghold(HoldType, HoldSize), Shelf))
			{
				return Shelf;
			}
		}
		else if (Mark.Kind == ESolidWorldChartMarkKind::LairEntrance
			|| Mark.Kind == ESolidWorldChartMarkKind::LairExit)
		{
			const int32 Index = CellY * MapWidth + CellX;
			if (Cells.IsValidIndex(Index))
			{
				return Cells[Index];
			}
		}
		break;
	}
	const TCHAR Marker = static_cast<TCHAR>(Markers[CellY * MapWidth + CellX]);
	return MarkerToBiome(Marker);
}

bool USolidWorldChart::StrongholdAtCell(
	const int32 CellX,
	const int32 CellY,
	ESolidStrongholdType& OutType,
	ESolidStrongholdSize& OutSize) const
{
	if (!EnsureQuery() || CellX < 0 || CellY < 0 || CellX >= MapWidth || CellY >= MapHeight)
	{
		return false;
	}
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		FIntPoint Cell;
		if (!CellAtTerrainPoint(Mark.TerrainPoint, Cell) || Cell.X != CellX || Cell.Y != CellY)
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

bool USolidWorldChart::StrongholdSiteAtCell(const int32 CellX, const int32 CellY, ESolidStrongholdSize& OutSize) const
{
	if (!EnsureQuery() || CellX < 0 || CellY < 0 || CellX >= MapWidth || CellY >= MapHeight)
	{
		return false;
	}
	for (const FSolidWorldChartMark& Mark : Marks)
	{
		if (Mark.Kind != ESolidWorldChartMarkKind::StrongholdSite)
		{
			continue;
		}
		FIntPoint Cell;
		if (!CellAtTerrainPoint(Mark.TerrainPoint, Cell) || Cell.X != CellX || Cell.Y != CellY)
		{
			continue;
		}
		OutSize = Mark.StrongholdSize;
		return true;
	}
	return false;
}

TCHAR USolidWorldChart::GetMarkerAtCell(int32 CellX, int32 CellY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return 0;
	}
	CellX = FMath::Clamp(CellX, 0, MapWidth - 1);
	CellY = FMath::Clamp(CellY, 0, MapHeight - 1);
	return static_cast<TCHAR>(Markers[CellY * MapWidth + CellX]);
}

bool USolidWorldChart::WorldXYToCell(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	int32& OutCellX,
	int32& OutCellY) const
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

	OutCellX = FMath::Clamp(FMath::FloorToInt(UFromWest * MapWidth), 0, MapWidth - 1);
	OutCellY = FMath::Clamp(FMath::FloorToInt(VFromNorth * MapHeight), 0, MapHeight - 1);
	return true;
}

void USolidWorldChart::PointUnderCell(const int32 CellX, const int32 CellY, int32& OutTerrainX, int32& OutTerrainY)
{
	const float Span = static_cast<float>(PointAxis - 1);
	OutTerrainX = FMath::Clamp(
		FMath::RoundToInt((1.f - (static_cast<float>(CellX) + 0.5f) / static_cast<float>(MapWidth)) * Span),
		0,
		PointAxis - 1);
	OutTerrainY = FMath::Clamp(
		FMath::RoundToInt((1.f - (static_cast<float>(CellY) + 0.5f) / static_cast<float>(MapHeight)) * Span),
		0,
		PointAxis - 1);
}

bool USolidWorldChart::CellAtTerrainPoint(const FIntPoint TerrainPoint, FIntPoint& OutCell)
{
	if (TerrainPoint.X < 0 || TerrainPoint.Y < 0
		|| TerrainPoint.X >= PointAxis || TerrainPoint.Y >= PointAxis)
	{
		return false;
	}
	const float Span = static_cast<float>(PointAxis - 1);
	const float UFromWest = 1.f - static_cast<float>(TerrainPoint.X) / Span;
	const float VFromNorth = 1.f - static_cast<float>(TerrainPoint.Y) / Span;
	OutCell.X = FMath::Clamp(FMath::FloorToInt(UFromWest * static_cast<float>(MapWidth)), 0, MapWidth - 1);
	OutCell.Y = FMath::Clamp(FMath::FloorToInt(VFromNorth * static_cast<float>(MapHeight)), 0, MapHeight - 1);
	return true;
}

namespace
{
	bool WorldMapMarkerInCell(const USolidWorldChart& Map, const int32 CellX, const int32 CellY, TCHAR& OutMarker)
	{
		if (CellX < 0 || CellY < 0 || CellX >= USolidWorldChart::MapWidth || CellY >= USolidWorldChart::MapHeight)
		{
			return false;
		}
		OutMarker = Map.GetMarkerAtCell(CellX, CellY);
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

	ESolidBiome WorldMapRiverBank(const USolidWorldChart& Map, const int32 CellX, const int32 CellY, const SolidRiver::ESide Preferred)
	{
		const int32 First = static_cast<int32>(Preferred);
		const int32 Order[4] = { First, (First + 1) & 3, (First + 3) & 3, (First + 2) & 3 };
		for (const int32 Index : Order)
		{
			const FIntPoint Step = WorldMapRiverSideStep(static_cast<SolidRiver::ESide>(Index));
			const int32 NextX = CellX + Step.X;
			const int32 NextY = CellY + Step.Y;
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

bool USolidWorldChart::IsMarkedTown(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	int32 CellX = 0;
	int32 CellY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, CellX, CellY))
	{
		return false;
	}

	const TCHAR Marker = GetMarkerAtCell(CellX, CellY);
	if (Marker == DeprecatedLocationMarker)
	{
		return GetLocationIdAtCell(CellX, CellY) != INDEX_NONE;
	}
	return Marker == SolidBiome::Of(ESolidBiome::Town).Marker();
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

ESolidBiome USolidWorldChart::SampleCellPlay(const int32 TerrainX, const int32 TerrainY) const
{
	FIntPoint Cell = FIntPoint::ZeroValue;
	if (!CellAtTerrainPoint(FIntPoint(TerrainX, TerrainY), Cell))
	{
		return ESolidBiome::Grassland;
	}
	const int32 CellX = Cell.X;
	const int32 CellY = Cell.Y;

	TCHAR Marker = 0;
	if (!WorldMapMarkerInCell(*this, CellX, CellY, Marker) || Marker != SolidBiome::Of(ESolidBiome::River).Marker())
	{
		return GetBiomeAtCell(CellX, CellY);
	}

	int32 CentreX = 0;
	int32 CentreY = 0;
	PointUnderCell(CellX, CellY, CentreX, CentreY);
	const USolidTerrainMap* Terrain = GetDefault<USolidTerrainMap>();
	const float Spacing = Terrain ? Terrain->PointSpacing : 200.f;
	const float CellW = Spacing * static_cast<float>(PointAxis - 1) / static_cast<float>(MapWidth);
	const float CellH = Spacing * static_cast<float>(PointAxis - 1) / static_cast<float>(MapHeight);
	const FVector2D Local(
		static_cast<float>(TerrainX - CentreX) * Spacing,
		static_cast<float>(TerrainY - CentreY) * Spacing);
	SolidRiver::FLinks Links;
	TCHAR Neighbor = 0;
	Links.bNorth = WorldMapMarkerInCell(*this, CellX, CellY - 1, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bEast = WorldMapMarkerInCell(*this, CellX + 1, CellY, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bSouth = WorldMapMarkerInCell(*this, CellX, CellY + 1, Neighbor) && SolidRiver::Joins(Neighbor);
	Links.bWest = WorldMapMarkerInCell(*this, CellX - 1, CellY, Neighbor) && SolidRiver::Joins(Neighbor);
	if (SolidRiver::IsWater(Local, CellW, CellH, Links))
	{
		return ESolidBiome::River;
	}
	return WorldMapRiverBank(*this, CellX, CellY, SolidRiver::Side(Local, CellW, CellH, Links));
}

void USolidWorldChart::CopyPlayOntoPoints(const bool bForce) const
{
	if (!bForce && Points.Num() == PointCount)
	{
		return;
	}

	Points.SetNum(PointCount);
	for (int32 TerrainY = 0; TerrainY < PointAxis; ++TerrainY)
	{
		for (int32 TerrainX = 0; TerrainX < PointAxis; ++TerrainX)
		{
			Points[PointSlot(TerrainX, TerrainY)] = SampleCellPlay(TerrainX, TerrainY);
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
	for (const FTownSeed& Seed : TownSeeds)
	{
		int32 Start = 0;
		{
			int32 TerrainX = 0;
			int32 TerrainY = 0;
			PointUnderCell(Seed.CellX, Seed.CellY, TerrainX, TerrainY);
			Start = PointSlot(TerrainX, TerrainY);
		}
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
			const int32 TerrainX = Slot % PointAxis;
			const int32 TerrainY = Slot / PointAxis;
			const FIntPoint Steps[] = {
				FIntPoint(1, 0),
				FIntPoint(-1, 0),
				FIntPoint(0, 1),
				FIntPoint(0, -1),
			};
			for (const FIntPoint& Step : Steps)
			{
				const int32 NextTerrainX = TerrainX + Step.X;
				const int32 NextTerrainY = TerrainY + Step.Y;
				if (NextTerrainX < 0 || NextTerrainY < 0 || NextTerrainX >= PointAxis || NextTerrainY >= PointAxis)
				{
					continue;
				}
				const int32 Next = PointSlot(NextTerrainX, NextTerrainY);
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
	CopyPlayOntoPoints(true);
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

	for (int32 TerrainY = 0; TerrainY < PointAxis; ++TerrainY)
	{
		for (int32 TerrainX = 0; TerrainX < PointAxis; ++TerrainX)
		{
			FIntPoint Cell = FIntPoint::ZeroValue;
			if (!CellAtTerrainPoint(FIntPoint(TerrainX, TerrainY), Cell))
			{
				continue;
			}
			if (!Want.Contains(Cell.Y * MapWidth + Cell.X))
			{
				continue;
			}
			Points[PointSlot(TerrainX, TerrainY)] = Biome;
		}
	}
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
	for (const FIntPoint& TerrainPoint : Covered)
	{
		if (!OnPoint(TerrainPoint))
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

ESolidBiome USolidWorldChart::GetPointBiomeAtCell(int32 CellX, int32 CellY) const
{
	if (!EnsurePoints())
	{
		return GetBiomeAtCell(CellX, CellY);
	}

	CellX = FMath::Clamp(CellX, 0, MapWidth - 1);
	CellY = FMath::Clamp(CellY, 0, MapHeight - 1);
	int32 TerrainX = 0;
	int32 TerrainY = 0;
	PointUnderCell(CellX, CellY, TerrainX, TerrainY);
	return Points[PointSlot(TerrainX, TerrainY)];
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

	int32 CellX = 0;
	int32 CellY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, CellX, CellY))
	{
		return Ground;
	}

	// The road shares the grassland shelf, so this is a texture change there.
	// A stronghold keeps the shelf it names. Mountain and sea keep theirs because
	// those shelves sit at a different height. The channel itself stays water.
	bool bNamedShelf = false;
	ESolidStrongholdType HoldType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize HoldSize = ESolidStrongholdSize::Meagre;
	if (StrongholdAtCell(CellX, CellY, HoldType, HoldSize))
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

int32 USolidWorldChart::GetLocationIdAtCell(int32 CellX, int32 CellY) const
{
	if (!EnsureQuery() || Markers.Num() != MapWidth * MapHeight)
	{
		return INDEX_NONE;
	}

	CellX = FMath::Clamp(CellX, 0, MapWidth - 1);
	CellY = FMath::Clamp(CellY, 0, MapHeight - 1);
	return ResolveLocationId(CellX, CellY);
}

int32 USolidWorldChart::ResolveLocationId(const int32 CellX, const int32 CellY) const
{
	if (Markers.Num() != MapWidth * MapHeight
		|| CellX < 0 || CellY < 0 || CellX >= MapWidth || CellY >= MapHeight)
	{
		return INDEX_NONE;
	}

	if (static_cast<TCHAR>(Markers[CellY * MapWidth + CellX]) != DeprecatedLocationMarker)
	{
		return INDEX_NONE;
	}
	const int32 Index = CellY * MapWidth + CellX;
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

bool USolidWorldChart::TownCell(const int32 TownId, FIntPoint& OutCell) const
{
	FSolidWorldChartMark Mark;
	if (!FindTownMarker(TownId, Mark))
	{
		return false;
	}
	return CellAtTerrainPoint(Mark.TerrainPoint, OutCell);
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
	int32 CellX = 0;
	int32 CellY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, CellX, CellY))
	{
		return false;
	}
	OutCell = FIntPoint(CellX, CellY);
	return true;
}

bool USolidWorldChart::GetLocationWorldXY(
	const int32 Id,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	FIntPoint Cell;
	if (!TownCell(Id, Cell))
	{
		return false;
	}
	return CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, OutWorldXY);
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

FString USolidWorldChart::RoadEndError(const FString& From, const FString& To) const
{
	TArray<FString> Sentences;
	const SolidRoad::EChartRoadEnd FromKind = SolidRoad::RoadEndKind(Marks, From);
	const SolidRoad::EChartRoadEnd ToKind = SolidRoad::RoadEndKind(Marks, To);
	if (FromKind == SolidRoad::EChartRoadEnd::Site || FromKind == SolidRoad::EChartRoadEnd::Missing)
	{
		Sentences.Add(SolidRoad::RoadEndSentence(FromKind, From));
	}
	if (ToKind == SolidRoad::EChartRoadEnd::Site || ToKind == SolidRoad::EChartRoadEnd::Missing)
	{
		Sentences.Add(SolidRoad::RoadEndSentence(ToKind, To));
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
	SolvedRoads.Reset();
	RoutedRoads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
}

namespace
{
	bool CellsForPoints(
		const USolidWorldChart& Map,
		const TArray<FIntPoint>& Points,
		TArray<FIntPoint>& OutCells)
	{
		OutCells.Reset();
		for (const FIntPoint& TerrainPoint : Points)
		{
			FIntPoint Cell = FIntPoint::ZeroValue;
			if (!USolidWorldChart::CellAtTerrainPoint(TerrainPoint, Cell))
			{
				OutCells.Reset();
				return false;
			}
			const ESolidBiome CellBiome = Map.GetBiomeAtCell(Cell.X, Cell.Y);
			if (CellBiome == ESolidBiome::Mountain || CellBiome == ESolidBiome::Sea)
			{
				continue;
			}
			if (OutCells.Num() == 0 || OutCells.Last() != Cell)
			{
				OutCells.Add(Cell);
			}
		}
		return true;
	}
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

	SolidRoad::FRoute Route;
	if (Road->FindRoute(*this))
	{
		Road->CopyPoints(Route.Points);
		if (!CellsForPoints(*this, Route.Points, Route.Cells) || Route.Cells.Num() == 0)
		{
			Route.Cells.Reset();
			Route.Points.Reset();
		}
	}
	if (Route.Cells.Num() == 0)
	{
		UE_LOG(LogSolid, Warning, TEXT("WorldMap: no route for road %s -> %s."), *Road->From.Name, *Road->To.Name);
	}

	SolidRoad::FDefinition Definition = Road->ToDefinition();
	if (Road->From.Kind == ESolidWorldChartMarkKind::Stronghold)
	{
		SolidRoad::LocateStrongholdEnd(*this, Definition.From);
	}
	if (Road->To.Kind == ESolidWorldChartMarkKind::Stronghold)
	{
		SolidRoad::LocateStrongholdEnd(*this, Definition.To);
	}
	Route.Definition = Definition;
	SolvedRoads.Add(Route);
	if (Road->Points.Num() > 1)
	{
		RoutedRoads.Add(*Road);
	}
}

bool USolidWorldChart::IsOnRoad(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!EnsureQuery())
	{
		return false;
	}

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
