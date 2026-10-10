#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "SolidTerrainTestHelpers.h"
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
	// Tiny synthetic map: 64 identical rows of G, with one Z in the center of row 0.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 0 && Col == 32)
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
		static_cast<uint8>(Map->GetBiomeAtCell(32, 0)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("desert marker"),
		static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
		static_cast<uint8>(ESolidBiome::Desert));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	// Row 0 = north = max Y → Z at col 32 should sample near top-center.
	const ESolidBiome NorthCenter = Map->SampleBiome(0.f, 6300.f, WorldMin, WorldMax);
	TestEqual(
		TEXT("north-center samples Town"),
		static_cast<uint8>(NorthCenter),
		static_cast<uint8>(ESolidBiome::Town));

	// File column 0 is east (max X): desert at col 10 → positive X.
	const float DesertWorldX = WorldMax.X - (10.5f) * ((WorldMax.X - WorldMin.X) / 64.f);
	const float DesertWorldY = WorldMax.Y - (10.5f) * ((WorldMax.Y - WorldMin.Y) / 64.f);
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
	TestEqual(TEXT("five locations"), Map->GetLocationCount(), 5);

	FSolidWorldLocation Iglin;
	TestTrue(TEXT("location 0"), Map->FindLocation(0, Iglin));
	TestEqual(TEXT("Iglin"), Iglin.Name, FString(TEXT("Iglin")));
	TestTrue(TEXT("role is not in the map file"), Iglin.Role.IsEmpty());
	TestEqual(TEXT("Iglin is Town"), static_cast<uint8>(Iglin.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("one Iglin cell"), Iglin.Cells.Num(), 1);

	FSolidWorldLocation Relion;
	TestTrue(TEXT("location 1"), Map->FindLocation(1, Relion));
	TestEqual(TEXT("Relion"), Relion.Name, FString(TEXT("Relion")));
	TestTrue(TEXT("Relion role is not in the map file"), Relion.Role.IsEmpty());
	TestEqual(TEXT("one Relion cell"), Relion.Cells.Num(), 1);

	FSolidWorldLocation Kanfold;
	TestTrue(TEXT("location 2"), Map->FindLocation(2, Kanfold));
	TestEqual(TEXT("Kanfold"), Kanfold.Name, FString(TEXT("Kanfold")));
	TestTrue(TEXT("Kanfold role is not in the map file"), Kanfold.Role.IsEmpty());
	TestEqual(TEXT("two Kanfold cells"), Kanfold.Cells.Num(), 2);

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

	TestEqual(TEXT("start is only location 0"), Map->GetStartTownCellCount(), Iglin.Cells.Num());

	const FSolidWorldLocation* Named[] = { &Iglin, &Relion, &Kanfold, &Visolar, &Jethan };
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
	// Column 0 is east. Cell (4, 2) center is (5950, 6150), not the Z at column 8.
	TestTrue(TEXT("start is the 0 cell, not Z"), FMath::IsNearlyEqual(SyntheticStart.X, 5950.f, 1.f));
	TestTrue(TEXT("start row is the 0 cell"), FMath::IsNearlyEqual(SyntheticStart.Y, 6150.f, 1.f));
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
	const float CellCm = Extent / static_cast<float>(USolidWorldMap::MapWidth);
	TestTrue(TEXT("world map cell is 16 m"), FMath::IsNearlyEqual(CellCm, 1600.f, 1.f));

	USolidWorldMap* WorldMap = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadDefault"), WorldMap->LoadDefault());

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

#endif // WITH_DEV_AUTOMATION_TESTS
