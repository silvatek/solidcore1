#include "SolidWorldMap.h"
#include "SolidCore1.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

bool USolidWorldMap::IsGridMarker(const TCHAR Marker)
{
	const TCHAR Upper = FChar::ToUpper(Marker);
	return (Upper >= TEXT('A') && Upper <= TEXT('Z')) || FChar::IsDigit(Marker);
}

ESolidBiome USolidWorldMap::MarkerToBiome(const TCHAR Marker)
{
	switch (FChar::ToUpper(Marker))
	{
	case TEXT('G'): return ESolidBiome::Grassland;
	case TEXT('F'): return ESolidBiome::Forest;
	case TEXT('M'): return ESolidBiome::Mountain;
	case TEXT('T'): return ESolidBiome::Town;
	case TEXT('Z'): return ESolidBiome::Town;
	case TEXT('D'): return ESolidBiome::Desert;
	case TEXT('S'): return ESolidBiome::Sea;
	case TEXT('R'): return ESolidBiome::River;
	default:
		// Numbered locations resolve through the location key in GetBiomeAtCell.
		return FChar::IsDigit(Marker) ? ESolidBiome::Town : ESolidBiome::Grassland;
	}
}

bool USolidWorldMap::BiomeFromName(const FString& Name, ESolidBiome& OutBiome)
{
	const FString Trimmed = Name.TrimStartAndEnd();
	static const ESolidBiome AllBiomes[] = {
		ESolidBiome::Grassland, ESolidBiome::Forest, ESolidBiome::Mountain, ESolidBiome::Town,
		ESolidBiome::Desert, ESolidBiome::Swamp, ESolidBiome::Sea, ESolidBiome::River,
	};
	for (const ESolidBiome Biome : AllBiomes)
	{
		if (Trimmed.Equals(SolidTerrainTypes::BiomeToString(Biome), ESearchCase::IgnoreCase))
		{
			OutBiome = Biome;
			return true;
		}
	}
	return false;
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
	// "S = Sea (blue)" or "Z = Starting town (brown)"
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

	const TCHAR Marker = FChar::ToUpper(Trimmed[0]);
	if (Marker < TEXT('A') || Marker > TEXT('Z'))
	{
		return false;
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

bool USolidWorldMap::ParseLocationLine(const FString& Line)
{
	// "0 = Starting village, biome=Town, name=Iglin"
	const FString Trimmed = Line.TrimStartAndEnd();
	int32 EqIndex = INDEX_NONE;
	if (!Trimmed.FindChar(TEXT('='), EqIndex) || EqIndex < 1)
	{
		return false;
	}

	const FString Left = Trimmed.Left(EqIndex).TrimStartAndEnd();
	if (Left.Len() != 1 || !FChar::IsDigit(Left[0]))
	{
		return false;
	}

	const int32 Id = static_cast<int32>(Left[0] - TEXT('0'));
	FSolidWorldLocation& Loc = Locations.FindOrAdd(Id);
	Loc.Id = Id;
	Loc.Biome = ESolidBiome::Town;

	const FString Right = Trimmed.Mid(EqIndex + 1).TrimStartAndEnd();
	TArray<FString> Parts;
	Right.ParseIntoArray(Parts, TEXT(","), /*CullEmpty=*/true);
	for (int32 PartIndex = 0; PartIndex < Parts.Num(); ++PartIndex)
	{
		const FString Part = Parts[PartIndex].TrimStartAndEnd();
		FString Key;
		FString Value;
		if (Part.Split(TEXT("="), &Key, &Value))
		{
			Key = Key.TrimStartAndEnd();
			Value = Value.TrimStartAndEnd();
			if (Key.Equals(TEXT("biome"), ESearchCase::IgnoreCase))
			{
				ESolidBiome ParsedBiome = ESolidBiome::Town;
				if (BiomeFromName(Value, ParsedBiome))
				{
					Loc.Biome = ParsedBiome;
				}
				else
				{
					UE_LOG(LogSolid, Warning,
						TEXT("WorldMap: location %d has unknown biome '%s'; using Town."),
						Id, *Value);
					Loc.Biome = ESolidBiome::Town;
				}
			}
			else if (Key.Equals(TEXT("name"), ESearchCase::IgnoreCase))
			{
				Loc.Name = Value;
			}
		}
		else if (PartIndex == 0)
		{
			Loc.Role = Part;
		}
	}
	return true;
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
	bHasStartTown = false;

	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		const FString& RowText = GridLines[Row];
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const TCHAR Raw = RowText[Col];
			const TCHAR Marker = FChar::IsDigit(Raw) ? Raw : FChar::ToUpper(Raw);
			Markers[Row * MapWidth + Col] = static_cast<uint8>(Marker);
		}
	}

	BiomeColors.Reset();
	for (const FString& RawLine : Lines)
	{
		if (!ParseLocationLine(RawLine))
		{
			ParseKeyLine(RawLine);
		}
	}

	TArray<FIntPoint> LegacyStartCells;
	bool bHasLocationZero = false;
	for (int32 Row = 0; Row < MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < MapWidth; ++Col)
		{
			const TCHAR Marker = static_cast<TCHAR>(Markers[Row * MapWidth + Col]);
			if (FChar::IsDigit(Marker))
			{
				const int32 Id = static_cast<int32>(Marker - TEXT('0'));
				FSolidWorldLocation& Loc = Locations.FindOrAdd(Id);
				if (Loc.Id == INDEX_NONE)
				{
					Loc.Id = Id;
					Loc.Biome = ESolidBiome::Town;
				}
				Loc.Cells.Add(FIntPoint(Col, Row));
				if (Id == 0)
				{
					bHasLocationZero = true;
				}
			}
			else if (Marker == TEXT('Z'))
			{
				LegacyStartCells.Add(FIntPoint(Col, Row));
			}
		}
	}

	if (bHasLocationZero)
	{
		if (const FSolidWorldLocation* Start = Locations.Find(0))
		{
			StartTownCells = Start->Cells;
		}
	}
	else
	{
		StartTownCells = MoveTemp(LegacyStartCells);
	}
	bHasStartTown = StartTownCells.Num() > 0;

	// Ensure every biome used by markers has a color.
	static const ESolidBiome AllBiomes[] = {
		ESolidBiome::Grassland, ESolidBiome::Forest, ESolidBiome::Mountain, ESolidBiome::Town,
		ESolidBiome::Desert, ESolidBiome::Swamp, ESolidBiome::Sea, ESolidBiome::River,
	};
	for (const ESolidBiome Biome : AllBiomes)
	{
		if (!BiomeColors.Contains(Biome))
		{
			BiomeColors.Add(Biome, DefaultColorForBiome(Biome));
		}
	}

	bIsLoaded = true;
	UE_LOG(LogSolid, Warning,
		TEXT("WorldMap loaded: %dx%d startTownCells=%d locations=%d"),
		MapWidth, MapHeight, StartTownCells.Num(), Locations.Num());
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

	const FString SourcePath =
		FPaths::ProjectDir() / TEXT("Source/SolidCore1/WorldMap.txt");
	if (FPaths::FileExists(SourcePath) && LoadFromFile(SourcePath))
	{
		return true;
	}

	UE_LOG(LogSolid, Error, TEXT("WorldMap: no WorldMap.txt under Content/ or Source/SolidCore1/."));
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
	const TCHAR Marker = static_cast<TCHAR>(Markers[MapY * MapWidth + MapX]);
	if (FChar::IsDigit(Marker))
	{
		const int32 Id = static_cast<int32>(Marker - TEXT('0'));
		if (const FSolidWorldLocation* Loc = Locations.Find(Id))
		{
			return Loc->Biome;
		}
		return ESolidBiome::Town;
	}
	return MarkerToBiome(Marker);
}

ESolidBiome USolidWorldMap::SampleBiome(
	const float WorldX,
	const float WorldY,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY) const
{
	if (!bIsLoaded)
	{
		return ESolidBiome::Grassland;
	}

	const float ExtentX = FMath::Max(WorldMaxXY.X - WorldMinXY.X, 1.f);
	const float ExtentY = FMath::Max(WorldMaxXY.Y - WorldMinXY.Y, 1.f);
	// File column 0 is the left side of the text → world east (max X).
	const float UFromEast = FMath::Clamp((WorldMaxXY.X - WorldX) / ExtentX, 0.f, 1.f);
	// File row 0 is north (max Y).
	const float VFromSouth = FMath::Clamp((WorldY - WorldMinXY.Y) / ExtentY, 0.f, 1.f);
	const float VFromNorth = 1.f - VFromSouth;

	const int32 MapX = FMath::Clamp(FMath::FloorToInt(UFromEast * MapWidth), 0, MapWidth - 1);
	const int32 MapY = FMath::Clamp(FMath::FloorToInt(VFromNorth * MapHeight), 0, MapHeight - 1);
	return GetBiomeAtCell(MapX, MapY);
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
