#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
