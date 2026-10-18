#include "SolidWorldMap.h"
#include "SolidCore1.h"
#include "SolidLair.h"
#include "SolidRiver.h"
#include "SolidTowns.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	bool WorldMapMarkerMatches(const TCHAR Raw, const TCHAR Expected)
	{
		return Raw == Expected || FChar::ToUpper(Raw) == Expected;
	}

	TCHAR WorldMapCanonicalMarker(const TCHAR Raw)
	{
		const TCHAR Expected[] = {
			USolidWorldMap::GrassMarker,
			USolidWorldMap::TownMarker,
			USolidWorldMap::ForestMarker,
			USolidWorldMap::RiverMarker,
			USolidWorldMap::SeaMarker,
			USolidWorldMap::DesertMarker,
			USolidWorldMap::LocationMarker,
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

bool USolidWorldMap::IsGridMarker(const TCHAR Marker)
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

ESolidBiome USolidWorldMap::MarkerToBiome(const TCHAR Marker)
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

FLinearColor USolidWorldMap::DefaultColorForBiome(const ESolidBiome Biome)
{
	switch (Biome)
	{
	case ESolidBiome::Grassland: return FLinearColor(0.45f, 0.72f, 0.28f);
	case ESolidBiome::Forest: return FLinearColor(0.08f, 0.28f, 0.08f);
	case ESolidBiome::Mountain: return FLinearColor(0.16f, 0.16f, 0.18f);
	case ESolidBiome::Town: return FLinearColor(0.45f, 0.28f, 0.12f);
	case ESolidBiome::Desert: return FLinearColor(0.85f, 0.75f, 0.32f);
	case ESolidBiome::Swamp: return FLinearColor(0.22f, 0.32f, 0.18f);
	case ESolidBiome::Sea: return FLinearColor(0.15f, 0.35f, 0.75f);
	case ESolidBiome::River: return FLinearColor(0.20f, 0.48f, 0.85f);
	case ESolidBiome::Road: return FLinearColor(0.52f, 0.38f, 0.20f);
	default: return FLinearColor(0.4f, 0.4f, 0.4f);
	}
}

FLinearColor USolidWorldMap::ColorFromName(const FString& ColorName)
{
	const FString Name = ColorName.TrimStartAndEnd().ToLower();
	if (Name.Contains(TEXT("light green")) || Name.Equals(TEXT("lightgreen")))
	{
		return FLinearColor(0.45f, 0.72f, 0.28f);
	}
	if (Name.Contains(TEXT("dark green")) || Name.Equals(TEXT("darkgreen")))
	{
		return FLinearColor(0.08f, 0.28f, 0.08f);
	}
	if (Name.Contains(TEXT("green")))
	{
		return FLinearColor(0.20f, 0.55f, 0.18f);
	}
	if (Name.Contains(TEXT("blue")))
	{
		return FLinearColor(0.15f, 0.35f, 0.75f);
	}
	if (Name.Contains(TEXT("brown")))
	{
		return FLinearColor(0.45f, 0.28f, 0.12f);
	}
	if (Name.Contains(TEXT("dark grey")) || Name.Contains(TEXT("dark gray"))
		|| Name.Equals(TEXT("darkgrey")) || Name.Equals(TEXT("darkgray")))
	{
		return FLinearColor(0.16f, 0.16f, 0.18f);
	}
	if (Name.Contains(TEXT("grey")) || Name.Contains(TEXT("gray")))
	{
		return FLinearColor(0.55f, 0.55f, 0.58f);
	}
	if (Name.Contains(TEXT("yellow")))
	{
		return FLinearColor(0.85f, 0.75f, 0.32f);
	}
	return FLinearColor(0.5f, 0.5f, 0.5f);
}

bool USolidWorldMap::ParseKeyLine(const FString& Line)
{
	// "~ = Sea (blue)" or "M = Mountain (dark grey)"
	FString Trimmed = Line.TrimStartAndEnd();
	if (Trimmed.Len() < 3 || Trimmed[1] != TEXT(' ') && Trimmed[1] != TEXT('='))
	{
		// Allow "S=Sea (blue)"
	}

	int32 EqIndex = INDEX_NONE;
	if (!Trimmed.FindChar(TEXT('='), EqIndex) || EqIndex < 1)
	{
		return false;
	}

	const TCHAR Marker = WorldMapCanonicalMarker(Trimmed[0]);
	if (!IsGridMarker(Marker))
	{
		return false;
	}

	// Letters, digits, and ! are not biome keys. A leftover "C = Cave" line must not recolor grass.
	if (Marker == XMarker || Marker == YMarker || Marker == LocationMarker
		|| Marker == TEXT('C') || Marker == TEXT('Z') || FChar::IsDigit(Marker))
	{
		return true;
	}

	FString Right = Trimmed.Mid(EqIndex + 1).TrimStartAndEnd();
	FString ColorName;
	int32 OpenParen = INDEX_NONE;
	int32 CloseParen = INDEX_NONE;
	if (Right.FindChar(TEXT('('), OpenParen) && Right.FindLastChar(TEXT(')'), CloseParen)
		&& CloseParen > OpenParen)
	{
		ColorName = Right.Mid(OpenParen + 1, CloseParen - OpenParen - 1);
	}

	const ESolidBiome Biome = MarkerToBiome(Marker);
	BiomeColors.Add(Biome, ColorName.IsEmpty() ? DefaultColorForBiome(Biome) : ColorFromName(ColorName));
	return true;
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
		if (Trimmed.IsEmpty() || Trimmed[0] != USolidWorldMap::LocationMarker)
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
			|| Row > USolidWorldMap::MapHeight
			|| Column > USolidWorldMap::MapWidth)
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
}

bool USolidWorldMap::LoadFromString(const FString& Text)
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

	Markers.SetNum(MapWidth * MapHeight);
	StartTownCells.Reset();
	Locations.Reset();
	Roads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	bHasStartTown = false;

	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		const FString& RowText = GridLines[Row];
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const TCHAR Marker = WorldMapCanonicalMarker(RowText[Col]);
			Markers[Row * MapWidth + Col] = static_cast<uint8>(Marker);
		}
	}

	BiomeColors.Reset();
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
		else
		{
			ParseKeyLine(RawLine);
		}
	}

	BangMarks.Reset();
	BangLocationIds.Init(INDEX_NONE, MapWidth * MapHeight);
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
			break;
		}
		case FWorldMapBangRef::EKind::Stronghold:
		{
			FSolidStronghold Attributes;
			if (!SolidStronghold::FromToken(Ref.Identifier, Attributes))
			{
				UE_LOG(LogSolid, Warning,
					TEXT("WorldMap: ! %d,%d stronghold '%s' is unknown; ignored."),
					Row, Column, *Ref.Identifier);
				continue;
			}
			Mark.Kind = ESolidWorldBang::Stronghold;
			Mark.StrongholdType = Attributes.Type;
			Mark.StrongholdSize = Attributes.Size;
			Mark.Name = SolidStronghold::Label(Attributes);
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
			break;
		}
		default:
			continue;
		}
		BangMarks.Add(Index, Mark);
	}

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

	// Ensure every biome used by markers has a color.
	static const ESolidBiome AllBiomes[] = {
		ESolidBiome::Grassland, ESolidBiome::Forest, ESolidBiome::Mountain, ESolidBiome::Town,
		ESolidBiome::Desert, ESolidBiome::Swamp, ESolidBiome::Sea, ESolidBiome::River,
		ESolidBiome::Road,
	};
	for (const ESolidBiome Biome : AllBiomes)
	{
		if (!BiomeColors.Contains(Biome))
		{
			BiomeColors.Add(Biome, DefaultColorForBiome(Biome));
		}
	}

	bIsLoaded = true;
	BuildRoads(RoadDefs);
	UE_LOG(LogSolid, Warning,
		TEXT("WorldMap loaded: %dx%d startTownCells=%d locations=%d roads=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), Locations.Num(), Roads.Num());
	return true;
}

bool USolidWorldMap::LoadFromFile(const FString& FilePath)
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *FilePath))
	{
		UE_LOG(LogSolid, Error, TEXT("WorldMap: failed to read %s"), *FilePath);
		return false;
	}
	return LoadFromString(Text);
}

