#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRiver.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidLair.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	void AddChartMark(
		USolidWorldChart& Chart,
		const int32 CellX,
		const int32 CellY,
		const ESolidWorldChartMarkKind Kind,
		const FString& Name,
		const FString& LairId = FString())
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = Kind;
		Mark.Name = Name;
		Mark.LairId = LairId;
		int32 TerrainX = 0;
		int32 TerrainY = 0;
		USolidWorldChart::PointUnderCell(CellX, CellY, TerrainX, TerrainY);
		Mark.TerrainPoint = FIntPoint(TerrainX, TerrainY);
		Chart.Marks.Add(Mark);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapLoadDefaultTest,
	"SolidCore1.WorldMap.LoadDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapLoadDefaultTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("LoadDefault succeeds"), Map->LoadDefault());
	TestTrue(TEXT("reports loaded"), Map->IsLoaded());
	FVector2D StartXY = FVector2D::ZeroVector;
	TestTrue(
		TEXT("has starting town Z cells"),
		Map->GetStartTownWorldXY(FVector2D(-100.f, -100.f), FVector2D(100.f, 100.f), StartXY));

	// Row 0 is all sea (north edge of the file).
	TestEqual(
		TEXT("north-west cell is sea"),
		static_cast<uint8>(Map->GetBiomeAtCell(0, 0)),
		static_cast<uint8>(ESolidBiome::Sea));

	// Find a Z cell and confirm it maps to Town.
	bool bFoundZTown = false;
	for (int32 Y = 0; Y < USolidWorldChart::MapHeight && !bFoundZTown; ++Y)
	{
		for (int32 X = 0; X < USolidWorldChart::MapWidth; ++X)
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
	// Synthetic map: grass, with one named town in the middle of row 0.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Row == 0 && Col == USolidWorldChart::MapWidth / 2)
			{
				Line.AppendChar(USolidWorldChart::DeprecatedLocationMarker);
			}
			else if (Row == 10 && Col == 10)
			{
				Line.AppendChar(SolidBiome::Of(ESolidBiome::Desert).Marker());
			}
			else
			{
				Line.AppendChar(SolidBiome::Of(ESolidBiome::Grassland).Marker());
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("\nG = Grassland (light green)\n");
	Text.AppendChar(SolidBiome::Of(ESolidBiome::Desert).Marker());
	Text += TEXT(" = Desert (yellow)\n");
	Text += TEXT("Z = Starting town (brown)\n");
	Text += FString::Printf(TEXT("! 1,%d = Town Iglin\n"), USolidWorldChart::MapWidth / 2 + 1);

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	AddChartMark(*Map, USolidWorldChart::MapWidth / 2, 0, ESolidWorldChartMarkKind::Town, TEXT("Iglin"));
	TestTrue(TEXT("LoadFromString"), Map->LoadFromString(Text));
	FVector2D StartXY = FVector2D::ZeroVector;
	TestTrue(
		TEXT("one start cell"),
		Map->GetStartTownWorldXY(FVector2D(-6400.f, -6400.f), FVector2D(6400.f, 6400.f), StartXY));
	TestEqual(
		TEXT("the named town is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(USolidWorldChart::MapWidth / 2, 0)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("desert marker"),
		static_cast<uint8>(Map->GetBiomeAtCell(10, 10)),
		static_cast<uint8>(ESolidBiome::Desert));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);

	// Row 0 = north = max Y → the town at the middle column samples near top-center.
	const ESolidBiome NorthCenter = Map->SampleBiome(0.f, 6300.f, WorldMin, WorldMax);
	TestEqual(
		TEXT("north-center samples Town"),
		static_cast<uint8>(NorthCenter),
		static_cast<uint8>(ESolidBiome::Town));

	// File column 0 is west (max X): desert at col 10 → positive X.
	const float DesertWorldX = WorldMax.X - (10.5f) * ((WorldMax.X - WorldMin.X) / static_cast<float>(USolidWorldChart::MapWidth));
	const float DesertWorldY = WorldMax.Y - (10.5f) * ((WorldMax.Y - WorldMin.Y) / static_cast<float>(USolidWorldChart::MapHeight));
	TestTrue(TEXT("desert cell is on the west side"), DesertWorldX > 0.f);
	TestEqual(
		TEXT("west-side sample is Desert"),
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
		FIntPoint TownCell = FIntPoint::ZeroValue;
		if (TestNotNull(TEXT("chart grid map"), Chart)
			&& TestTrue(TEXT("chart grid exposes the start town"), ChartGrid->GetStartTownWorldXY(ChartTown))
			&& TestTrue(
				TEXT("the start town is a cell"),
				Chart->CellAtWorld(
					ChartTown.X,
					ChartTown.Y,
					ChartGrid->GetWorldMinXY(),
					ChartGrid->GetWorldMaxXY(),
					TownCell)))
		{
			TestEqual(
				TEXT("the start town point is Town"),
				static_cast<uint8>(Chart->GetPointBiomeAtCell(TownCell.X, TownCell.Y)),
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
	TestTrue(TEXT("start town exists"), Map->GetStartTownWorldXY(StartXY));
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
	FSolidWorldMapNumberedLocationsTest,
	"SolidCore1.WorldMap.NumberedLocations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapNumberedLocationsTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("LoadDefault"), Map->LoadDefault());

	FIntPoint Iglin;
	TestTrue(TEXT("location 0"), Map->TownCell(0, Iglin));
	TestEqual(TEXT("Iglin is !"), Map->GetMarkerAtCell(Iglin.X, Iglin.Y), USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("Iglin cell id"), Map->GetLocationIdAtCell(Iglin.X, Iglin.Y), 0);
	TestEqual(
		TEXT("Iglin is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(Iglin.X, Iglin.Y)),
		static_cast<uint8>(ESolidBiome::Town));

	const int32 NamedIds[] = { 1, 2, 3, 4, 6 };
	const TCHAR* NamedLabels[] = { TEXT("Relion"), TEXT("Kanfold"), TEXT("Visolar"), TEXT("Jethan"), TEXT("Rathen") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(NamedIds); ++Index)
	{
		FIntPoint Cell;
		TestTrue(*FString::Printf(TEXT("%s"), NamedLabels[Index]), Map->TownCell(NamedIds[Index], Cell));
		TestEqual(
			*FString::Printf(TEXT("%s is !"), NamedLabels[Index]),
			Map->GetMarkerAtCell(Cell.X, Cell.Y),
			USolidWorldChart::DeprecatedLocationMarker);
		TestEqual(
			*FString::Printf(TEXT("%s cell id"), NamedLabels[Index]),
			Map->GetLocationIdAtCell(Cell.X, Cell.Y),
			NamedIds[Index]);
		TestEqual(
			*FString::Printf(TEXT("%s is Town"), NamedLabels[Index]),
			static_cast<uint8>(Map->GetBiomeAtCell(Cell.X, Cell.Y)),
			static_cast<uint8>(ESolidBiome::Town));
	}

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
	if (WallGate)
	{
		FIntPoint Dragongate;
		TestTrue(TEXT("Dragongate"), Map->TownCell(WallGate->Id, Dragongate));
		TestEqual(TEXT("Dragongate is !"), Map->GetMarkerAtCell(Dragongate.X, Dragongate.Y), USolidWorldChart::DeprecatedLocationMarker);
		TestEqual(TEXT("Dragongate cell id"), Map->GetLocationIdAtCell(Dragongate.X, Dragongate.Y), WallGate->Id);
		TestEqual(
			TEXT("Dragongate is Town"),
			static_cast<uint8>(Map->GetBiomeAtCell(Dragongate.X, Dragongate.Y)),
			static_cast<uint8>(ESolidBiome::Town));
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

	// A named ! is a town. A digit, Z, and an old key line are not.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Row == 2 && Col == 4)
			{
				Line.AppendChar(USolidWorldChart::DeprecatedLocationMarker);
			}
			else if (Row == 2 && Col == 8)
			{
				Line.AppendChar(TEXT('Z'));
			}
			else if (Row == 4 && Col == 6)
			{
				Line.AppendChar(USolidWorldChart::DeprecatedLocationMarker);
			}
			else if (Row == 6 && Col == 6)
			{
				Line.AppendChar(TEXT('0'));
			}
			else
			{
				Line.AppendChar(SolidBiome::Of(ESolidBiome::Grassland).Marker());
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("G = Grassland (light green)\n");
	Text += TEXT("! 3,5 = Town (Iglin)\n");
	Text += TEXT("! 5,7 = Town (Relion)\n");
	Text += TEXT("0 = Starting village, biome=Town, name=Ignored\n");
	Text += TEXT("1 = Watch, biome=Forest, name=Pine\n");
	Text += TEXT("Z = Starting town (brown)\n");

	USolidWorldChart* Synthetic = NewObject<USolidWorldChart>();
	AddChartMark(*Synthetic, 4, 2, ESolidWorldChartMarkKind::Town, TEXT("Iglin"));
	AddChartMark(*Synthetic, 6, 4, ESolidWorldChartMarkKind::Town, TEXT("Relion"));
	TestTrue(TEXT("synthetic loads named towns"), Synthetic->LoadFromString(Text));
	TestEqual(
		TEXT("Iglin cell is Town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(4, 2)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(
		TEXT("Z is grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(8, 2)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("Z is not a location"), Synthetic->GetLocationIdAtCell(8, 2), INDEX_NONE);
	TestEqual(
		TEXT("Relion stays Town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(6, 4)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("a digit is not a location"), Synthetic->GetLocationIdAtCell(6, 6), INDEX_NONE);
	TestEqual(
		TEXT("a digit is grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(6, 6)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestEqual(TEXT("Iglin cell id"), Synthetic->GetLocationIdAtCell(4, 2), 0);
	TestEqual(TEXT("Relion cell id"), Synthetic->GetLocationIdAtCell(6, 4), 1);

	const FLinearColor Grass = Synthetic->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("an old key line leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);

	FVector2D SyntheticStart = FVector2D::ZeroVector;
	const FVector2D SynMin(0.f, 0.f);
	const FVector2D SynMax(6400.f, 6400.f);
	TestTrue(TEXT("synthetic start"), Synthetic->GetStartTownWorldXY(SynMin, SynMax, SyntheticStart));
	// Column 0 is west. Cell (4, 2) is one column and one row in from that corner.
	const float CellW = (SynMax.X - SynMin.X) / static_cast<float>(USolidWorldChart::MapWidth);
	const float CellH = (SynMax.Y - SynMin.Y) / static_cast<float>(USolidWorldChart::MapHeight);
	TestTrue(TEXT("start is the Iglin cell, not Z"),
		FMath::IsNearlyEqual(SyntheticStart.X, SynMax.X - 4.5f * CellW, 1.f));
	TestTrue(TEXT("start row is the Iglin cell"),
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
	const float CellEastCm = Extent / static_cast<float>(USolidWorldChart::MapWidth);
	const float CellNorthCm = Extent / static_cast<float>(USolidWorldChart::MapHeight);
	const FVector2D WorldMin(-Extent * 0.5f, -Extent * 0.5f);
	const FVector2D WorldMax(Extent * 0.5f, Extent * 0.5f);

	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
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
	Rows.SetNum(USolidWorldChart::MapHeight);
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			Line.AppendChar(SolidBiome::Of(ESolidBiome::Grassland).Marker());
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
	Put(4, 14, TEXT("123"));
	Put(4, 16, TEXT("55"));
	Put(0, 20, TEXT("08"));

	FString Text;
	for (const FString& Row : Rows)
	{
		Text += Row;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (light green)\n");
	Text += TEXT("15 = Numbered key, biome=Forest\n");
	Text += TEXT("0 = Starting village, biome=Town\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("synthetic digit map loads"), Map->LoadFromString(Text));

	const int32 DigitCells[][2] = {
		{ 4, 4 }, { 5, 4 }, { 8, 6 }, { 4, 8 }, { 5, 8 },
		{ 4, 14 }, { 5, 14 }, { 6, 14 }, { 4, 16 }, { 5, 16 },
		{ 0, 20 }, { 1, 20 },
	};
	for (const auto& Cell : DigitCells)
	{
		TestEqual(TEXT("a digit is not a location"), Map->GetLocationIdAtCell(Cell[0], Cell[1]), INDEX_NONE);
		TestEqual(
			TEXT("a digit is grassland"),
			static_cast<uint8>(Map->GetBiomeAtCell(Cell[0], Cell[1])),
			static_cast<uint8>(ESolidBiome::Grassland));
	}
	FVector2D DigitStart = FVector2D::ZeroVector;
	TestFalse(
		TEXT("digits are not the start"),
		Map->GetStartTownWorldXY(FVector2D(-100.f, -100.f), FVector2D(100.f, 100.f), DigitStart));
	FIntPoint Missed;
	TestFalse(TEXT("a numbered key line is not a location"), Map->TownCell(15, Missed));
	TestFalse(TEXT("a location key line is not a town"), Map->TownCell(0, Missed));
	const FLinearColor Grass = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("an old key line leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}

	FIntPoint ShippedIglin;
	TestTrue(TEXT("shipped location 0"), Shipped->TownCell(0, ShippedIglin));
	TestEqual(TEXT("Iglin is !"), Shipped->GetMarkerAtCell(ShippedIglin.X, ShippedIglin.Y), USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("Iglin cell id"), Shipped->GetLocationIdAtCell(ShippedIglin.X, ShippedIglin.Y), 0);
	TestEqual(TEXT("Iglin row"), ShippedIglin.Y + 1, 8);
	TestEqual(TEXT("Iglin column"), ShippedIglin.X + 1, 21);

	FIntPoint ShippedKanfold;
	TestTrue(TEXT("shipped location 2"), Shipped->TownCell(2, ShippedKanfold));
	TestEqual(TEXT("Kanfold is !"), Shipped->GetMarkerAtCell(ShippedKanfold.X, ShippedKanfold.Y), USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("Kanfold cell id"), Shipped->GetLocationIdAtCell(ShippedKanfold.X, ShippedKanfold.Y), 2);

	FIntPoint ShippedGate;
	TestTrue(TEXT("shipped location 5"), Shipped->TownCell(5, ShippedGate));
	TestEqual(TEXT("Dragongate is !"), Shipped->GetMarkerAtCell(ShippedGate.X, ShippedGate.Y), USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("Dragongate cell id"), Shipped->GetLocationIdAtCell(ShippedGate.X, ShippedGate.Y), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapSeaMarkerTest,
	"SolidCore1.WorldMap.SeaMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapSeaMarkerTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("the sea marker is a wave"), SolidBiome::Of(ESolidBiome::Sea).Marker(), TEXT('~'));
	TestTrue(TEXT("a wave joins the sea"), SolidRiver::Joins(SolidBiome::Of(ESolidBiome::Sea).Marker()));
	TestFalse(TEXT("the letter S does not join the sea"), SolidRiver::Joins(TEXT('S')));
	TestFalse(TEXT("a lower-case s does not join the sea"), SolidRiver::Joins(TEXT('s')));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Row == 0 && Col == 1)
			{
				Line.AppendChar(SolidBiome::Of(ESolidBiome::Sea).Marker());
			}
			else if (Row == 0 && Col == 2)
			{
				Line.AppendChar(TEXT('S'));
			}
			else
			{
				Line.AppendChar(SolidBiome::Of(ESolidBiome::Grassland).Marker());
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("~ = Sea (blue)\n");

	USolidWorldChart* Synthetic = NewObject<USolidWorldChart>();
	TestTrue(TEXT("wave map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a wave is sea"), Synthetic->GetMarkerAtCell(1, 0), SolidBiome::Of(ESolidBiome::Sea).Marker());
	TestEqual(
		TEXT("a wave cell is the sea biome"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("the letter S stays a letter"), Synthetic->GetMarkerAtCell(2, 0), TEXT('S'));
	TestEqual(
		TEXT("the letter S is not sea"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	bool bSawSea = false;
	int32 LetterS = 0;
	int32 SeaNotWave = 0;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			const TCHAR Marker = Shipped->GetMarkerAtCell(Col, Row);
			if (Marker == TEXT('S') || Marker == TEXT('s'))
			{
				++LetterS;
			}
			if (Shipped->GetBiomeAtCell(Col, Row) == ESolidBiome::Sea)
			{
				bSawSea = true;
				if (Marker != SolidBiome::Of(ESolidBiome::Sea).Marker())
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapLandMarkersTest,
	"SolidCore1.WorldMap.LandMarkers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapLandMarkersTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("grass is a dot"), SolidBiome::Of(ESolidBiome::Grassland).Marker(), TEXT('.'));
	TestEqual(TEXT("a town cell is a small circle"), SolidBiome::Of(ESolidBiome::Town).Marker(), TEXT('o'));
	TestEqual(TEXT("forest is a hatch"), SolidBiome::Of(ESolidBiome::Forest).Marker(), TEXT('#'));
	TestEqual(TEXT("a river is a star"), SolidBiome::Of(ESolidBiome::River).Marker(), TEXT('*'));
	TestTrue(TEXT("a star joins a river"), SolidRiver::Joins(SolidBiome::Of(ESolidBiome::River).Marker()));
	TestFalse(TEXT("the letter R does not join a river"), SolidRiver::Joins(TEXT('R')));
	TestFalse(TEXT("a lower-case r does not join a river"), SolidRiver::Joins(TEXT('r')));
	TestFalse(TEXT("grass does not join a river"), SolidRiver::Joins(SolidBiome::Of(ESolidBiome::Grassland).Marker()));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			TCHAR Marker = SolidBiome::Of(ESolidBiome::Grassland).Marker();
			if (Row == 0)
			{
				if (Col == 1) Marker = TEXT('G');
				else if (Col == 2) Marker = SolidBiome::Of(ESolidBiome::Town).Marker();
				else if (Col == 3) Marker = TEXT('T');
				else if (Col == 4) Marker = SolidBiome::Of(ESolidBiome::Forest).Marker();
				else if (Col == 5) Marker = TEXT('F');
				else if (Col == 6) Marker = SolidBiome::Of(ESolidBiome::River).Marker();
				else if (Col == 7) Marker = TEXT('R');
			}
			Line.AppendChar(Marker);
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (yellow)\n");

	USolidWorldChart* Synthetic = NewObject<USolidWorldChart>();
	TestTrue(TEXT("symbol map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a dot stays a dot"), Synthetic->GetMarkerAtCell(0, 0), SolidBiome::Of(ESolidBiome::Grassland).Marker());
	TestEqual(
		TEXT("a dot is grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(0, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the letter G stays a letter"), Synthetic->GetMarkerAtCell(1, 0), TEXT('G'));
	TestEqual(
		TEXT("the letter G is still grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a circle is the town marker"), Synthetic->GetMarkerAtCell(2, 0), SolidBiome::Of(ESolidBiome::Town).Marker());
	TestEqual(
		TEXT("a circle is town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("the letter T stays a letter"), Synthetic->GetMarkerAtCell(3, 0), TEXT('T'));
	TestEqual(
		TEXT("the letter T is not town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(3, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a hatch is the forest marker"), Synthetic->GetMarkerAtCell(4, 0), SolidBiome::Of(ESolidBiome::Forest).Marker());
	TestEqual(
		TEXT("a hatch is forest"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(4, 0)),
		static_cast<uint8>(ESolidBiome::Forest));
	TestEqual(TEXT("the letter F stays a letter"), Synthetic->GetMarkerAtCell(5, 0), TEXT('F'));
	TestEqual(
		TEXT("the letter F is not forest"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(5, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a star is the river marker"), Synthetic->GetMarkerAtCell(6, 0), SolidBiome::Of(ESolidBiome::River).Marker());
	TestEqual(
		TEXT("a star is river"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(6, 0)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(TEXT("the letter R stays a letter"), Synthetic->GetMarkerAtCell(7, 0), TEXT('R'));
	TestEqual(
		TEXT("the letter R is not river"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(7, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));

	const FVector2D WorldMin(-51200.f, -51200.f);
	const FVector2D WorldMax(51200.f, 51200.f);
	FVector2D TownXY = FVector2D::ZeroVector;
	FVector2D LetterT = FVector2D::ZeroVector;
	TestTrue(TEXT("town cell projects"), Synthetic->CellToWorldXY(FIntPoint(2, 0), WorldMin, WorldMax, TownXY));
	TestTrue(TEXT("letter T projects"), Synthetic->CellToWorldXY(FIntPoint(3, 0), WorldMin, WorldMax, LetterT));
	TestTrue(TEXT("a circle is a marked town"), Synthetic->IsMarkedTown(TownXY.X, TownXY.Y, WorldMin, WorldMax));
	TestFalse(TEXT("the letter T is not a marked town"), Synthetic->IsMarkedTown(LetterT.X, LetterT.Y, WorldMin, WorldMax));

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	int32 OldLetters = 0;
	int32 GrassNotDot = 0;
	int32 ForestNotHatch = 0;
	int32 RiverNotStar = 0;
	int32 TownCircles = 0;
	bool bSawGrass = false;
	bool bSawForest = false;
	bool bSawRiver = false;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			const TCHAR Marker = Shipped->GetMarkerAtCell(Col, Row);
			if (Marker == TEXT('G') || Marker == TEXT('g')
				|| Marker == TEXT('T') || Marker == TEXT('t')
				|| Marker == TEXT('F') || Marker == TEXT('f')
				|| Marker == TEXT('R') || Marker == TEXT('r'))
			{
				++OldLetters;
			}
			if (Marker == SolidBiome::Of(ESolidBiome::Town).Marker())
			{
				++TownCircles;
			}
			const ESolidBiome Biome = Shipped->GetBiomeAtCell(Col, Row);
			if (Biome == ESolidBiome::Grassland)
			{
				bSawGrass = true;
				// A stronghold on the grass shelf is grassland, and its cell is !.
				if (Marker != SolidBiome::Of(ESolidBiome::Grassland).Marker() && Marker != USolidWorldChart::DeprecatedLocationMarker)
				{
					++GrassNotDot;
				}
			}
			else if (Biome == ESolidBiome::Forest)
			{
				bSawForest = true;
				if (Marker != SolidBiome::Of(ESolidBiome::Forest).Marker())
				{
					++ForestNotHatch;
				}
			}
			else if (Biome == ESolidBiome::River)
			{
				bSawRiver = true;
				if (Marker != SolidBiome::Of(ESolidBiome::River).Marker())
				{
					++RiverNotStar;
				}
			}
		}
	}
	TestEqual(TEXT("the shipped map has no G, T, F, or R"), OldLetters, 0);
	TestEqual(TEXT("grass cells are dots or a place mark"), GrassNotDot, 0);
	TestEqual(TEXT("forest cells are hatches"), ForestNotHatch, 0);
	TestEqual(TEXT("river cells are stars"), RiverNotStar, 0);
	TestTrue(TEXT("the shipped map has grass"), bSawGrass);
	TestTrue(TEXT("the shipped map has forest"), bSawForest);
	TestTrue(TEXT("the shipped map has a river"), bSawRiver);
	TestTrue(TEXT("the shipped map has a town circle"), TownCircles > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapDesertMarkerTest,
	"SolidCore1.WorldMap.DesertMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapDesertMarkerTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("desert is a peak"), SolidBiome::Of(ESolidBiome::Desert).Marker(), TEXT('^'));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			TCHAR Marker = SolidBiome::Of(ESolidBiome::Grassland).Marker();
			if (Row == 0 && Col == 1)
			{
				Marker = SolidBiome::Of(ESolidBiome::Desert).Marker();
			}
			else if (Row == 0 && Col == 2)
			{
				Marker = TEXT('D');
			}
			Line.AppendChar(Marker);
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text.AppendChar(SolidBiome::Of(ESolidBiome::Desert).Marker());
	Text += TEXT(" = Desert (blue)\n");

	USolidWorldChart* Synthetic = NewObject<USolidWorldChart>();
	TestTrue(TEXT("peak map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a peak stays a peak"), Synthetic->GetMarkerAtCell(1, 0), SolidBiome::Of(ESolidBiome::Desert).Marker());
	TestEqual(
		TEXT("a peak is desert"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("the letter D stays a letter"), Synthetic->GetMarkerAtCell(2, 0), TEXT('D'));
	TestEqual(
		TEXT("the letter D is not desert"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	int32 LetterD = 0;
	int32 DesertNotPeak = 0;
	int32 Mountains = 0;
	int32 Caves = 0;
	bool bSawDesert = false;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			const TCHAR Marker = Shipped->GetMarkerAtCell(Col, Row);
			if (Marker == TEXT('D') || Marker == TEXT('d'))
			{
				++LetterD;
			}
			if (Marker == TEXT('M'))
			{
				++Mountains;
			}
			ESolidStrongholdType NamedType = ESolidStrongholdType::Dungeon;
			ESolidStrongholdSize NamedSize = ESolidStrongholdSize::Meagre;
			if (Shipped->StrongholdAtCell(Col, Row, NamedType, NamedSize))
			{
				++Caves;
			}
			if (Shipped->GetBiomeAtCell(Col, Row) == ESolidBiome::Desert)
			{
				bSawDesert = true;
				if (Marker != SolidBiome::Of(ESolidBiome::Desert).Marker())
				{
					++DesertNotPeak;
				}
			}
		}
	}
	TestEqual(TEXT("the shipped map has no letter D"), LetterD, 0);
	TestEqual(TEXT("desert cells are peaks"), DesertNotPeak, 0);
	TestTrue(TEXT("the shipped map has desert"), bSawDesert);
	TestTrue(TEXT("mountains stay M"), Mountains > 0);
	TestTrue(TEXT("the shipped map has a cave"), Caves > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapXYMarkerTest,
	"SolidCore1.WorldMap.XYMarkers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapXYMarkerTest::RunTest(const FString& Parameters)
{
	const int32 DigitCol = 8;
	const int32 MarkerRow = 3;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			Line.AppendChar(Row == MarkerRow && Col == DigitCol ? TEXT('5') : SolidBiome::Of(ESolidBiome::Grassland).Marker());
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (light green)\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("a map with a digit loads"), Map->LoadFromString(Text));
	TestEqual(TEXT("a digit is not a location"), Map->GetLocationIdAtCell(DigitCol, MarkerRow), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapXYMountainTest,
	"SolidCore1.WorldMap.XYMountain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapXYMountainTest::RunTest(const FString& Parameters)
{
	const int32 MountainCol = 1;
	const int32 MarkerRow = 2;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			Line.AppendChar(Row == MarkerRow && Col == MountainCol ? TEXT('M') : SolidBiome::Of(ESolidBiome::Grassland).Marker());
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("M = Mountain (yellow)\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestTrue(TEXT("a map with a mountain loads"), Map->LoadFromString(Text));
	TestEqual(
		TEXT("M stays mountain"),
		static_cast<uint8>(Map->GetBiomeAtCell(MountainCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Mountain));

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	int32 NotGrass = 0;
	int32 LairEntrances = 0;
	int32 LairExits = 0;
	for (const FSolidWorldChartMark& Mark : Shipped->Marks)
	{
		if (Mark.Kind == ESolidWorldChartMarkKind::LairEntrance)
		{
			++LairEntrances;
		}
		else if (Mark.Kind == ESolidWorldChartMarkKind::LairExit)
		{
			++LairExits;
		}
		else
		{
			continue;
		}
		FIntPoint Cell;
		if (!USolidWorldChart::CellAtTerrainPoint(Mark.TerrainPoint, Cell)
			|| Shipped->GetBiomeAtCell(Cell.X, Cell.Y) != ESolidBiome::Grassland)
		{
			++NotGrass;
		}
	}
	TestTrue(TEXT("the shipped map has a lair entrance"), LairEntrances > 0);
	TestTrue(TEXT("the shipped map has a lair exit"), LairExits > 0);
	TestEqual(TEXT("shipped tunnel mouths keep the grassland"), NotGrass, 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapBangLocationTest,
	"SolidCore1.WorldMap.BangLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapBangLocationTest::RunTest(const FString& Parameters)
{
	const int32 IglinCol = 0;
	const int32 IglinRow = 0;
	const int32 GateCol = 3;
	const int32 GateRow = 2;
	const int32 BareCol = 6;
	const int32 BareRow = 2;
	const int32 DigitCol = 8;
	const int32 DigitRow = 4;
	const int32 PairCol = 10;
	const int32 PairRow = 4;
	const int32 SplitCol = 13;
	const int32 SplitRow = 6;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			TCHAR Marker = SolidBiome::Of(ESolidBiome::Grassland).Marker();
			if (Row == IglinRow && Col == IglinCol)
			{
				Marker = USolidWorldChart::DeprecatedLocationMarker;
			}
			else if (Row == GateRow && (Col == GateCol || Col == GateCol + 1))
			{
				Marker = USolidWorldChart::DeprecatedLocationMarker;
			}
			else if (Row == BareRow && Col == BareCol)
			{
				Marker = USolidWorldChart::DeprecatedLocationMarker;
			}
			else if (Row == DigitRow && Col == DigitCol)
			{
				Marker = TEXT('7');
			}
			else if (Row == PairRow && Col == PairCol)
			{
				Marker = TEXT('2');
			}
			else if (Row == PairRow && Col == PairCol + 1)
			{
				Marker = TEXT('3');
			}
			else if (Row == SplitRow && Col == SplitCol)
			{
				Marker = TEXT('1');
			}
			else if (Row == SplitRow && Col == SplitCol + 1)
			{
				Marker = USolidWorldChart::DeprecatedLocationMarker;
			}
			else if (Row == SplitRow && Col == SplitCol + 2)
			{
				Marker = TEXT('2');
			}
			Line.AppendChar(Marker);
		}
		Text += Line;
		Text += TEXT("\n");
	}

	Text += TEXT(". = Grassland (light green)\n");
	Text += TEXT("! = Later (red)\n");
	Text += TEXT("! 1,1 = Town (Iglin)\n");
	Text += TEXT("! 1,1 = Town (Kanfold)\n");
	Text += TEXT("! 3, 4 = Town Dragongate\n");
	Text += TEXT("! 3,5=Town Dragongate\n");
	Text += TEXT("! 1,2 = Town Visolar\n");
	Text += TEXT("! 0,1 = Town Kanfold\n");
	Text += FString::Printf(TEXT("! 1,%d = Town Jethan\n"), USolidWorldChart::MapWidth + 1);
	Text += FString::Printf(TEXT("! %d,1 = Town Rathen\n"), USolidWorldChart::MapHeight + 1);
	Text += FString::Printf(TEXT("! %d,%d = 8\n"), SplitRow + 1, SplitCol + 2);
	Text += TEXT("0 = Starting village, biome=Town\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	AddChartMark(*Map, IglinCol, IglinRow, ESolidWorldChartMarkKind::Town, TEXT("Iglin"));
	TestTrue(TEXT("a map with ! loads"), Map->LoadFromString(Text));
	TestEqual(
		TEXT("top-left marker is !"),
		Map->GetMarkerAtCell(IglinCol, IglinRow),
		USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("top-left is location 0"), Map->GetLocationIdAtCell(IglinCol, IglinRow), 0);
	FVector2D StartXY = FVector2D::ZeroVector;
	TestTrue(
		TEXT("location 0 is the start"),
		Map->GetStartTownWorldXY(FVector2D(-100.f, -100.f), FVector2D(100.f, 100.f), StartXY));

	TestEqual(
		TEXT("a named ! is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(IglinCol, IglinRow)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("east ! is location 5"), Map->GetLocationIdAtCell(GateCol, GateRow), 5);
	TestEqual(TEXT("west ! is location 5"), Map->GetLocationIdAtCell(GateCol + 1, GateRow), 5);

	TestEqual(
		TEXT("a bare ! is kept"),
		Map->GetMarkerAtCell(BareCol, BareRow),
		USolidWorldChart::DeprecatedLocationMarker);
	TestEqual(TEXT("a bare ! is not a location"), Map->GetLocationIdAtCell(BareCol, BareRow), INDEX_NONE);
	TestEqual(
		TEXT("a bare ! is grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(BareCol, BareRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestEqual(TEXT("a digit is not a location"), Map->GetLocationIdAtCell(DigitCol, DigitRow), INDEX_NONE);
	TestEqual(TEXT("a digit pair is not a location"), Map->GetLocationIdAtCell(PairCol, PairRow), INDEX_NONE);
	TestEqual(TEXT("the other digit is not a location"), Map->GetLocationIdAtCell(PairCol + 1, PairRow), INDEX_NONE);
	TestEqual(TEXT("a digit before ! is not a location"), Map->GetLocationIdAtCell(SplitCol, SplitRow), INDEX_NONE);
	TestEqual(TEXT("a numeric ! line is ignored"), Map->GetLocationIdAtCell(SplitCol + 1, SplitRow), INDEX_NONE);
	TestEqual(TEXT("a digit after ! is not a location"), Map->GetLocationIdAtCell(SplitCol + 2, SplitRow), INDEX_NONE);

	TestEqual(TEXT("that grass cell stays grass"), Map->GetMarkerAtCell(1, 0), SolidBiome::Of(ESolidBiome::Grassland).Marker());
	TestEqual(TEXT("a line on grass is not a location"), Map->GetLocationIdAtCell(1, 0), INDEX_NONE);

	const FLinearColor Grass = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("the ! key leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);
	TestFalse(TEXT("! does not join a river"), SolidRiver::Joins(USolidWorldChart::DeprecatedLocationMarker));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D IglinXY = FVector2D::ZeroVector;
	FVector2D BareXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Iglin cell"), Map->CellToWorldXY(FIntPoint(IglinCol, IglinRow), WorldMin, WorldMax, IglinXY));
	TestTrue(TEXT("bare cell"), Map->CellToWorldXY(FIntPoint(BareCol, BareRow), WorldMin, WorldMax, BareXY));
	TestTrue(TEXT("a named ! is a town"), Map->IsMarkedTown(IglinXY.X, IglinXY.Y, WorldMin, WorldMax));
	TestFalse(TEXT("a bare ! is not a town"), Map->IsMarkedTown(BareXY.X, BareXY.Y, WorldMin, WorldMax));
	TestEqual(
		TEXT("sampling the top-left reads location 0"),
		Map->SampleLocationId(IglinXY.X, IglinXY.Y, WorldMin, WorldMax),
		0);
	TestEqual(
		TEXT("sampling a bare ! reads no location"),
		Map->SampleLocationId(BareXY.X, BareXY.Y, WorldMin, WorldMax),
		INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapBangDescriptionTest,
	"SolidCore1.WorldMap.BangDescription",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapBangDescriptionTest::RunTest(const FString& Parameters)
{
	const int32 IglinCol = 0;
	const int32 IglinRow = 0;
	const int32 RelionCol = 1;
	const int32 RelionRow = 1;
	const int32 UnknownTownCol = 2;
	const int32 UnknownTypeCol = 3;
	const int32 CaveCol = 5;
	const int32 CaveRow = 5;
	const int32 TowerCol = 8;
	const int32 TowerRow = 5;
	const int32 ImperiousCol = 12;
	const int32 ImperiousRow = 5;
	const int32 BadHoldCol = 10;
	const int32 BadHoldRow = 5;
	const int32 EnterCol = 3;
	const int32 EnterRow = 9;
	const int32 ExitCol = 3;
	const int32 ExitRow = 10;
	const int32 MissingLairCol = 3;
	const int32 MissingLairRow = 11;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			const bool bBang = (Row == IglinRow && Col == IglinCol)
				|| (Row == RelionRow && (Col == RelionCol || Col == UnknownTownCol || Col == UnknownTypeCol))
				|| (Row == CaveRow && (Col == CaveCol || Col == TowerCol || Col == ImperiousCol || Col == BadHoldCol))
				|| (Col == EnterCol && (Row == EnterRow || Row == ExitRow || Row == MissingLairRow));
			Line.AppendChar(bBang ? USolidWorldChart::DeprecatedLocationMarker : SolidBiome::Of(ESolidBiome::Grassland).Marker());
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (light green)\n");
	Text += TEXT("! 1,1 = Town (Iglin) # start\n");
	Text += TEXT("! 2,2 = Town (Relion)\n");
	Text += TEXT("! 2,3 = Town Nowhere\n");
	Text += TEXT("! 2,4 = Village Iglin\n");
	Text += TEXT("! 6,6 = Stronghold MeagreDungeon # the cave\n");
	Text += TEXT("! 6,9 = Stronghold (Humble Tower)\n");
	Text += TEXT("! 6,13 = Stronghold (Imperious Tower)\n");
	Text += TEXT("! 6,11 = Stronghold NotAThing\n");
	Text += TEXT("! 10,4 = LairEntrance Iglin tunnels # south of town\n");
	Text += TEXT("! 11,4 = LairExit (Iglin tunnels)\n");
	Text += TEXT("! 12,4 = LairEntrance Missing Lair\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	AddChartMark(*Map, RelionCol, RelionRow, ESolidWorldChartMarkKind::Town, TEXT("Relion"));
	AddChartMark(*Map, CaveCol, CaveRow, ESolidWorldChartMarkKind::Stronghold, TEXT("MeagreDungeon"));
	AddChartMark(*Map, TowerCol, TowerRow, ESolidWorldChartMarkKind::Stronghold, TEXT("Humble Tower"));
	AddChartMark(*Map, ImperiousCol, ImperiousRow, ESolidWorldChartMarkKind::Stronghold, TEXT("Imperious Tower"));
	AddChartMark(
		*Map,
		EnterCol,
		EnterRow,
		ESolidWorldChartMarkKind::LairEntrance,
		SolidLair::IglinTunnels.Name,
		SolidLair::IdIglinTunnels);
	TestTrue(TEXT("described map loads"), Map->LoadFromString(Text));

	TestEqual(
		TEXT("a named town is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(RelionCol, RelionRow)),
		static_cast<uint8>(ESolidBiome::Town));

	TestEqual(TEXT("an unknown town is not a location"), Map->GetLocationIdAtCell(UnknownTownCol, RelionRow), INDEX_NONE);
	TestEqual(
		TEXT("an unknown type stays grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(UnknownTypeCol, RelionRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestEqual(TEXT("a stronghold is not a location"), Map->GetLocationIdAtCell(CaveCol, CaveRow), INDEX_NONE);
	ESolidBiome CaveShelf = ESolidBiome::Grassland;
	TestTrue(
		TEXT("the cave names a shelf"),
		SolidStronghold::BiomeFor(
			FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre),
			CaveShelf));
	TestEqual(
		TEXT("a stronghold uses its shelf"),
		static_cast<uint8>(Map->GetBiomeAtCell(CaveCol, CaveRow)),
		static_cast<uint8>(CaveShelf));

	TestEqual(
		TEXT("an unlisted tower keeps the marker biome"),
		static_cast<uint8>(Map->GetBiomeAtCell(TowerCol, TowerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestEqual(
		TEXT("Imperious Tower uses grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(ImperiousCol, ImperiousRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("an unknown stronghold stays grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(BadHoldCol, BadHoldRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestEqual(
		TEXT("a lair entrance keeps the marker biome"),
		static_cast<uint8>(Map->GetBiomeAtCell(EnterCol, EnterRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D RelionXY = FVector2D::ZeroVector;
	FVector2D CaveXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Relion cell"), Map->CellToWorldXY(FIntPoint(RelionCol, RelionRow), WorldMin, WorldMax, RelionXY));
	TestTrue(TEXT("cave cell"), Map->CellToWorldXY(FIntPoint(CaveCol, CaveRow), WorldMin, WorldMax, CaveXY));
	TestTrue(TEXT("a town ! is a town"), Map->IsMarkedTown(RelionXY.X, RelionXY.Y, WorldMin, WorldMax));
	TestFalse(TEXT("a stronghold ! is not a town"), Map->IsMarkedTown(CaveXY.X, CaveXY.Y, WorldMin, WorldMax));

	auto CheckFace = [&](const int32 Col, const int32 Row, const ESolidStrongholdType Type, const ESolidStrongholdSize Size, const TCHAR* Label)
	{
		ESolidStrongholdType ReadType = ESolidStrongholdType::Fortress;
		ESolidStrongholdSize ReadSize = ESolidStrongholdSize::Humble;
		TestTrue(Label, Map->StrongholdAtCell(Col, Row, ReadType, ReadSize));
		TestEqual(Label, static_cast<uint8>(ReadType), static_cast<uint8>(Type));
		TestEqual(Label, static_cast<uint8>(ReadSize), static_cast<uint8>(Size));
		SolidStrongholds::FEntrance Face;
		TestTrue(Label, SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(Col, Row), Face));
		TestEqual(Label, Face.Cell, FIntPoint(Col, Row));
	};
	CheckFace(CaveCol, CaveRow, ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre, TEXT("the cave cell"));
	CheckFace(TowerCol, TowerRow, ESolidStrongholdType::Tower, ESolidStrongholdSize::Humble, TEXT("the humble tower cell"));
	CheckFace(ImperiousCol, ImperiousRow, ESolidStrongholdType::Tower, ESolidStrongholdSize::Imperious, TEXT("the imperious tower cell"));

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	FSolidWorldChartMark ShippedIglin;
	TestTrue(TEXT("shipped Iglin"), Shipped->FindTownMarker(0, ShippedIglin));
	TestEqual(TEXT("shipped Iglin name"), ShippedIglin.Name, FString(TEXT("Iglin")));
	FIntPoint IglinCell;
	TestTrue(
		TEXT("shipped Iglin cell"),
		USolidWorldChart::CellAtTerrainPoint(ShippedIglin.TerrainPoint, IglinCell));
	TestEqual(TEXT("shipped Iglin column"), IglinCell.X, 20);
	TestEqual(TEXT("shipped Iglin row"), IglinCell.Y, 7);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapStrongholdSiteTest,
	"SolidCore1.WorldMap.StrongholdSite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapStrongholdSiteTest::RunTest(const FString& Parameters)
{
	const int32 HumbleCol = 3;
	const int32 HumbleRow = 3;
	const int32 AverageCol = 7;
	const int32 AverageRow = 3;
	const int32 ImperiousCol = 11;
	const int32 ImperiousRow = 3;
	const int32 VastCol = 3;
	const int32 VastRow = 4;
	const int32 JoinedCol = 7;
	const int32 JoinedRow = 4;
	const int32 CaveCol = 3;
	const int32 CaveRow = 5;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldChart::MapWidth);
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			const bool bBang = (Row == HumbleRow && Col == HumbleCol)
				|| (Row == AverageRow && Col == AverageCol)
				|| (Row == ImperiousRow && Col == ImperiousCol)
				|| (Row == VastRow && Col == VastCol)
				|| (Row == JoinedRow && Col == JoinedCol)
				|| (Row == CaveRow && Col == CaveCol);
			Line.AppendChar(bBang ? USolidWorldChart::DeprecatedLocationMarker : SolidBiome::Of(ESolidBiome::Grassland).Marker());
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (light green)\n");
	Text += TEXT("! 4,4 = StrongholdSite (Humble)\n");
	Text += TEXT("! 4,8 = StrongholdSite Average\n");
	Text += TEXT("! 4,12 = strongholdsite (imperious)\n");
	Text += TEXT("! 5,4 = StrongholdSite (Vast)\n");
	Text += TEXT("! 5,8 = StrongholdSite (Humble Tower)\n");
	Text += TEXT("! 6,4 = Stronghold (MeagreDungeon)\n");

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	auto AddSite = [](USolidWorldChart& Chart, const int32 CellX, const int32 CellY, const ESolidStrongholdSize Size)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::StrongholdSite;
		Mark.StrongholdSize = Size;
		Mark.Name = SolidStronghold::SizeName(Size);
		int32 TerrainX = 0;
		int32 TerrainY = 0;
		USolidWorldChart::PointUnderCell(CellX, CellY, TerrainX, TerrainY);
		Mark.TerrainPoint = FIntPoint(TerrainX, TerrainY);
		Chart.Marks.Add(Mark);
	};
	AddSite(*Map, HumbleCol, HumbleRow, ESolidStrongholdSize::Humble);
	AddSite(*Map, AverageCol, AverageRow, ESolidStrongholdSize::Average);
	AddSite(*Map, ImperiousCol, ImperiousRow, ESolidStrongholdSize::Imperious);
	TestTrue(TEXT("a map with sites loads"), Map->LoadFromString(Text));

	TestEqual(TEXT("a site is not a location"), Map->GetLocationIdAtCell(HumbleCol, HumbleRow), INDEX_NONE);
	TestEqual(
		TEXT("a site keeps the marker biome"),
		static_cast<uint8>(Map->GetBiomeAtCell(HumbleCol, HumbleRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	ESolidStrongholdSize ReadSize = ESolidStrongholdSize::Stately;
	TestTrue(TEXT("the site cell reports its size"), Map->StrongholdSiteAtCell(HumbleCol, HumbleRow, ReadSize));
	TestEqual(TEXT("reported size"), static_cast<uint8>(ReadSize), static_cast<uint8>(ESolidStrongholdSize::Humble));
	ESolidStrongholdType AsType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize AsSize = ESolidStrongholdSize::Meagre;
	TestFalse(TEXT("a site is not a stronghold"), Map->StrongholdAtCell(HumbleCol, HumbleRow, AsType, AsSize));

	TestEqual(
		TEXT("an unknown size stays grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(VastCol, VastRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	ReadSize = ESolidStrongholdSize::Stately;
	TestFalse(TEXT("an unknown size is not a site"), Map->StrongholdSiteAtCell(VastCol, VastRow, ReadSize));
	TestEqual(
		TEXT("a failed site lookup leaves the previous size"),
		static_cast<uint8>(ReadSize),
		static_cast<uint8>(ESolidStrongholdSize::Stately));

	TestFalse(TEXT("a stronghold is not a site"), Map->StrongholdSiteAtCell(CaveCol, CaveRow, ReadSize));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D SiteXY = FVector2D::ZeroVector;
	TestTrue(TEXT("site cell"), Map->CellToWorldXY(FIntPoint(HumbleCol, HumbleRow), WorldMin, WorldMax, SiteXY));
	TestFalse(TEXT("a site is not a town"), Map->IsMarkedTown(SiteXY.X, SiteXY.Y, WorldMin, WorldMax));

	SolidStrongholds::FEntrance CaveFace;
	TestTrue(
		TEXT("the stronghold cell has a grass face"),
		SolidStrongholds::MakeEntrance(Map, WorldMin, WorldMax, FIntPoint(CaveCol, CaveRow), CaveFace));
	TestEqual(TEXT("the face is the stronghold"), CaveFace.Cell, FIntPoint(CaveCol, CaveRow));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
