#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRiver.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidTowns.h"
#include "SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapLoadDefaultTest,
	"SolidCore1.WorldMap.LoadDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapLoadDefaultTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadDefault succeeds"), Map->LoadDefault());
	TestTrue(TEXT("reports loaded"), Map->IsLoaded());
	TestTrue(TEXT("has starting town Z cells"), Map->HasStartTown());
	TestTrue(TEXT("start town cell count > 0"), Map->GetStartTownCellCount() > 0);

	// Row 0 is all sea (north edge of the file).
	TestEqual(
		TEXT("north-west cell is sea"),
		static_cast<uint8>(Map->GetBiomeAtCell(0, 0)),
		static_cast<uint8>(ESolidBiome::Sea));

	// Find a Z cell and confirm it maps to Town.
	bool bFoundZTown = false;
	for (int32 Y = 0; Y < USolidWorldMap::MapHeight && !bFoundZTown; ++Y)
	{
		for (int32 X = 0; X < USolidWorldMap::MapWidth; ++X)
		{
			if (Map->GetBiomeAtCell(X, Y) == ESolidBiome::Town)
			{
				// Z and T both become Town; just confirm Town exists.
				bFoundZTown = true;
				break;
			}
		}
	}
	TestTrue(TEXT("map contains Town biome cells"), bFoundZTown);

	const FLinearColor SeaColor = Map->GetBiomeColor(ESolidBiome::Sea);
	TestTrue(TEXT("sea color is bluish"), SeaColor.B > SeaColor.R && SeaColor.B > SeaColor.G);

	const FLinearColor GrassColor = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("grass color is greenish"), GrassColor.G > GrassColor.R && GrassColor.G > GrassColor.B);

	const FLinearColor MountainColor = Map->GetBiomeColor(ESolidBiome::Mountain);
	TestTrue(TEXT("mountain color is dark grey"),
		MountainColor.R < 0.28f && MountainColor.G < 0.28f && MountainColor.B < 0.28f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapParseKeyAndSampleTest,
	"SolidCore1.WorldMap.ParseKeyAndSample",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapParseKeyAndSampleTest::RunTest(const FString& Parameters)
{
	// Synthetic map: grass, with one Z in the middle of row 0.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 0 && Col == USolidWorldMap::MapWidth / 2)
			{
				Line.AppendChar(TEXT('Z'));
			}
			else if (Row == 10 && Col == 10)
			{
				Line.AppendChar(TEXT('D'));
			}
			else
			{
				Line.AppendChar(TEXT('G'));
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("\nG = Grassland (light green)\n");
	Text += TEXT("D = Desert (yellow)\n");
	Text += TEXT("Z = Starting town (brown)\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadFromString"), Map->LoadFromString(Text));
	TestEqual(TEXT("one Z cell"), Map->GetStartTownCellCount(), 1);
	TestEqual(
		TEXT("Z is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(USolidWorldMap::MapWidth / 2, 0)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("desert marker"),
		static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
		static_cast<uint8>(ESolidBiome::Desert));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	// Row 0 = north = max Y → Z at the middle column samples near top-center.
	const ESolidBiome NorthCenter = Map->SampleBiome(0.f, 6300.f, WorldMin, WorldMax);
	TestEqual(
		TEXT("north-center samples Town"),
		static_cast<uint8>(NorthCenter),
		static_cast<uint8>(ESolidBiome::Town));

	// File column 0 is east (max X): desert at col 10 → positive X.
	const float DesertWorldX = WorldMax.X - (10.5f) * ((WorldMax.X - WorldMin.X) / static_cast<float>(USolidWorldMap::MapWidth));
	const float DesertWorldY = WorldMax.Y - (10.5f) * ((WorldMax.Y - WorldMin.Y) / static_cast<float>(USolidWorldMap::MapHeight));
	TestTrue(TEXT("desert cell is on the east side"), DesertWorldX > 0.f);
	TestEqual(
		TEXT("east-side sample is Desert"),
		static_cast<uint8>(Map->SampleBiome(DesertWorldX, DesertWorldY, WorldMin, WorldMax)),
		static_cast<uint8>(ESolidBiome::Desert));

	FVector2D TownXY = FVector2D::ZeroVector;
	TestTrue(TEXT("start town world XY"), Map->GetStartTownWorldXY(WorldMin, WorldMax, TownXY));
	TestTrue(TEXT("town near north"), TownXY.Y > 4000.f);
	TestTrue(TEXT("town near center X"), FMath::Abs(TownXY.X) < 500.f);

	const FLinearColor Desert = Map->GetBiomeColor(ESolidBiome::Desert);
	TestTrue(TEXT("desert is yellowish"), Desert.R > 0.5f && Desert.G > 0.5f && Desert.B < 0.5f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapTerrainOverlayTest,
	"SolidCore1.WorldMap.TerrainOverlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapTerrainOverlayTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Terrain = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("terrain map"), Terrain);
	TestNotNull(TEXT("world map attached"), Terrain->GetWorldMap());
	TestTrue(TEXT("world map loaded"), Terrain->GetWorldMap()->IsLoaded());

	FVector2D TownXY = FVector2D::ZeroVector;
	TestTrue(TEXT("terrain exposes start town"), Terrain->GetStartTownWorldXY(TownXY));

	const FSolidTerrainPoint TownPoint = Terrain->SamplePoint(TownXY.X, TownXY.Y);
	TestEqual(
		TEXT("town centroid is Town biome"),
		static_cast<uint8>(TownPoint.Biome),
		static_cast<uint8>(ESolidBiome::Town));

	// MakeSmallMap centers fog on a fixture player at the origin, not on the Z cell.
	const FVector2D FogOrigin = Terrain->GetFogOriginXY();
	TestTrue(TEXT("fixture fog origin is the player at world origin"),
		FogOrigin.Equals(FVector2D::ZeroVector, 1.f));
	TestTrue(TEXT("Z town is not the fog center"),
		FVector2D::Distance(FogOrigin, TownXY) > SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters));
	TestTrue(TEXT("Z town starts fully fogged"),
		FMath::IsNearlyEqual(Terrain->GetNearestPoint(TownXY.X, TownXY.Y).Fog, 1.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapFogFollowsPlayerTest,
	"SolidCore1.WorldMap.FogFollowsPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapFogFollowsPlayerTest::RunTest(const FString& Parameters)
{
	USolidTerrainMap* Map = NewObject<USolidTerrainMap>();
	TestNotNull(TEXT("map"), Map);
	Map->Build(
		/*InSeed=*/1337,
		/*FrequencyScale=*/0.00012f,
		/*Amplitude=*/3000.f,
		/*BaseHeight=*/0.f,
		/*InGridWidth=*/65,
		/*InGridHeight=*/65,
		/*InPointSpacing=*/200.f,
		/*bForceRebuild=*/true);

	FVector2D StartXY = FVector2D::ZeroVector;
	TestTrue(TEXT("start town exists"), Map->GetStartTownWorldXY(StartXY));
	TestTrue(TEXT("build leaves the Z town fogged"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(StartXY.X, StartXY.Y).Fog, 1.f));
	TestTrue(TEXT("build leaves the world origin fogged"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(0.f, 0.f).Fog, 1.f));

	const FVector2D Player(1200.f, -800.f);
	Map->CenterExplorationFogOn(Player.X, Player.Y);
	TestTrue(TEXT("fog origin is the player"),
		Map->GetFogOriginXY().Equals(Player, 1.f));
	TestTrue(TEXT("player cell is clear"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(Player.X, Player.Y).Fog, 0.f));
	TestTrue(TEXT("Z town stays fogged when the player is elsewhere"),
		FVector2D::Distance(StartXY, Player) > SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters)
		&& FMath::IsNearlyEqual(Map->GetNearestPoint(StartXY.X, StartXY.Y).Fog, 1.f));

	// Standing on the start town is what clears it. The marker itself is not the fog key.
	Map->CenterExplorationFogOn(StartXY.X, StartXY.Y);
	TestTrue(TEXT("fog origin follows the player onto the Z town"),
		Map->GetFogOriginXY().Equals(StartXY, 1.f));
	TestTrue(TEXT("Z town is clear only while the player is there"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(StartXY.X, StartXY.Y).Fog, 0.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapDefaultColorPublicTest,
	"SolidCore1.WorldMap.DefaultColorForBiome",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapDefaultColorPublicTest::RunTest(const FString& Parameters)
{
	const FLinearColor Sea = USolidWorldMap::DefaultColorForBiome(ESolidBiome::Sea);
	const FLinearColor River = USolidWorldMap::DefaultColorForBiome(ESolidBiome::River);
	const FLinearColor Mountain = USolidWorldMap::DefaultColorForBiome(ESolidBiome::Mountain);
	TestTrue(TEXT("default sea is blue"), Sea.B > 0.5f);
	TestTrue(TEXT("default river is blue"), River.B > 0.5f);
	TestTrue(TEXT("default mountain is dark"),
		Mountain.R < 0.28f && Mountain.G < 0.28f && Mountain.B < 0.28f);
	TestTrue(TEXT("default mountain is grey (channels close)"),
		FMath::Abs(Mountain.R - Mountain.G) < 0.05f
		&& FMath::Abs(Mountain.G - Mountain.B) < 0.05f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapNumberedLocationsTest,
	"SolidCore1.WorldMap.NumberedLocations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapNumberedLocationsTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadDefault"), Map->LoadDefault());
	TestTrue(TEXT("the shipped towns are on the map"), Map->GetLocationCount() >= 6);

	FSolidWorldLocation Iglin;
	TestTrue(TEXT("location 0"), Map->FindLocation(0, Iglin));
	TestEqual(TEXT("Iglin"), Iglin.Name, FString(TEXT("Iglin")));
	TestTrue(TEXT("role is not in the map file"), Iglin.Role.IsEmpty());
	TestEqual(TEXT("Iglin is Town"), static_cast<uint8>(Iglin.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestTrue(TEXT("Iglin has a cell"), Iglin.Cells.Num() >= 1);

	FSolidWorldLocation Relion;
	TestTrue(TEXT("location 1"), Map->FindLocation(1, Relion));
	TestEqual(TEXT("Relion"), Relion.Name, FString(TEXT("Relion")));
	TestTrue(TEXT("Relion role is not in the map file"), Relion.Role.IsEmpty());
	TestEqual(TEXT("one Relion cell"), Relion.Cells.Num(), 1);

	FSolidWorldLocation Kanfold;
	TestTrue(TEXT("location 2"), Map->FindLocation(2, Kanfold));
	TestEqual(TEXT("Kanfold"), Kanfold.Name, FString(TEXT("Kanfold")));
	TestTrue(TEXT("Kanfold role is not in the map file"), Kanfold.Role.IsEmpty());
	TestTrue(TEXT("Kanfold has more than one cell"), Kanfold.Cells.Num() > 1);

	FSolidWorldLocation Visolar;
	TestTrue(TEXT("location 3"), Map->FindLocation(3, Visolar));
	TestEqual(TEXT("Visolar"), Visolar.Name, FString(TEXT("Visolar")));
	TestTrue(TEXT("Visolar role is not in the map file"), Visolar.Role.IsEmpty());
	TestEqual(TEXT("one Visolar cell"), Visolar.Cells.Num(), 1);

	FSolidWorldLocation Jethan;
	TestTrue(TEXT("location 4"), Map->FindLocation(4, Jethan));
	TestEqual(TEXT("Jethan"), Jethan.Name, FString(TEXT("Jethan")));
	TestTrue(TEXT("Jethan role is not in the map file"), Jethan.Role.IsEmpty());
	TestEqual(TEXT("Jethan is Town"), static_cast<uint8>(Jethan.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("one Jethan cell"), Jethan.Cells.Num(), 1);

	FSolidWorldLocation Rathen;
	TestTrue(TEXT("location 6"), Map->FindLocation(6, Rathen));
	TestEqual(TEXT("Rathen"), Rathen.Name, FString(TEXT("Rathen")));
	TestTrue(TEXT("Rathen role is not in the map file"), Rathen.Role.IsEmpty());
	TestEqual(TEXT("Rathen is Town"), static_cast<uint8>(Rathen.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("one Rathen cell"), Rathen.Cells.Num(), 1);
	bool bRathenBesideRiver = false;
	for (const FIntPoint& Cell : Rathen.Cells)
	{
		const FIntPoint Steps[] = { FIntPoint(0, -1), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(1, 0) };
		for (const FIntPoint& Step : Steps)
		{
			const int32 NextX = Cell.X + Step.X;
			const int32 NextY = Cell.Y + Step.Y;
			if (NextX < 0 || NextY < 0 || NextX >= USolidWorldMap::MapWidth || NextY >= USolidWorldMap::MapHeight)
			{
				continue;
			}
			if (Map->GetMarkerAtCell(NextX, NextY) == TEXT('R'))
			{
				bRathenBesideRiver = true;
			}
		}
	}
	TestTrue(TEXT("Rathen is beside the river"), bRathenBesideRiver);

	const SolidTowns::FTownInfo* WallGate = nullptr;
	for (const SolidTowns::FTownInfo& Info : SolidTowns::Definitions)
	{
		if (Info.Type == SolidTowns::ETownType::WallGate)
		{
			WallGate = &Info;
			break;
		}
	}
	TestNotNull(TEXT("a wall gate is defined"), WallGate);
	FSolidWorldLocation Dragongate;
	if (WallGate && Map->FindLocation(WallGate->Id, Dragongate))
	{
		TestEqual(TEXT("Dragongate"), Dragongate.Name, FString(TEXT("Dragongate")));
		TestEqual(TEXT("Dragongate is Town"), static_cast<uint8>(Dragongate.Biome), static_cast<uint8>(ESolidBiome::Town));
		TestTrue(TEXT("Dragongate has a cell"), Dragongate.Cells.Num() > 0);
	}

	TestEqual(TEXT("start is only location 0"), Map->GetStartTownCellCount(), Iglin.Cells.Num());

	const FSolidWorldLocation* Named[] = { &Iglin, &Relion, &Kanfold, &Visolar, &Jethan, &Rathen };
	for (const FSolidWorldLocation* Loc : Named)
	{
		for (const FIntPoint& Cell : Loc->Cells)
		{
			TestEqual(
				*FString::Printf(TEXT("location %d cell is its biome"), Loc->Id),
				static_cast<uint8>(Map->GetBiomeAtCell(Cell.X, Cell.Y)),
				static_cast<uint8>(Loc->Biome));
		}
	}

	const FVector2D WorldMin(-1000.f, -1000.f);
	const FVector2D WorldMax(1000.f, 1000.f);
	FVector2D StartXY = FVector2D::ZeroVector;
	FVector2D IglinXY = FVector2D::ZeroVector;
	TestTrue(TEXT("start XY"), Map->GetStartTownWorldXY(WorldMin, WorldMax, StartXY));
	TestTrue(TEXT("Iglin XY"), Map->GetLocationWorldXY(0, WorldMin, WorldMax, IglinXY));
	TestTrue(TEXT("start matches location 0"), StartXY.Equals(IglinXY, 0.1f));

	FVector2D RelionXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Relion XY"), Map->GetLocationWorldXY(1, WorldMin, WorldMax, RelionXY));
	TestFalse(TEXT("capital is not the start"), RelionXY.Equals(StartXY, 1.f));

	// Digits are grid markers. Location 0 is the start even if a legacy Z is also present.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 2 && Col == 4)
			{
				Line.AppendChar(TEXT('0'));
			}
			else if (Row == 2 && Col == 8)
			{
				Line.AppendChar(TEXT('Z'));
			}
			else if (Row == 4 && Col == 6)
			{
				Line.AppendChar(TEXT('1'));
			}
			else
			{
				Line.AppendChar(TEXT('G'));
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("G = Grassland (light green)\n");
	Text += TEXT("0 = Starting village, biome=Town, name=Ignored\n");
	Text += TEXT("1 = Watch, biome=Forest, name=Pine\n");
	Text += TEXT("Z = Starting town (brown)\n");

	USolidWorldMap* Synthetic = NewObject<USolidWorldMap>();
	TestTrue(TEXT("synthetic loads digits"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("start ignores Z when 0 is present"), Synthetic->GetStartTownCellCount(), 1);
	TestEqual(
		TEXT("0 cell is Town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(4, 2)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("Z is still Town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(8, 2)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("1 cell uses biome=Forest"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(6, 4)),
		static_cast<uint8>(ESolidBiome::Forest));

	FSolidWorldLocation IndexOne;
	TestTrue(TEXT("location 1"), Synthetic->FindLocation(1, IndexOne));
	TestEqual(TEXT("name comes from the town table, not name="), IndexOne.Name, FString(TEXT("Relion")));
	TestEqual(TEXT("role still comes from the file"), IndexOne.Role, FString(TEXT("Watch")));
	TestEqual(
		TEXT("biome still comes from the file"),
		static_cast<uint8>(IndexOne.Biome),
		static_cast<uint8>(ESolidBiome::Forest));
	FSolidWorldLocation IndexZero;
	TestTrue(TEXT("location 0"), Synthetic->FindLocation(0, IndexZero));
	TestEqual(TEXT("index 0 ignores name=Ignored"), IndexZero.Name, FString(TEXT("Iglin")));

	FVector2D SyntheticStart = FVector2D::ZeroVector;
	const FVector2D SynMin(0.f, 0.f);
	const FVector2D SynMax(6400.f, 6400.f);
	TestTrue(TEXT("synthetic start"), Synthetic->GetStartTownWorldXY(SynMin, SynMax, SyntheticStart));
	// Column 0 is east. Cell (4, 2) is one column and one row in from that corner.
	const float CellW = (SynMax.X - SynMin.X) / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellH = (SynMax.Y - SynMin.Y) / static_cast<float>(USolidWorldMap::MapHeight);
	TestTrue(TEXT("start is the 0 cell, not Z"),
		FMath::IsNearlyEqual(SyntheticStart.X, SynMax.X - 4.5f * CellW, 1.f));
	TestTrue(TEXT("start row is the 0 cell"),
		FMath::IsNearlyEqual(SyntheticStart.Y, SynMax.Y - 2.5f * CellH, 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapDoubledScaleTest,
	"SolidCore1.WorldMap.DoubledScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapDoubledScaleTest::RunTest(const FString& Parameters)
{
	const ASolidTerrainStreamer* Defaults = GetDefault<ASolidTerrainStreamer>();
	TestNotNull(TEXT("streamer defaults"), Defaults);
	TestEqual(TEXT("points doubled from 257"), Defaults->TerrainMapSize, 513);
	TestEqual(TEXT("spacing stays 200 cm"), Defaults->TerrainPointSpacing, 200.f);

	const float Extent = static_cast<float>(Defaults->TerrainMapSize - 1) * Defaults->TerrainPointSpacing;
	TestTrue(TEXT("world side is 1024 m"), FMath::IsNearlyEqual(Extent, 102400.f, 1.f));
	const float CellEastCm = Extent / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellNorthCm = Extent / static_cast<float>(USolidWorldMap::MapHeight);
	const FVector2D WorldMin(-Extent * 0.5f, -Extent * 0.5f);
	const FVector2D WorldMax(Extent * 0.5f, Extent * 0.5f);

	USolidWorldMap* WorldMap = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadDefault"), WorldMap->LoadDefault());

	FVector2D EastCell = FVector2D::ZeroVector;
	FVector2D NextColumn = FVector2D::ZeroVector;
	FVector2D NextRow = FVector2D::ZeroVector;
	TestTrue(TEXT("first cell projects"), WorldMap->CellToWorldXY(FIntPoint(0, 0), WorldMin, WorldMax, EastCell));
	TestTrue(TEXT("next column projects"), WorldMap->CellToWorldXY(FIntPoint(1, 0), WorldMin, WorldMax, NextColumn));
	TestTrue(TEXT("next row projects"), WorldMap->CellToWorldXY(FIntPoint(0, 1), WorldMin, WorldMax, NextRow));
	TestTrue(TEXT("column step follows the map width"),
		FMath::IsNearlyEqual(EastCell.X - NextColumn.X, CellEastCm, 1.f));
	TestTrue(TEXT("row step follows the map height"),
		FMath::IsNearlyEqual(EastCell.Y - NextRow.Y, CellNorthCm, 1.f));

	// Previous rectangle was half this size. A sample and its doubled twin must hit the same marker.
	const FVector2D OldMin(-25600.f, -25600.f);
	const FVector2D OldMax(25600.f, 25600.f);
	const FVector2D NewMin = OldMin * 2.f;
	const FVector2D NewMax = OldMax * 2.f;
	const FVector2D Samples[] = {
		FVector2D(0.f, 0.f),
		FVector2D(12000.f, -8000.f),
		FVector2D(-18000.f, 22000.f),
		FVector2D(25000.f, 25000.f),
	};
	for (const FVector2D& Sample : Samples)
	{
		const ESolidBiome Previous = WorldMap->SampleBiome(Sample.X, Sample.Y, OldMin, OldMax);
		const ESolidBiome Doubled = WorldMap->SampleBiome(Sample.X * 2.f, Sample.Y * 2.f, NewMin, NewMax);
		TestEqual(
			*FString::Printf(TEXT("biome scales at (%.0f, %.0f)"), Sample.X, Sample.Y),
			static_cast<uint8>(Doubled),
			static_cast<uint8>(Previous));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapDigitPairTest,
	"SolidCore1.WorldMap.DigitPairs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapDigitPairTest::RunTest(const FString& Parameters)
{
	TArray<FString> Rows;
	Rows.SetNum(USolidWorldMap::MapHeight);
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			Line.AppendChar(TEXT('G'));
		}
		Rows[Row] = Line;
	}

	auto Put = [&Rows](const int32 Col, const int32 Row, const TCHAR* Text)
	{
		for (int32 Index = 0; Text[Index] != 0; ++Index)
		{
			Rows[Row][Col + Index] = Text[Index];
		}
	};
	Put(4, 4, TEXT("05"));
	Put(8, 6, TEXT("0"));
	Put(4, 8, TEXT("02"));
	Put(4, 10, TEXT("1"));
	Put(4, 11, TEXT("3"));
	Put(4, 14, TEXT("123"));
	Put(4, 16, TEXT("55"));
	Put(0, 20, TEXT("08"));

	FString Text;
	for (const FString& Row : Rows)
	{
		Text += Row;
		Text += TEXT("\n");
	}
	Text += TEXT("G = Grassland (light green)\n");
	Text += TEXT("15 = Outpost, biome=Forest\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("synthetic pair map loads"), Map->LoadFromString(Text));

	TestEqual(TEXT("eastern digit of 05"), Map->GetMarkerAtCell(4, 4), TEXT('0'));
	TestEqual(TEXT("western digit of 05"), Map->GetMarkerAtCell(5, 4), TEXT('5'));
	TestEqual(TEXT("05 east cell is location 5"), Map->GetLocationIdAtCell(4, 4), 5);
	TestEqual(TEXT("05 west cell is location 5"), Map->GetLocationIdAtCell(5, 4), 5);
	TestEqual(
		TEXT("05 east cell is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(4, 4)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("05 west cell is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(5, 4)),
		static_cast<uint8>(ESolidBiome::Town));

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);
	const float CellEastCm = (WorldMax.X - WorldMin.X) / static_cast<float>(USolidWorldMap::MapWidth);
	FVector2D PairEast = FVector2D::ZeroVector;
	FVector2D PairWest = FVector2D::ZeroVector;
	TestTrue(TEXT("05 east cell projects"), Map->CellToWorldXY(FIntPoint(4, 4), WorldMin, WorldMax, PairEast));
	TestTrue(TEXT("05 west cell projects"), Map->CellToWorldXY(FIntPoint(5, 4), WorldMin, WorldMax, PairWest));
	TestEqual(TEXT("05 east cell samples as 5"), Map->SampleLocationId(PairEast.X, PairEast.Y, WorldMin, WorldMax), 5);
	TestEqual(TEXT("05 west cell samples as 5"), Map->SampleLocationId(PairWest.X, PairWest.Y, WorldMin, WorldMax), 5);
	TestTrue(TEXT("05 is still two cells of ground"),
		FMath::IsNearlyEqual(PairEast.X - PairWest.X, CellEastCm, 1.f));

	FSolidWorldLocation Dragongate;
	TestTrue(TEXT("location 5"), Map->FindLocation(5, Dragongate));
	TestEqual(TEXT("05 is the only location 5"), Dragongate.Cells.Num(), 2);
	TestEqual(TEXT("05 is Dragongate"), Dragongate.Name, FString(TEXT("Dragongate")));

	TestEqual(TEXT("one start cell"), Map->GetStartTownCellCount(), 1);
	TestEqual(TEXT("lone 0 is Iglin"), Map->GetLocationIdAtCell(8, 6), 0);
	FSolidWorldLocation Iglin;
	TestTrue(TEXT("location 0"), Map->FindLocation(0, Iglin));
	TestEqual(TEXT("Iglin is the lone 0"), Iglin.Cells.Num(), 1);
	if (Iglin.Cells.Num() == 1)
	{
		TestEqual(TEXT("Iglin column"), Iglin.Cells[0].X, 8);
		TestEqual(TEXT("Iglin row"), Iglin.Cells[0].Y, 6);
	}

	TestEqual(TEXT("02 east cell is location 2"), Map->GetLocationIdAtCell(4, 8), 2);
	TestEqual(TEXT("02 west cell is location 2"), Map->GetLocationIdAtCell(5, 8), 2);
	FSolidWorldLocation Kanfold;
	TestTrue(TEXT("location 2"), Map->FindLocation(2, Kanfold));
	TestEqual(TEXT("02 is two Kanfold cells"), Kanfold.Cells.Num(), 2);

	TestEqual(TEXT("a digit above another stays itself"), Map->GetLocationIdAtCell(4, 10), 1);
	TestEqual(TEXT("a digit below another stays itself"), Map->GetLocationIdAtCell(4, 11), 3);
	FSolidWorldLocation Thirteen;
	TestFalse(TEXT("vertical digits are not one number"), Map->FindLocation(13, Thirteen));

	TestEqual(TEXT("123 starts as 12"), Map->GetLocationIdAtCell(4, 14), 12);
	TestEqual(TEXT("123 continues as 12"), Map->GetLocationIdAtCell(5, 14), 12);
	TestEqual(TEXT("123 leaves the last digit"), Map->GetLocationIdAtCell(6, 14), 3);

	TestEqual(TEXT("55 east cell is location 55"), Map->GetLocationIdAtCell(4, 16), 55);
	TestEqual(TEXT("55 west cell is location 55"), Map->GetLocationIdAtCell(5, 16), 55);
	FSolidWorldLocation FiftyFive;
	TestTrue(TEXT("location 55"), Map->FindLocation(55, FiftyFive));
	TestEqual(TEXT("55 is two cells"), FiftyFive.Cells.Num(), 2);
	TestTrue(TEXT("location 55 has no town name"), FiftyFive.Name.IsEmpty());

	TestEqual(TEXT("east-edge pair is location 8"), Map->GetLocationIdAtCell(0, 20), 8);
	TestEqual(TEXT("east-edge pair shares location 8"), Map->GetLocationIdAtCell(1, 20), 8);

	FSolidWorldLocation Outpost;
	TestTrue(TEXT("a two-digit key line is a location"), Map->FindLocation(15, Outpost));
	TestEqual(TEXT("two-digit key role"), Outpost.Role, FString(TEXT("Outpost")));
	TestEqual(
		TEXT("two-digit key biome"),
		static_cast<uint8>(Outpost.Biome),
		static_cast<uint8>(ESolidBiome::Forest));
	TestEqual(TEXT("key line without cells has none"), Outpost.Cells.Num(), 0);
	TestTrue(TEXT("unlisted two-digit id has no name"), Outpost.Name.IsEmpty());

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}

	FSolidWorldLocation ShippedIglin;
	TestTrue(TEXT("shipped location 0"), Shipped->FindLocation(0, ShippedIglin));
	TestEqual(TEXT("shipped Iglin is one cell"), ShippedIglin.Cells.Num(), 1);
	if (ShippedIglin.Cells.Num() == 1)
	{
		const FIntPoint Cell = ShippedIglin.Cells[0];
		TestEqual(TEXT("shipped Iglin marker is 0"), Shipped->GetMarkerAtCell(Cell.X, Cell.Y), TEXT('0'));
		const FIntPoint Steps[] = {
			FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1),
		};
		for (const FIntPoint& Step : Steps)
		{
			const int32 NextX = Cell.X + Step.X;
			const int32 NextY = Cell.Y + Step.Y;
			if (NextX < 0 || NextY < 0 || NextX >= USolidWorldMap::MapWidth || NextY >= USolidWorldMap::MapHeight)
			{
				continue;
			}
			TestFalse(
				TEXT("shipped Iglin stands beside a non-digit"),
				FChar::IsDigit(Shipped->GetMarkerAtCell(NextX, NextY)));
		}
	}

	FSolidWorldLocation ShippedKanfold;
	TestTrue(TEXT("shipped location 2"), Shipped->FindLocation(2, ShippedKanfold));
	TestEqual(TEXT("shipped Kanfold is the 02 pair"), ShippedKanfold.Cells.Num(), 2);
	if (ShippedKanfold.Cells.Num() == 2)
	{
		TestEqual(TEXT("Kanfold cells share a row"), ShippedKanfold.Cells[0].Y, ShippedKanfold.Cells[1].Y);
		const FIntPoint East = ShippedKanfold.Cells[0].X < ShippedKanfold.Cells[1].X
			? ShippedKanfold.Cells[0]
			: ShippedKanfold.Cells[1];
		const FIntPoint West = ShippedKanfold.Cells[0].X < ShippedKanfold.Cells[1].X
			? ShippedKanfold.Cells[1]
			: ShippedKanfold.Cells[0];
		TestEqual(TEXT("Kanfold cells are adjacent"), West.X - East.X, 1);
		TestEqual(TEXT("Kanfold tens digit is 0"), Shipped->GetMarkerAtCell(East.X, East.Y), TEXT('0'));
		TestEqual(TEXT("Kanfold ones digit is 2"), Shipped->GetMarkerAtCell(West.X, West.Y), TEXT('2'));
		TestEqual(TEXT("Kanfold east cell id"), Shipped->GetLocationIdAtCell(East.X, East.Y), 2);
		TestEqual(TEXT("Kanfold west cell id"), Shipped->GetLocationIdAtCell(West.X, West.Y), 2);
	}

	FSolidWorldLocation ShippedGate;
	TestTrue(TEXT("shipped location 5"), Shipped->FindLocation(5, ShippedGate));
	TestEqual(TEXT("shipped Dragongate is two cells"), ShippedGate.Cells.Num(), 2);
	if (ShippedGate.Cells.Num() == 2)
	{
		TestEqual(TEXT("Dragongate cells share a row"), ShippedGate.Cells[0].Y, ShippedGate.Cells[1].Y);
		const FIntPoint East = ShippedGate.Cells[0].X < ShippedGate.Cells[1].X
			? ShippedGate.Cells[0]
			: ShippedGate.Cells[1];
		const FIntPoint West = ShippedGate.Cells[0].X < ShippedGate.Cells[1].X
			? ShippedGate.Cells[1]
			: ShippedGate.Cells[0];
		TestEqual(TEXT("Dragongate cells are adjacent"), West.X - East.X, 1);
		TestEqual(TEXT("Dragongate tens digit is 0"), Shipped->GetMarkerAtCell(East.X, East.Y), TEXT('0'));
		TestEqual(TEXT("Dragongate ones digit is 5"), Shipped->GetMarkerAtCell(West.X, West.Y), TEXT('5'));
		TestEqual(TEXT("Dragongate east cell id"), Shipped->GetLocationIdAtCell(East.X, East.Y), 5);
		TestEqual(TEXT("Dragongate west cell id"), Shipped->GetLocationIdAtCell(West.X, West.Y), 5);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapSeaMarkerTest,
	"SolidCore1.WorldMap.SeaMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapSeaMarkerTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the sea marker is a wave"), USolidWorldMap::SeaMarker, TEXT('~'));
	TestTrue(TEXT("a wave joins the sea"), SolidRiver::Joins(USolidWorldMap::SeaMarker));
	TestFalse(TEXT("the letter S does not join the sea"), SolidRiver::Joins(TEXT('S')));
	TestFalse(TEXT("a lower-case s does not join the sea"), SolidRiver::Joins(TEXT('s')));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 0 && Col == 1)
			{
				Line.AppendChar(USolidWorldMap::SeaMarker);
			}
			else if (Row == 0 && Col == 2)
			{
				Line.AppendChar(TEXT('S'));
			}
			else
			{
				Line.AppendChar(TEXT('G'));
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("~ = Sea (blue)\n");

	USolidWorldMap* Synthetic = NewObject<USolidWorldMap>();
	TestTrue(TEXT("wave map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a wave is sea"), Synthetic->GetMarkerAtCell(1, 0), USolidWorldMap::SeaMarker);
	TestEqual(
		TEXT("a wave cell is the sea biome"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("the letter S stays a letter"), Synthetic->GetMarkerAtCell(2, 0), TEXT('S'));
	TestEqual(
		TEXT("the letter S is not sea"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	TestEqual(TEXT("the north-east cell is a wave"), Shipped->GetMarkerAtCell(0, 0), USolidWorldMap::SeaMarker);
	bool bSawSea = false;
	int32 LetterS = 0;
	int32 SeaNotWave = 0;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const TCHAR Marker = Shipped->GetMarkerAtCell(Col, Row);
			if (Marker == TEXT('S') || Marker == TEXT('s'))
			{
				++LetterS;
			}
			if (Shipped->GetBiomeAtCell(Col, Row) == ESolidBiome::Sea)
			{
				bSawSea = true;
				if (Marker != USolidWorldMap::SeaMarker)
				{
					++SeaNotWave;
				}
			}
		}
	}
	TestEqual(TEXT("the shipped map has no letter S"), LetterS, 0);
	TestEqual(TEXT("sea cells are waves"), SeaNotWave, 0);
	TestTrue(TEXT("the shipped map has sea"), bSawSea);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
