#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
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

	const FVector2D FogOrigin = Terrain->GetFogOriginXY();
	TestTrue(TEXT("fog origin matches start town X"),
		FMath::IsNearlyEqual(FogOrigin.X, TownXY.X, 1.f));
	TestTrue(TEXT("fog origin matches start town Y"),
		FMath::IsNearlyEqual(FogOrigin.Y, TownXY.Y, 1.f));

	const FSolidTerrainPoint TownPoint = Terrain->SamplePoint(TownXY.X, TownXY.Y);
	TestEqual(
		TEXT("town centroid is Town biome"),
		static_cast<uint8>(TownPoint.Biome),
		static_cast<uint8>(ESolidBiome::Town));
	TestTrue(TEXT("town starts clear of fog"), FMath::IsNearlyEqual(TownPoint.Fog, 0.f));

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
	TestTrue(TEXT("default sea is blue"), Sea.B > 0.5f);
	TestTrue(TEXT("default river is blue"), River.B > 0.5f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
