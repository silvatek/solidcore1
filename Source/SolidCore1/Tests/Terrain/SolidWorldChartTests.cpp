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
	int32 CaveX = 0;
	int32 CaveY = 0;
	USolidWorldChart::PointUnderCell(Cave.X, Cave.Y, CaveX, CaveY);
	int32 CaveMark = INDEX_NONE;
	for (int32 Index = 0; Index < DefaultMap->Marks.Num(); ++Index)
	{
		if (DefaultMap->Marks[Index].TerrainPoint == FIntPoint(CaveX, CaveY))
		{
			CaveMark = Index;
			break;
		}
	}
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

	FSolidWorldChartMark Iglin;
	TestTrue(TEXT("Iglin stays location 0"), DefaultMap->FindTownMarker(0, Iglin));
	FSolidWorldChartMark Gate;
	TestTrue(TEXT("Dragongate stays one town"), DefaultMap->FindTownMarker(SolidTowns::Dragongate, Gate));
	return true;
}

#endif
