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
	FSolidWorldBang CaveBang;
	TestTrue(TEXT("the cave is the Meagre Dungeon mark"), DefaultMap->FindBang(Cave.X, Cave.Y, CaveBang));
	TestEqual(
		TEXT("the cave stays Meagre"),
		static_cast<uint8>(CaveBang.StrongholdSize),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(
		TEXT("the cave stays a Dungeon"),
		static_cast<uint8>(CaveBang.StrongholdType),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the cave row stays 9"), CaveBang.Cell.Y + 1, SolidStronghold::StartingRow);
	TestEqual(TEXT("the cave column stays 17"), CaveBang.Cell.X + 1, SolidStronghold::StartingColumn);

	FIntPoint Iglin;
	TestTrue(TEXT("Iglin stays location 0"), DefaultMap->TownCell(0, Iglin));
	FIntPoint Gate;
	TestTrue(TEXT("Dragongate stays one town"), DefaultMap->TownCell(SolidTowns::Dragongate, Gate));
	return true;
}

#endif
