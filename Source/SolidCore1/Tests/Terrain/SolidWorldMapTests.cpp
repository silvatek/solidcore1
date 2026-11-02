#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

namespace
{
	bool IglinWorldXY(
		const USolidWorldChart* Chart,
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		FVector2D& OutXY)
	{
		FSolidWorldChartMark Mark;
		if (!Chart || !Chart->FindTownMarker(SolidTowns::Iglin, Mark))
		{
			return false;
		}
		return Chart->TerrainPointToWorld(Mark.TerrainPoint, WorldMin, WorldMax, OutXY);
	}

	bool WorldMapCellWorld(
		const USolidWorldChart& Chart,
		const FIntPoint Cell,
		const FVector2D& WorldMin,
		const FVector2D& WorldMax,
		FVector2D& OutXY)
	{
		return Chart.TerrainPointToWorld(FIntPoint(510 - 4 * Cell.X, 508 - 8 * Cell.Y), WorldMin, WorldMax, OutXY);
	}
}

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapLoadDefaultTest,
	"SolidCore1.WorldMap.LoadDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapLoadDefaultTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("LoadDefault succeeds"), Map->LoadDefault());
	TestTrue(TEXT("reports loaded"), Map->IsLoaded());
	FSolidWorldChartMark Iglin;
	TestTrue(TEXT("Iglin has a mark"), Map->FindTownMarker(SolidTowns::Iglin, Iglin));

	const int32 NorthX = 510;
	const int32 NorthY = 508;
	TestEqual(
		TEXT("north-west point is sea"),
		static_cast<uint8>(Map->GetPointBiome(FIntPoint(NorthX, NorthY))),
		static_cast<uint8>(ESolidBiome::Sea));

	bool bFoundTown = false;
	for (const ESolidBiome Point : Map->Points)
	{
		if (Point == ESolidBiome::Town)
		{
			bFoundTown = true;
			break;
		}
	}
	TestTrue(TEXT("map contains town ground"), bFoundTown);

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
	TestTrue(TEXT("terrain exposes start town"),
		IglinWorldXY(Terrain->GetWorldMap(), Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), TownXY));

	// The small fixture steps across the whole world, so its nearest point can
	// miss a painted town. Play uses the chart grid. A road ends on the town
	// point, and that sample is the road because town and road share a shelf.
	USolidTerrainMap* ChartGrid = NewObject<USolidTerrainMap>();
	TestNotNull(TEXT("chart grid"), ChartGrid);
	if (ChartGrid)
	{
		ChartGrid->Build(
			/*InSeed=*/1337,
			/*FrequencyScale=*/0.00012f,
			/*Amplitude=*/3000.f,
			/*BaseHeight=*/0.f,
			/*InGridWidth=*/USolidWorldChart::PointAxis,
			/*InGridHeight=*/USolidWorldChart::PointAxis,
			/*InPointSpacing=*/200.f,
			/*bForceRebuild=*/true);
		USolidWorldChart* Chart = ChartGrid->GetWorldMap();
		FVector2D ChartTown = FVector2D::ZeroVector;
		FSolidWorldChartMark IglinMark;
		if (TestNotNull(TEXT("chart grid map"), Chart)
			&& TestTrue(TEXT("chart grid exposes the start town"),
				IglinWorldXY(Chart, ChartGrid->GetWorldMinXY(), ChartGrid->GetWorldMaxXY(), ChartTown))
			&& TestTrue(TEXT("Iglin has a mark"), Chart->FindTownMarker(SolidTowns::Iglin, IglinMark)))
		{
			TestEqual(
				TEXT("the start town point is Town"),
				static_cast<uint8>(Chart->GetPointBiome(IglinMark.TerrainPoint)),
				static_cast<uint8>(ESolidBiome::Town));
			const ESolidBiome AtTown = ChartGrid->SamplePoint(ChartTown.X, ChartTown.Y).Biome;
			TestTrue(
				TEXT("the terrain at the town is the town or the road through it"),
				AtTown == ESolidBiome::Town || AtTown == ESolidBiome::Road);
		}
	}

	// MakeSmallMap centers fog on a fixture player at the origin, not on the start cell.
	const FVector2D FogOrigin = Terrain->GetFogOriginWorldCm();
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
	TestTrue(TEXT("start town exists"),
		IglinWorldXY(Map->GetWorldMap(), Map->GetWorldMinXY(), Map->GetWorldMaxXY(), StartXY));
	TestTrue(TEXT("build leaves the Z town fogged"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(StartXY.X, StartXY.Y).Fog, 1.f));
	TestTrue(TEXT("build leaves the world origin fogged"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(0.f, 0.f).Fog, 1.f));

	const FVector2D Player(1200.f, -800.f);
	Map->CenterExplorationFogOn(Player.X, Player.Y);
	TestTrue(TEXT("fog origin is the player"),
		Map->GetFogOriginWorldCm().Equals(Player, 1.f));
	TestTrue(TEXT("player cell is clear"),
		FMath::IsNearlyEqual(Map->GetNearestPoint(Player.X, Player.Y).Fog, 0.f));
	TestTrue(TEXT("Z town stays fogged when the player is elsewhere"),
		FVector2D::Distance(StartXY, Player) > SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters)
		&& FMath::IsNearlyEqual(Map->GetNearestPoint(StartXY.X, StartXY.Y).Fog, 1.f));

	// Standing on the start town is what clears it. The marker itself is not the fog key.
	Map->CenterExplorationFogOn(StartXY.X, StartXY.Y);
	TestTrue(TEXT("fog origin follows the player onto the Z town"),
		Map->GetFogOriginWorldCm().Equals(StartXY, 1.f));
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
	const FLinearColor Sea = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Sea);
	const FLinearColor River = USolidWorldChart::DefaultColorForBiome(ESolidBiome::River);
	const FLinearColor Mountain = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Mountain);
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
	const float ColumnStepCm = 4.f * Defaults->TerrainPointSpacing;
	const float RowStepCm = 8.f * Defaults->TerrainPointSpacing;
	const FVector2D WorldMin(-Extent * 0.5f, -Extent * 0.5f);
	const FVector2D WorldMax(Extent * 0.5f, Extent * 0.5f);

	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	TestTrue(TEXT("LoadDefault"), WorldMap->LoadDefault());

	FVector2D EastCell = FVector2D::ZeroVector;
	FVector2D NextColumn = FVector2D::ZeroVector;
	FVector2D NextRow = FVector2D::ZeroVector;
	TestTrue(TEXT("first cell projects"), WorldMapCellWorld(*WorldMap, FIntPoint(0, 0), WorldMin, WorldMax, EastCell));
	TestTrue(TEXT("next column projects"), WorldMapCellWorld(*WorldMap, FIntPoint(1, 0), WorldMin, WorldMax, NextColumn));
	TestTrue(TEXT("next row projects"), WorldMapCellWorld(*WorldMap, FIntPoint(0, 1), WorldMin, WorldMax, NextRow));
	TestTrue(TEXT("a column is four terrain steps"),
		FMath::IsNearlyEqual(EastCell.X - NextColumn.X, ColumnStepCm, 1.f));
	TestTrue(TEXT("a row is eight terrain steps"),
		FMath::IsNearlyEqual(EastCell.Y - NextRow.Y, RowStepCm, 1.f));

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
