#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidStronghold.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidShippedWorldChartTest,
	"SolidCore1.WorldMap.ShippedChartMatchesText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidShippedWorldChartTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	if (!TestNotNull(TEXT("/Game/World/WorldChart"), Chart))
	{
		return false;
	}

	USolidWorldChart* DefaultMap = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("LoadDefault loads the chart"), DefaultMap->LoadDefault()))
	{
		return false;
	}

	const FIntPoint Cave = SolidStronghold::StartingCell();
	const int32 CaveMark = DefaultMap->MarkAtCell(Cave);
	TestTrue(TEXT("the cave is the Meagre Dungeon mark"), CaveMark != INDEX_NONE);
	if (CaveMark != INDEX_NONE)
	{
		const FSolidWorldChartMark& Mark = DefaultMap->Marks[CaveMark];
		TestEqual(
			TEXT("the cave stays a stronghold"),
			static_cast<uint8>(Mark.Kind),
			static_cast<uint8>(ESolidWorldChartMarkKind::Stronghold));
		TestEqual(
			TEXT("the cave stays Meagre"),
			static_cast<uint8>(Mark.StrongholdSize),
			static_cast<uint8>(ESolidStrongholdSize::Meagre));
		TestEqual(
			TEXT("the cave stays a Dungeon"),
			static_cast<uint8>(Mark.StrongholdType),
			static_cast<uint8>(ESolidStrongholdType::Dungeon));
	}
	TestEqual(TEXT("the cave row stays 9"), Cave.Y + 1, SolidStronghold::StartingRow);
	TestEqual(TEXT("the cave column stays 17"), Cave.X + 1, SolidStronghold::StartingColumn);

	FIntPoint Iglin;
	TestTrue(TEXT("Iglin stays location 0"), DefaultMap->TownCell(0, Iglin));
	FIntPoint Gate;
	TestTrue(TEXT("Dragongate stays one town"), DefaultMap->TownCell(SolidTowns::Dragongate, Gate));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartPointsWithoutCellsLoadTest,
	"SolidCore1.WorldMap.PointsWithoutCellsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldChartPointsWithoutCellsLoadTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Source = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("point chart"), Source);
	Source->Init(ESolidBiome::Forest);
	Source->Cells.Reset();

	USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("loaded chart"), Loaded);
	TestTrue(TEXT("a chart with points and no cells loads"), Loaded->LoadFromChart(Source));
	TestTrue(TEXT("the loaded chart is solved"), Loaded->IsLoaded());
	TestEqual(
		TEXT("the point biome stays forest"),
		static_cast<uint8>(Loaded->GetPointBiome(FIntPoint(0, 0))),
		static_cast<uint8>(ESolidBiome::Forest));
	return true;
}

#endif