bool USolidWorldMap::LoadDefault(const bool bForceReload)
{
	if (bIsLoaded && !bForceReload)
	{
		return true;
	}

	const FString ContentPath = FPaths::ProjectContentDir() / TEXT("WorldMap.txt");
	if (FPaths::FileExists(ContentPath) && LoadFromFile(ContentPath))
	{
		return true;
	}

	UE_LOG(LogSolid, Error, TEXT("WorldMap: failed to read Content/WorldMap.txt."));
	return false;
}

ESolidBiome USolidWorldMap::GetBiomeAtCell(int32 MapX, int32 MapY) const
{
	if (!bIsLoaded || Markers.Num() != MapWidth * MapHeight)
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
			FSolidStronghold Attributes;
			Attributes.Type = Mark->StrongholdType;
			Attributes.Size = Mark->StrongholdSize;
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

bool USolidWorldMap::FindBang(const int32 MapX, const int32 MapY, FSolidWorldBang& Out) const
{
	if (!bIsLoaded || MapX < 0 || MapY < 0 || MapX >= MapWidth || MapY >= MapHeight)
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

bool USolidWorldMap::StrongholdAtCell(const int32 MapX, const int32 MapY, FSolidStronghold& Out) const
{
	FSolidWorldBang Mark;
	if (!FindBang(MapX, MapY, Mark) || Mark.Kind != ESolidWorldBang::Stronghold)
	{
		return false;
	}
	Out.Type = Mark.StrongholdType;
	Out.Size = Mark.StrongholdSize;
	return true;
}

bool USolidWorldMap::StrongholdSiteAtCell(const int32 MapX, const int32 MapY, ESolidStrongholdSize& OutSize) const
{
	FSolidWorldBang Mark;
	if (!FindBang(MapX, MapY, Mark) || Mark.Kind != ESolidWorldBang::StrongholdSite)
	{
		return false;
	}
	OutSize = Mark.StrongholdSize;
	return true;
}

TCHAR USolidWorldMap::GetMarkerAtCell(int32 MapX, int32 MapY) const
{
	if (!bIsLoaded || Markers.Num() != MapWidth * MapHeight)
	{
		return 0;
	}
	MapX = FMath::Clamp(MapX, 0, MapWidth - 1);
	MapY = FMath::Clamp(MapY, 0, MapHeight - 1);
	return static_cast<TCHAR>(Markers[MapY * MapWidth + MapX]);
}

bool USolidWorldMap::WorldXYToCell(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	int32& OutMapX,
	int32& OutMapY) const
{
	if (!bIsLoaded)
	{
		return false;
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	// File column 0 is the left side of the text → world east (max X).
	const float UFromEast = FMath::Clamp((WorldMaxXY.X - WorldX) / ExtentX, 0.f, 1.f);
	// File row 0 is north (max Y).
	const float VFromSouth = FMath::Clamp((WorldY - WorldMinXY.Y) / ExtentY, 0.f, 1.f);
	const float VFromNorth = 1.f - VFromSouth;

	OutMapX = FMath::Clamp(FMath::FloorToInt(UFromEast * MapWidth), 0, MapWidth - 1);
	OutMapY = FMath::Clamp(FMath::FloorToInt(VFromNorth * MapHeight), 0, MapHeight - 1);
	return true;
}

namespace
{
	bool WorldMapMarkerInCell(const USolidWorldMap& Map, const int32 MapX, const int32 MapY, TCHAR& OutMarker)
	{
		if (MapX < 0 || MapY < 0 || MapX >= USolidWorldMap::MapWidth || MapY >= USolidWorldMap::MapHeight)
		{
			return false;
		}
		OutMarker = Map.GetMarkerAtCell(MapX, MapY);
		return OutMarker != 0;
	}

	/** File column 0 is east, file row 0 is north. */
	FIntPoint WorldMapRiverSideStep(const SolidRiver::ESide Side)
	{
		switch (Side)
		{
		case SolidRiver::ESide::North: return FIntPoint(0, -1);
		case SolidRiver::ESide::East: return FIntPoint(-1, 0);
		case SolidRiver::ESide::South: return FIntPoint(0, 1);
		case SolidRiver::ESide::West: return FIntPoint(1, 0);
		default: return FIntPoint(0, -1);
		}
	}

	ESolidBiome WorldMapRiverBank(const USolidWorldMap& Map, const int32 MapX, const int32 MapY, const SolidRiver::ESide Preferred)
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

void USolidWorldMap::BuildRoads(const TArray<SolidRoad::FDefinition>& Definitions)
{
	Roads.Reset();
	RoadSpans.Reset();
	RoadSpansByCell.Reset();
	RoadSpansByCell.SetNum(MapWidth * MapHeight);

	for (const SolidRoad::FDefinition& Definition : Definitions)
	{
		SolidRoad::FRoute Route;
		Route.Definition = Definition;
		if (!SolidRoad::Solve(*this, Definition, Route.Cells))
		{
			const auto EndName = [](const SolidRoad::FEndpoint& End)
			{
				return End.bStronghold
					? SolidStronghold::Label(End.Stronghold)
					: SolidTowns::NameFor(End.LocationId);
			};
			const FString From = EndName(Definition.From);
			const FString To = EndName(Definition.To);
			UE_LOG(LogSolid, Warning, TEXT("WorldMap: no route for road %s -> %s."), *From, *To);
		}
		Roads.Add(Route);

		if (Route.Cells.Num() == 0)
		{
			continue;
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
}

bool USolidWorldMap::IsOnRoad(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (RoadSpans.Num() == 0 || RoadSpansByCell.Num() != MapWidth * MapHeight)
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

bool USolidWorldMap::IsMarkedTown(
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

ESolidBiome USolidWorldMap::SampleBiome(
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

	ESolidBiome Ground = ESolidBiome::Grassland;
	TCHAR Marker = 0;
	if (!WorldMapMarkerInCell(*this, MapX, MapY, Marker) || Marker != RiverMarker)
	{
		Ground = GetBiomeAtCell(MapX, MapY);
	}
	else
	{
		const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
		const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
		const float CellW = ExtentX / static_cast<float>(MapWidth);
		const float CellH = ExtentY / static_cast<float>(MapHeight);

		FVector2D Center = FVector2D::ZeroVector;
		if (!CellToWorldXY(FIntPoint(MapX, MapY), WorldMinXY, WorldMaxXY, Center))
		{
			Ground = ESolidBiome::River;
		}
		else
		{
			const FVector2D Local(WorldX - Center.X, WorldY - Center.Y);

			SolidRiver::FLinks Links;
			TCHAR Neighbor = 0;
			Links.bNorth = WorldMapMarkerInCell(*this, MapX, MapY - 1, Neighbor) && SolidRiver::Joins(Neighbor);
			Links.bEast = WorldMapMarkerInCell(*this, MapX - 1, MapY, Neighbor) && SolidRiver::Joins(Neighbor);
			Links.bSouth = WorldMapMarkerInCell(*this, MapX, MapY + 1, Neighbor) && SolidRiver::Joins(Neighbor);
			Links.bWest = WorldMapMarkerInCell(*this, MapX + 1, MapY, Neighbor) && SolidRiver::Joins(Neighbor);

			if (SolidRiver::IsWater(Local, CellW, CellH, Links))
			{
				Ground = ESolidBiome::River;
			}
			else
			{
				Ground = WorldMapRiverBank(*this, MapX, MapY, SolidRiver::Side(Local, CellW, CellH, Links));
			}
		}
	}

	// The road shares the grassland shelf, so this is a texture change there.
	// Mountain and sea keep their shelves. The channel itself stays water.
	if (Ground != ESolidBiome::River
		&& IsOnRoad(WorldX, WorldY, WorldMinXY, WorldMaxXY)
		&& SolidTerrainTypes::BiomeHeightOffsetCm(Ground)
			== SolidTerrainTypes::BiomeHeightOffsetCm(ESolidBiome::Road))
	{
		return ESolidBiome::Road;
	}
	return Ground;
}

int32 USolidWorldMap::GetLocationIdAtCell(const int32 MapX, const int32 MapY) const
{
	if (!bIsLoaded || Markers.Num() != MapWidth * MapHeight)
	{
		return INDEX_NONE;
	}

	const int32 X = FMath::Clamp(MapX, 0, MapWidth - 1);
	const int32 Y = FMath::Clamp(MapY, 0, MapHeight - 1);
	return ResolveLocationId(X, Y);
}

int32 USolidWorldMap::ResolveLocationId(const int32 MapX, const int32 MapY) const
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

int32 USolidWorldMap::SampleLocationId(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	int32 MapX = 0;
	int32 MapY = 0;
	if (!WorldXYToCell(WorldX, WorldY, WorldMinXY, WorldMaxXY, MapX, MapY))
	{
		return INDEX_NONE;
	}
	return GetLocationIdAtCell(MapX, MapY);
}

FLinearColor USolidWorldMap::GetBiomeColor(const ESolidBiome Biome) const
{
	if (const FLinearColor* Found = BiomeColors.Find(Biome))
	{
		return *Found;
	}
	return DefaultColorForBiome(Biome);
}

bool USolidWorldMap::CellsToWorldXY(
	const TArray<FIntPoint>& Cells,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (Cells.Num() == 0)
	{
		return false;
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	const float CellW = ExtentX / static_cast<float>(MapWidth);
	const float CellH = ExtentY / static_cast<float>(MapHeight);

	FVector2D Sum = FVector2D::ZeroVector;
	for (const FIntPoint& Cell : Cells)
	{
		// Column 0 = east = max X (matches SampleBiome).
		const float X = WorldMaxXY.X - (static_cast<float>(Cell.X) + 0.5f) * CellW;
		// Row 0 = north = max Y.
		const float Y = WorldMaxXY.Y - (static_cast<float>(Cell.Y) + 0.5f) * CellH;
		Sum += FVector2D(X, Y);
	}
	OutWorldXY = Sum / static_cast<float>(Cells.Num());
	return true;
}

bool USolidWorldMap::GetStartTownWorldXY(
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	if (!bHasStartTown || StartTownCells.Num() == 0)
	{
		return false;
	}
	return CellsToWorldXY(StartTownCells, WorldMinXY, WorldMaxXY, OutWorldXY);
}

bool USolidWorldMap::FindLocation(const int32 Id, FSolidWorldLocation& OutLocation) const
{
	if (const FSolidWorldLocation* Found = Locations.Find(Id))
	{
		OutLocation = *Found;
		return true;
	}
	return false;
}

void USolidWorldMap::CollectLocations(TArray<FSolidWorldLocation>& OutLocations) const
{
	OutLocations.Reset();
	Locations.GenerateValueArray(OutLocations);
	OutLocations.Sort([](const FSolidWorldLocation& A, const FSolidWorldLocation& B)
	{
		return A.Id < B.Id;
	});
}

bool USolidWorldMap::CellToWorldXY(
	const FIntPoint Cell,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	const TArray<FIntPoint> Cells = { Cell };
	return CellsToWorldXY(Cells, WorldMinXY, WorldMaxXY, OutWorldXY);
}

bool USolidWorldMap::CellAtWorld(
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

bool USolidWorldMap::GetLocationWorldXY(
	const int32 Id,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	FVector2D& OutWorldXY) const
{
	const FSolidWorldLocation* Found = Locations.Find(Id);
	if (!Found)
	{
		return false;
	}
	return CellsToWorldXY(Found->Cells, WorldMinXY, WorldMaxXY, OutWorldXY);
}
