#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRiver.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainStreamer.h"
#include "SolidTerrainTestHelpers.h"
#include "SolidLair.h"
#include "SolidStronghold.h"
#include "SolidStrongholds.h"
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
	// Synthetic map: grass, with one named town in the middle of row 0.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 0 && Col == USolidWorldMap::MapWidth / 2)
			{
				Line.AppendChar(USolidWorldMap::LocationMarker);
			}
			else if (Row == 10 && Col == 10)
			{
				Line.AppendChar(USolidWorldMap::DesertMarker);
			}
			else
			{
				Line.AppendChar(USolidWorldMap::GrassMarker);
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("\nG = Grassland (light green)\n");
	Text.AppendChar(USolidWorldMap::DesertMarker);
	Text += TEXT(" = Desert (yellow)\n");
	Text += TEXT("Z = Starting town (brown)\n");
	Text += FString::Printf(TEXT("! 1,%d = Town Iglin\n"), USolidWorldMap::MapWidth / 2 + 1);

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("LoadFromString"), Map->LoadFromString(Text));
	TestEqual(TEXT("one start cell"), Map->GetStartTownCellCount(), 1);
	TestEqual(
		TEXT("the named town is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(USolidWorldMap::MapWidth / 2, 0)),
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

	// MakeSmallMap centers fog on a fixture player at the origin, not on the start cell.
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
	TestEqual(TEXT("Relion is one cell"), Relion.Cells.Num(), 1);
	if (Relion.Cells.Num() == 1)
	{
		TestEqual(TEXT("Relion is !"), Map->GetMarkerAtCell(Relion.Cells[0].X, Relion.Cells[0].Y), USolidWorldMap::LocationMarker);
	}

	FSolidWorldLocation Kanfold;
	TestTrue(TEXT("location 2"), Map->FindLocation(2, Kanfold));
	TestEqual(TEXT("Kanfold"), Kanfold.Name, FString(TEXT("Kanfold")));
	TestTrue(TEXT("Kanfold role is not in the map file"), Kanfold.Role.IsEmpty());
	TestEqual(TEXT("Kanfold is one cell"), Kanfold.Cells.Num(), 1);
	if (Kanfold.Cells.Num() == 1)
	{
		TestEqual(TEXT("Kanfold is !"), Map->GetMarkerAtCell(Kanfold.Cells[0].X, Kanfold.Cells[0].Y), USolidWorldMap::LocationMarker);
	}

	FSolidWorldLocation Visolar;
	TestTrue(TEXT("location 3"), Map->FindLocation(3, Visolar));
	TestEqual(TEXT("Visolar"), Visolar.Name, FString(TEXT("Visolar")));
	TestTrue(TEXT("Visolar role is not in the map file"), Visolar.Role.IsEmpty());
	TestEqual(TEXT("Visolar is one cell"), Visolar.Cells.Num(), 1);
	if (Visolar.Cells.Num() == 1)
	{
		TestEqual(TEXT("Visolar is !"), Map->GetMarkerAtCell(Visolar.Cells[0].X, Visolar.Cells[0].Y), USolidWorldMap::LocationMarker);
	}

	FSolidWorldLocation Jethan;
	TestTrue(TEXT("location 4"), Map->FindLocation(4, Jethan));
	TestEqual(TEXT("Jethan"), Jethan.Name, FString(TEXT("Jethan")));
	TestTrue(TEXT("Jethan role is not in the map file"), Jethan.Role.IsEmpty());
	TestEqual(TEXT("Jethan is Town"), static_cast<uint8>(Jethan.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("Jethan is one cell"), Jethan.Cells.Num(), 1);
	if (Jethan.Cells.Num() == 1)
	{
		TestEqual(TEXT("Jethan is !"), Map->GetMarkerAtCell(Jethan.Cells[0].X, Jethan.Cells[0].Y), USolidWorldMap::LocationMarker);
	}

	FSolidWorldLocation Rathen;
	TestTrue(TEXT("location 6"), Map->FindLocation(6, Rathen));
	TestEqual(TEXT("Rathen"), Rathen.Name, FString(TEXT("Rathen")));
	TestTrue(TEXT("Rathen role is not in the map file"), Rathen.Role.IsEmpty());
	TestEqual(TEXT("Rathen is Town"), static_cast<uint8>(Rathen.Biome), static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("Rathen is one cell"), Rathen.Cells.Num(), 1);
	if (Rathen.Cells.Num() == 1)
	{
		TestEqual(TEXT("Rathen is !"), Map->GetMarkerAtCell(Rathen.Cells[0].X, Rathen.Cells[0].Y), USolidWorldMap::LocationMarker);
	}
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
			if (Map->GetMarkerAtCell(NextX, NextY) == USolidWorldMap::RiverMarker)
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
		TestEqual(TEXT("Dragongate is two cells"), Dragongate.Cells.Num(), 2);
		if (Dragongate.Cells.Num() == 2)
		{
			TestEqual(TEXT("Dragongate cells share a row"), Dragongate.Cells[0].Y, Dragongate.Cells[1].Y);
			const int32 EastCol = FMath::Min(Dragongate.Cells[0].X, Dragongate.Cells[1].X);
			const int32 WestCol = FMath::Max(Dragongate.Cells[0].X, Dragongate.Cells[1].X);
			TestEqual(TEXT("Dragongate cells are adjacent"), WestCol - EastCol, 1);
			TestEqual(TEXT("Dragongate east cell is !"), Map->GetMarkerAtCell(EastCol, Dragongate.Cells[0].Y), USolidWorldMap::LocationMarker);
			TestEqual(TEXT("Dragongate west cell is !"), Map->GetMarkerAtCell(WestCol, Dragongate.Cells[0].Y), USolidWorldMap::LocationMarker);
		}
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

	// A named ! is a town. A digit, Z, and an old key line are not.
	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == 2 && Col == 4)
			{
				Line.AppendChar(USolidWorldMap::LocationMarker);
			}
			else if (Row == 2 && Col == 8)
			{
				Line.AppendChar(TEXT('Z'));
			}
			else if (Row == 4 && Col == 6)
			{
				Line.AppendChar(USolidWorldMap::LocationMarker);
			}
			else if (Row == 6 && Col == 6)
			{
				Line.AppendChar(TEXT('0'));
			}
			else
			{
				Line.AppendChar(USolidWorldMap::GrassMarker);
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

	USolidWorldMap* Synthetic = NewObject<USolidWorldMap>();
	TestTrue(TEXT("synthetic loads named towns"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("start is the named Iglin cell"), Synthetic->GetStartTownCellCount(), 1);
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

	FSolidWorldLocation IndexOne;
	TestTrue(TEXT("location 1"), Synthetic->FindLocation(1, IndexOne));
	TestEqual(TEXT("name comes from the town table"), IndexOne.Name, FString(TEXT("Relion")));
	TestTrue(TEXT("role is empty"), IndexOne.Role.IsEmpty());
	TestEqual(
		TEXT("biome stays Town"),
		static_cast<uint8>(IndexOne.Biome),
		static_cast<uint8>(ESolidBiome::Town));
	FSolidWorldLocation IndexZero;
	TestTrue(TEXT("location 0"), Synthetic->FindLocation(0, IndexZero));
	TestEqual(TEXT("index 0 is Iglin"), IndexZero.Name, FString(TEXT("Iglin")));
	TestTrue(TEXT("Iglin role is empty"), IndexZero.Role.IsEmpty());

	const FLinearColor Grass = Synthetic->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("an old key line leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);

	FVector2D SyntheticStart = FVector2D::ZeroVector;
	const FVector2D SynMin(0.f, 0.f);
	const FVector2D SynMax(6400.f, 6400.f);
	TestTrue(TEXT("synthetic start"), Synthetic->GetStartTownWorldXY(SynMin, SynMax, SyntheticStart));
	// Column 0 is east. Cell (4, 2) is one column and one row in from that corner.
	const float CellW = (SynMax.X - SynMin.X) / static_cast<float>(USolidWorldMap::MapWidth);
	const float CellH = (SynMax.Y - SynMin.Y) / static_cast<float>(USolidWorldMap::MapHeight);
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
			Line.AppendChar(USolidWorldMap::GrassMarker);
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
	Text += TEXT("15 = Outpost, biome=Forest\n");
	Text += TEXT("0 = Starting village, biome=Town\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
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
	TestFalse(TEXT("digits are not the start"), Map->HasStartTown());
	FSolidWorldLocation Outpost;
	TestFalse(TEXT("a numbered key line is not a location"), Map->FindLocation(15, Outpost));
	FSolidWorldLocation IglinKey;
	TestFalse(TEXT("a location key line is not a town"), Map->FindLocation(0, IglinKey));
	const FLinearColor Grass = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("an old key line leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);

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
		TestEqual(TEXT("Iglin is !"), Shipped->GetMarkerAtCell(Cell.X, Cell.Y), USolidWorldMap::LocationMarker);
		TestEqual(TEXT("Iglin cell id"), Shipped->GetLocationIdAtCell(Cell.X, Cell.Y), 0);
		TestEqual(TEXT("Iglin row"), Cell.Y + 1, 8);
		TestEqual(TEXT("Iglin column"), Cell.X + 1, 21);
	}

	FSolidWorldLocation ShippedKanfold;
	TestTrue(TEXT("shipped location 2"), Shipped->FindLocation(2, ShippedKanfold));
	TestEqual(TEXT("shipped Kanfold is one cell"), ShippedKanfold.Cells.Num(), 1);
	if (ShippedKanfold.Cells.Num() == 1)
	{
		const FIntPoint Cell = ShippedKanfold.Cells[0];
		TestEqual(TEXT("Kanfold is !"), Shipped->GetMarkerAtCell(Cell.X, Cell.Y), USolidWorldMap::LocationMarker);
		TestEqual(TEXT("Kanfold cell id"), Shipped->GetLocationIdAtCell(Cell.X, Cell.Y), 2);
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
		TestEqual(TEXT("Dragongate east cell is !"), Shipped->GetMarkerAtCell(East.X, East.Y), USolidWorldMap::LocationMarker);
		TestEqual(TEXT("Dragongate west cell is !"), Shipped->GetMarkerAtCell(West.X, West.Y), USolidWorldMap::LocationMarker);
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
				Line.AppendChar(USolidWorldMap::GrassMarker);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapLandMarkersTest,
	"SolidCore1.WorldMap.LandMarkers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapLandMarkersTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("grass is a dot"), USolidWorldMap::GrassMarker, TEXT('.'));
	TestEqual(TEXT("a town cell is a small circle"), USolidWorldMap::TownMarker, TEXT('o'));
	TestEqual(TEXT("forest is a hatch"), USolidWorldMap::ForestMarker, TEXT('#'));
	TestEqual(TEXT("a river is a star"), USolidWorldMap::RiverMarker, TEXT('*'));
	TestTrue(TEXT("a star joins a river"), SolidRiver::Joins(USolidWorldMap::RiverMarker));
	TestFalse(TEXT("the letter R does not join a river"), SolidRiver::Joins(TEXT('R')));
	TestFalse(TEXT("a lower-case r does not join a river"), SolidRiver::Joins(TEXT('r')));
	TestFalse(TEXT("grass does not join a river"), SolidRiver::Joins(USolidWorldMap::GrassMarker));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			TCHAR Marker = USolidWorldMap::GrassMarker;
			if (Row == 0)
			{
				if (Col == 1) Marker = TEXT('G');
				else if (Col == 2) Marker = USolidWorldMap::TownMarker;
				else if (Col == 3) Marker = TEXT('T');
				else if (Col == 4) Marker = USolidWorldMap::ForestMarker;
				else if (Col == 5) Marker = TEXT('F');
				else if (Col == 6) Marker = USolidWorldMap::RiverMarker;
				else if (Col == 7) Marker = TEXT('R');
			}
			Line.AppendChar(Marker);
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (yellow)\n");

	USolidWorldMap* Synthetic = NewObject<USolidWorldMap>();
	TestTrue(TEXT("symbol map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a dot stays a dot"), Synthetic->GetMarkerAtCell(0, 0), USolidWorldMap::GrassMarker);
	TestEqual(
		TEXT("a dot is grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(0, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the letter G stays a letter"), Synthetic->GetMarkerAtCell(1, 0), TEXT('G'));
	TestEqual(
		TEXT("the letter G is still grassland"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a circle is the town marker"), Synthetic->GetMarkerAtCell(2, 0), USolidWorldMap::TownMarker);
	TestEqual(
		TEXT("a circle is town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Town));
	TestEqual(TEXT("the letter T stays a letter"), Synthetic->GetMarkerAtCell(3, 0), TEXT('T'));
	TestEqual(
		TEXT("the letter T is not town"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(3, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a hatch is the forest marker"), Synthetic->GetMarkerAtCell(4, 0), USolidWorldMap::ForestMarker);
	TestEqual(
		TEXT("a hatch is forest"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(4, 0)),
		static_cast<uint8>(ESolidBiome::Forest));
	TestEqual(TEXT("the letter F stays a letter"), Synthetic->GetMarkerAtCell(5, 0), TEXT('F'));
	TestEqual(
		TEXT("the letter F is not forest"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(5, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("a star is the river marker"), Synthetic->GetMarkerAtCell(6, 0), USolidWorldMap::RiverMarker);
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
	const FLinearColor Grass = Synthetic->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("the grass key still loads"), Grass.R > 0.5f && Grass.G > 0.5f && Grass.B < 0.5f);

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const TCHAR Marker = Shipped->GetMarkerAtCell(Col, Row);
			if (Marker == TEXT('G') || Marker == TEXT('g')
				|| Marker == TEXT('T') || Marker == TEXT('t')
				|| Marker == TEXT('F') || Marker == TEXT('f')
				|| Marker == TEXT('R') || Marker == TEXT('r'))
			{
				++OldLetters;
			}
			if (Marker == USolidWorldMap::TownMarker)
			{
				++TownCircles;
			}
			const ESolidBiome Biome = Shipped->GetBiomeAtCell(Col, Row);
			if (Biome == ESolidBiome::Grassland)
			{
				bSawGrass = true;
				// A stronghold on the grass shelf is grassland, and its cell is !.
				if (Marker != USolidWorldMap::GrassMarker && Marker != USolidWorldMap::LocationMarker)
				{
					++GrassNotDot;
				}
			}
			else if (Biome == ESolidBiome::Forest)
			{
				bSawForest = true;
				if (Marker != USolidWorldMap::ForestMarker)
				{
					++ForestNotHatch;
				}
			}
			else if (Biome == ESolidBiome::River)
			{
				bSawRiver = true;
				if (Marker != USolidWorldMap::RiverMarker)
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
	TestEqual(TEXT("desert is a peak"), USolidWorldMap::DesertMarker, TEXT('^'));

	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			TCHAR Marker = USolidWorldMap::GrassMarker;
			if (Row == 0 && Col == 1)
			{
				Marker = USolidWorldMap::DesertMarker;
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
	Text.AppendChar(USolidWorldMap::DesertMarker);
	Text += TEXT(" = Desert (blue)\n");

	USolidWorldMap* Synthetic = NewObject<USolidWorldMap>();
	TestTrue(TEXT("peak map loads"), Synthetic->LoadFromString(Text));
	TestEqual(TEXT("a peak stays a peak"), Synthetic->GetMarkerAtCell(1, 0), USolidWorldMap::DesertMarker);
	TestEqual(
		TEXT("a peak is desert"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(1, 0)),
		static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("the letter D stays a letter"), Synthetic->GetMarkerAtCell(2, 0), TEXT('D'));
	TestEqual(
		TEXT("the letter D is not desert"),
		static_cast<uint8>(Synthetic->GetBiomeAtCell(2, 0)),
		static_cast<uint8>(ESolidBiome::Grassland));
	const FLinearColor Desert = Synthetic->GetBiomeColor(ESolidBiome::Desert);
	TestTrue(TEXT("the desert key still loads"), Desert.B > Desert.R && Desert.B > Desert.G);

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
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
			FSolidStronghold NamedCave;
			if (Shipped->StrongholdAtCell(Col, Row, NamedCave))
			{
				++Caves;
			}
			if (Shipped->GetBiomeAtCell(Col, Row) == ESolidBiome::Desert)
			{
				bSawDesert = true;
				if (Marker != USolidWorldMap::DesertMarker)
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
	const int32 XCol = 4;
	const int32 YCol = 6;
	const int32 DigitCol = 8;
	const int32 MarkerRow = 3;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			if (Row == MarkerRow && Col == XCol)
			{
				Line.AppendChar(TEXT('x'));
			}
			else if (Row == MarkerRow && Col == YCol)
			{
				Line.AppendChar(USolidWorldMap::YMarker);
			}
			else if (Row == MarkerRow && Col == DigitCol)
			{
				Line.AppendChar(TEXT('5'));
			}
			else
			{
				Line.AppendChar(USolidWorldMap::GrassMarker);
			}
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT(". = Grassland (light green)\n");
	Text += TEXT("X = Later (red)\n");
	Text += TEXT("Y = Later (blue)\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("a map with X and Y loads"), Map->LoadFromString(Text));
	TestEqual(TEXT("a lower-case x is kept as X"), Map->GetMarkerAtCell(XCol, MarkerRow), USolidWorldMap::XMarker);
	TestEqual(TEXT("Y is kept"), Map->GetMarkerAtCell(YCol, MarkerRow), USolidWorldMap::YMarker);
	TestEqual(
		TEXT("X is grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(XCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("Y is grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(YCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("X is not a location"), Map->GetLocationIdAtCell(XCol, MarkerRow), INDEX_NONE);
	TestEqual(TEXT("Y is not a location"), Map->GetLocationIdAtCell(YCol, MarkerRow), INDEX_NONE);
	TestEqual(TEXT("a digit is not a location"), Map->GetLocationIdAtCell(DigitCol, MarkerRow), INDEX_NONE);

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D XWorld = FVector2D::ZeroVector;
	FVector2D YWorld = FVector2D::ZeroVector;
	TestTrue(TEXT("X cell"), Map->CellToWorldXY(FIntPoint(XCol, MarkerRow), WorldMin, WorldMax, XWorld));
	TestTrue(TEXT("Y cell"), Map->CellToWorldXY(FIntPoint(YCol, MarkerRow), WorldMin, WorldMax, YWorld));
	TestFalse(TEXT("X is not a town"), Map->IsMarkedTown(XWorld.X, XWorld.Y, WorldMin, WorldMax));
	TestFalse(TEXT("Y is not a town"), Map->IsMarkedTown(YWorld.X, YWorld.Y, WorldMin, WorldMax));

	const FLinearColor Grass = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("the X key leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);
	TestFalse(TEXT("the map still has no start from X or Y"), Map->HasStartTown());

	FSolidStronghold Stronghold;
	TestFalse(TEXT("X is not a stronghold"), Map->StrongholdAtCell(XCol, MarkerRow, Stronghold));
	TestFalse(TEXT("Y is not a stronghold"), Map->StrongholdAtCell(YCol, MarkerRow, Stronghold));

	TArray<SolidLair::FEntrance> Mouths;
	TestEqual(TEXT("letters are not tunnel mouths"), SolidLair::CollectEntrances(Map, WorldMin, WorldMax, Mouths), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldMapXYMountainTest,
	"SolidCore1.WorldMap.XYMountain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldMapXYMountainTest::RunTest(const FString& Parameters)
{
	const int32 MountainCol = 1;
	const int32 XCol = 3;
	const int32 YCol = 5;
	const int32 MarkerRow = 2;

	FString Text;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			TCHAR Marker = USolidWorldMap::GrassMarker;
			if (Row == MarkerRow && Col == MountainCol)
			{
				Marker = TEXT('M');
			}
			else if (Row == MarkerRow && Col == XCol)
			{
				Marker = USolidWorldMap::XMarker;
			}
			else if (Row == MarkerRow && Col == YCol)
			{
				Marker = USolidWorldMap::YMarker;
			}
			Line.AppendChar(Marker);
		}
		Text += Line;
		Text += TEXT("\n");
	}
	Text += TEXT("M = Mountain (yellow)\n");
	Text += TEXT("X = Later (blue)\n");
	Text += TEXT("Y = Later (blue)\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("a map with tunnel letters loads"), Map->LoadFromString(Text));
	TestEqual(
		TEXT("X is grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(XCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("Y is grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(YCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(
		TEXT("M stays mountain"),
		static_cast<uint8>(Map->GetBiomeAtCell(MountainCol, MarkerRow)),
		static_cast<uint8>(ESolidBiome::Mountain));
	const FLinearColor Mountain = Map->GetBiomeColor(ESolidBiome::Mountain);
	TestTrue(TEXT("the X key leaves the mountain yellow"), Mountain.R > 0.5f && Mountain.G > 0.5f && Mountain.B < 0.5f);

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	if (!Shipped->IsLoaded())
	{
		return true;
	}
	int32 NotMountain = 0;
	int32 LairEntrances = 0;
	int32 LairExits = 0;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			FSolidWorldBang Bang;
			if (!Shipped->FindBang(Col, Row, Bang))
			{
				continue;
			}
			if (Bang.Kind == ESolidWorldBang::LairEntrance)
			{
				++LairEntrances;
			}
			else if (Bang.Kind == ESolidWorldBang::LairExit)
			{
				++LairExits;
			}
			else
			{
				continue;
			}
			if (Shipped->GetBiomeAtCell(Col, Row) != ESolidBiome::Mountain)
			{
				++NotMountain;
			}
		}
	}
	TestTrue(TEXT("the shipped map has a lair entrance"), LairEntrances > 0);
	TestTrue(TEXT("the shipped map has a lair exit"), LairExits > 0);
	TestEqual(TEXT("shipped tunnel mouths are mountain"), NotMountain, 0);
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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			TCHAR Marker = USolidWorldMap::GrassMarker;
			if (Row == IglinRow && Col == IglinCol)
			{
				Marker = USolidWorldMap::LocationMarker;
			}
			else if (Row == GateRow && (Col == GateCol || Col == GateCol + 1))
			{
				Marker = USolidWorldMap::LocationMarker;
			}
			else if (Row == BareRow && Col == BareCol)
			{
				Marker = USolidWorldMap::LocationMarker;
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
				Marker = USolidWorldMap::LocationMarker;
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
	Text += FString::Printf(TEXT("! 1,%d = Town Jethan\n"), USolidWorldMap::MapWidth + 1);
	Text += FString::Printf(TEXT("! %d,1 = Town Rathen\n"), USolidWorldMap::MapHeight + 1);
	Text += FString::Printf(TEXT("! %d,%d = 8\n"), SplitRow + 1, SplitCol + 2);
	Text += TEXT("0 = Starting village, biome=Town\n");

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("a map with ! loads"), Map->LoadFromString(Text));
	TestEqual(
		TEXT("top-left marker is !"),
		Map->GetMarkerAtCell(IglinCol, IglinRow),
		USolidWorldMap::LocationMarker);
	TestEqual(TEXT("top-left is location 0"), Map->GetLocationIdAtCell(IglinCol, IglinRow), 0);
	TestTrue(TEXT("location 0 is the start"), Map->HasStartTown());
	TestEqual(TEXT("one start cell"), Map->GetStartTownCellCount(), 1);

	FSolidWorldLocation Iglin;
	TestTrue(TEXT("location 0"), Map->FindLocation(0, Iglin));
	TestEqual(TEXT("Iglin name"), Iglin.Name, FString(TEXT("Iglin")));
	TestTrue(TEXT("Iglin role is empty"), Iglin.Role.IsEmpty());
	TestEqual(TEXT("Iglin is one cell"), Iglin.Cells.Num(), 1);
	TestEqual(
		TEXT("a named ! is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(IglinCol, IglinRow)),
		static_cast<uint8>(ESolidBiome::Town));

	FSolidWorldLocation Replaced;
	TestFalse(TEXT("a second line does not replace Iglin"), Map->FindLocation(SolidTowns::Kanfold, Replaced));

	FSolidWorldLocation Gate;
	TestTrue(TEXT("location 5"), Map->FindLocation(5, Gate));
	TestEqual(TEXT("two ! cells are one location"), Gate.Cells.Num(), 2);
	TestEqual(TEXT("Dragongate name"), Gate.Name, FString(TEXT("Dragongate")));
	TestEqual(TEXT("east ! is location 5"), Map->GetLocationIdAtCell(GateCol, GateRow), 5);
	TestEqual(TEXT("west ! is location 5"), Map->GetLocationIdAtCell(GateCol + 1, GateRow), 5);

	TestEqual(
		TEXT("a bare ! is kept"),
		Map->GetMarkerAtCell(BareCol, BareRow),
		USolidWorldMap::LocationMarker);
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

	FSolidWorldLocation Missed;
	TestFalse(TEXT("a line on grass is not a location"), Map->FindLocation(SolidTowns::Visolar, Missed));
	TestEqual(TEXT("that grass cell stays grass"), Map->GetMarkerAtCell(1, 0), USolidWorldMap::GrassMarker);
	TestFalse(TEXT("row 0 is ignored"), Map->FindLocation(SolidTowns::Kanfold, Missed));
	TestFalse(TEXT("a column past the grid is ignored"), Map->FindLocation(SolidTowns::Jethan, Missed));
	TestFalse(TEXT("a row past the grid is ignored"), Map->FindLocation(SolidTowns::Rathen, Missed));

	const FLinearColor Grass = Map->GetBiomeColor(ESolidBiome::Grassland);
	TestTrue(TEXT("the ! key leaves grass green"), Grass.G > Grass.R && Grass.G > Grass.B);
	TestFalse(TEXT("! does not join a river"), SolidRiver::Joins(USolidWorldMap::LocationMarker));

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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const bool bBang = (Row == IglinRow && Col == IglinCol)
				|| (Row == RelionRow && (Col == RelionCol || Col == UnknownTownCol || Col == UnknownTypeCol))
				|| (Row == CaveRow && (Col == CaveCol || Col == TowerCol || Col == ImperiousCol || Col == BadHoldCol))
				|| (Col == EnterCol && (Row == EnterRow || Row == ExitRow || Row == MissingLairRow));
			Line.AppendChar(bBang ? USolidWorldMap::LocationMarker : USolidWorldMap::GrassMarker);
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

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("described map loads"), Map->LoadFromString(Text));

	FSolidWorldBang Iglin;
	TestTrue(TEXT("named ! is a town"), Map->FindBang(IglinCol, IglinRow, Iglin));
	TestEqual(TEXT("town kind"), static_cast<uint8>(Iglin.Kind), static_cast<uint8>(ESolidWorldBang::Town));
	TestEqual(TEXT("town id"), Iglin.LocationId, 0);
	TestEqual(TEXT("town name"), Iglin.Name, FString(TEXT("Iglin")));
	TestEqual(TEXT("town cell id"), Map->GetLocationIdAtCell(IglinCol, IglinRow), 0);

	FSolidWorldBang Relion;
	TestTrue(TEXT("town name is a town"), Map->FindBang(RelionCol, RelionRow, Relion));
	TestEqual(TEXT("Relion id"), Relion.LocationId, SolidTowns::Relion);
	TestEqual(TEXT("Relion name"), Relion.Name, FString(TEXT("Relion")));
	TestEqual(
		TEXT("a named town is Town"),
		static_cast<uint8>(Map->GetBiomeAtCell(RelionCol, RelionRow)),
		static_cast<uint8>(ESolidBiome::Town));

	FSolidWorldBang Ignored;
	TestFalse(TEXT("an unknown town is not a mark"), Map->FindBang(UnknownTownCol, RelionRow, Ignored));
	TestEqual(TEXT("an unknown town is not a location"), Map->GetLocationIdAtCell(UnknownTownCol, RelionRow), INDEX_NONE);
	TestFalse(TEXT("an unknown type is not a mark"), Map->FindBang(UnknownTypeCol, RelionRow, Ignored));
	TestEqual(
		TEXT("an unknown type stays grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(UnknownTypeCol, RelionRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	FSolidWorldBang Cave;
	TestTrue(TEXT("MeagreDungeon is a stronghold"), Map->FindBang(CaveCol, CaveRow, Cave));
	TestEqual(TEXT("stronghold kind"), static_cast<uint8>(Cave.Kind), static_cast<uint8>(ESolidWorldBang::Stronghold));
	TestEqual(TEXT("stronghold size"), static_cast<uint8>(Cave.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("stronghold type"), static_cast<uint8>(Cave.StrongholdType), static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("stronghold name"), Cave.Name, FString(TEXT("Meagre Dungeon")));
	TestEqual(TEXT("a stronghold is not a location"), Map->GetLocationIdAtCell(CaveCol, CaveRow), INDEX_NONE);
	TestEqual(
		TEXT("a stronghold uses the mountain shelf"),
		static_cast<uint8>(Map->GetBiomeAtCell(CaveCol, CaveRow)),
		static_cast<uint8>(ESolidBiome::Mountain));

	FSolidWorldBang Tower;
	TestTrue(TEXT("a spaced size and type match"), Map->FindBang(TowerCol, TowerRow, Tower));
	TestEqual(TEXT("tower size"), static_cast<uint8>(Tower.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestEqual(TEXT("tower type"), static_cast<uint8>(Tower.StrongholdType), static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(
		TEXT("an unlisted tower keeps the marker biome"),
		static_cast<uint8>(Map->GetBiomeAtCell(TowerCol, TowerRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	FSolidWorldBang Imperious;
	TestTrue(TEXT("Imperious Tower is a stronghold"), Map->FindBang(ImperiousCol, ImperiousRow, Imperious));
	TestEqual(TEXT("imperious size"), static_cast<uint8>(Imperious.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(TEXT("imperious type"), static_cast<uint8>(Imperious.StrongholdType), static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(TEXT("imperious name"), Imperious.Name, FString(TEXT("Imperious Tower")));
	TestEqual(
		TEXT("Imperious Tower uses grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(ImperiousCol, ImperiousRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("an unknown stronghold is not a mark"), Map->FindBang(BadHoldCol, BadHoldRow, Ignored));
	TestEqual(
		TEXT("an unknown stronghold stays grassland"),
		static_cast<uint8>(Map->GetBiomeAtCell(BadHoldCol, BadHoldRow)),
		static_cast<uint8>(ESolidBiome::Grassland));

	FSolidWorldBang Entrance;
	TestTrue(TEXT("lair entrance"), Map->FindBang(EnterCol, EnterRow, Entrance));
	TestEqual(TEXT("entrance kind"), static_cast<uint8>(Entrance.Kind), static_cast<uint8>(ESolidWorldBang::LairEntrance));
	TestEqual(TEXT("entrance name"), Entrance.Name, FString(SolidLair::IglinTunnels.Name));
	TestEqual(
		TEXT("a lair entrance uses the mountain shelf"),
		static_cast<uint8>(Map->GetBiomeAtCell(EnterCol, EnterRow)),
		static_cast<uint8>(ESolidBiome::Mountain));

	FSolidWorldBang Exit;
	TestTrue(TEXT("lair exit"), Map->FindBang(ExitCol, ExitRow, Exit));
	TestEqual(TEXT("exit kind"), static_cast<uint8>(Exit.Kind), static_cast<uint8>(ESolidWorldBang::LairExit));
	TestEqual(TEXT("exit name"), Exit.Name, FString(SolidLair::IglinTunnels.Name));
	TestFalse(TEXT("an unknown lair is not a mark"), Map->FindBang(MissingLairCol, MissingLairRow, Ignored));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D RelionXY = FVector2D::ZeroVector;
	FVector2D CaveXY = FVector2D::ZeroVector;
	TestTrue(TEXT("Relion cell"), Map->CellToWorldXY(FIntPoint(RelionCol, RelionRow), WorldMin, WorldMax, RelionXY));
	TestTrue(TEXT("cave cell"), Map->CellToWorldXY(FIntPoint(CaveCol, CaveRow), WorldMin, WorldMax, CaveXY));
	TestTrue(TEXT("a town ! is a town"), Map->IsMarkedTown(RelionXY.X, RelionXY.Y, WorldMin, WorldMax));
	TestFalse(TEXT("a stronghold ! is not a town"), Map->IsMarkedTown(CaveXY.X, CaveXY.Y, WorldMin, WorldMax));

	TArray<SolidStrongholds::FEntrance> Mouths;
	TestEqual(TEXT("three stronghold mouths"), SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Mouths), 3);
	bool bFoundCave = false;
	bool bFoundTower = false;
	bool bFoundImperious = false;
	for (const SolidStrongholds::FEntrance& Mouth : Mouths)
	{
		if (Mouth.Cell == FIntPoint(CaveCol, CaveRow))
		{
			bFoundCave = true;
			TestTrue(TEXT("the cave mouth is the cliff mouth"), SolidStronghold::UsesCaveMouth(Mouth.Stronghold));
			TestEqual(
				TEXT("cave mouth size"),
				static_cast<uint8>(Mouth.Stronghold.Size),
				static_cast<uint8>(ESolidStrongholdSize::Meagre));
		}
		if (Mouth.Cell == FIntPoint(TowerCol, TowerRow))
		{
			bFoundTower = true;
			TestFalse(TEXT("an unlisted tower is not the cliff mouth"), SolidStronghold::UsesCaveMouth(Mouth.Stronghold));
			TestEqual(
				TEXT("tower mouth type"),
				static_cast<uint8>(Mouth.Stronghold.Type),
				static_cast<uint8>(ESolidStrongholdType::Tower));
		}
		if (Mouth.Cell == FIntPoint(ImperiousCol, ImperiousRow))
		{
			bFoundImperious = true;
			TestFalse(TEXT("Imperious Tower is not the cliff mouth"), SolidStronghold::UsesCaveMouth(Mouth.Stronghold));
			TestEqual(
				TEXT("imperious mouth size"),
				static_cast<uint8>(Mouth.Stronghold.Size),
				static_cast<uint8>(ESolidStrongholdSize::Imperious));
		}
	}
	TestTrue(TEXT("the cave mouth is placed"), bFoundCave);
	TestTrue(TEXT("the tower mouth is placed"), bFoundTower);
	TestTrue(TEXT("the imperious mouth is placed"), bFoundImperious);

	USolidWorldMap* Shipped = NewObject<USolidWorldMap>();
	TestTrue(TEXT("shipped map loads"), Shipped->LoadDefault());
	FSolidWorldBang ShippedIglin;
	TestTrue(TEXT("shipped Iglin !"), Shipped->FindBang(20, 7, ShippedIglin));
	TestEqual(TEXT("shipped Iglin id"), ShippedIglin.LocationId, 0);
	TestEqual(TEXT("shipped Iglin name"), ShippedIglin.Name, FString(TEXT("Iglin")));
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
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		FString Line;
		Line.Reserve(USolidWorldMap::MapWidth);
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			const bool bBang = (Row == HumbleRow && Col == HumbleCol)
				|| (Row == AverageRow && Col == AverageCol)
				|| (Row == ImperiousRow && Col == ImperiousCol)
				|| (Row == VastRow && Col == VastCol)
				|| (Row == JoinedRow && Col == JoinedCol)
				|| (Row == CaveRow && Col == CaveCol);
			Line.AppendChar(bBang ? USolidWorldMap::LocationMarker : USolidWorldMap::GrassMarker);
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

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestTrue(TEXT("a map with sites loads"), Map->LoadFromString(Text));

	FSolidWorldBang Humble;
	TestTrue(TEXT("a bracketed size is a site"), Map->FindBang(HumbleCol, HumbleRow, Humble));
	TestEqual(TEXT("site kind"), static_cast<uint8>(Humble.Kind), static_cast<uint8>(ESolidWorldBang::StrongholdSite));
	TestEqual(TEXT("site size"), static_cast<uint8>(Humble.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestEqual(TEXT("site name"), Humble.Name, FString(TEXT("Humble")));
	TestEqual(TEXT("a site is not a location"), Map->GetLocationIdAtCell(HumbleCol, HumbleRow), INDEX_NONE);
	TestEqual(
		TEXT("a site keeps the marker biome"),
		static_cast<uint8>(Map->GetBiomeAtCell(HumbleCol, HumbleRow)),
		static_cast<uint8>(ESolidBiome::Grassland));
	ESolidStrongholdSize ReadSize = ESolidStrongholdSize::Stately;
	TestTrue(TEXT("the site cell reports its size"), Map->StrongholdSiteAtCell(HumbleCol, HumbleRow, ReadSize));
	TestEqual(TEXT("reported size"), static_cast<uint8>(ReadSize), static_cast<uint8>(ESolidStrongholdSize::Humble));
	FSolidStronghold AsStronghold;
	TestFalse(TEXT("a site is not a stronghold"), Map->StrongholdAtCell(HumbleCol, HumbleRow, AsStronghold));

	FSolidWorldBang Average;
	TestTrue(TEXT("a size without brackets is a site"), Map->FindBang(AverageCol, AverageRow, Average));
	TestEqual(TEXT("unbracketed size"), static_cast<uint8>(Average.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Average));

	FSolidWorldBang Imperious;
	TestTrue(TEXT("the site word and the size ignore case"), Map->FindBang(ImperiousCol, ImperiousRow, Imperious));
	TestEqual(TEXT("imperious site"), static_cast<uint8>(Imperious.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Imperious));

	FSolidWorldBang Ignored;
	TestFalse(TEXT("an unknown size is not a mark"), Map->FindBang(VastCol, VastRow, Ignored));
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
	TestFalse(TEXT("a size and type is not a site"), Map->FindBang(JoinedCol, JoinedRow, Ignored));

	FSolidWorldBang Cave;
	TestTrue(TEXT("a stronghold line is still a stronghold"), Map->FindBang(CaveCol, CaveRow, Cave));
	TestEqual(TEXT("cave kind"), static_cast<uint8>(Cave.Kind), static_cast<uint8>(ESolidWorldBang::Stronghold));
	TestFalse(TEXT("a stronghold is not a site"), Map->StrongholdSiteAtCell(CaveCol, CaveRow, ReadSize));

	const FVector2D WorldMin(-6400.f, -6400.f);
	const FVector2D WorldMax(6400.f, 6400.f);
	FVector2D SiteXY = FVector2D::ZeroVector;
	TestTrue(TEXT("site cell"), Map->CellToWorldXY(FIntPoint(HumbleCol, HumbleRow), WorldMin, WorldMax, SiteXY));
	TestFalse(TEXT("a site is not a town"), Map->IsMarkedTown(SiteXY.X, SiteXY.Y, WorldMin, WorldMax));

	TArray<SolidStrongholds::FEntrance> Mouths;
	TestEqual(TEXT("a site adds no mouth"), SolidStrongholds::CollectEntrances(Map, WorldMin, WorldMax, Mouths), 1);
	TestTrue(TEXT("the mouth is the stronghold"), Mouths.Num() == 1 && Mouths[0].Cell == FIntPoint(CaveCol, CaveRow));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
