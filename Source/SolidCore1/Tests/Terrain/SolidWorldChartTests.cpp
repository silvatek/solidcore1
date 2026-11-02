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

	int32 CaveMark = INDEX_NONE;
	for (int32 Index = 0; Index < DefaultMap->Marks.Num(); ++Index)
	{
		const FSolidWorldChartMark& Mark = DefaultMap->Marks[Index];
		if (Mark.Kind == ESolidWorldChartMarkKind::Stronghold
			&& Mark.StrongholdSize == ESolidStrongholdSize::Meagre
			&& Mark.StrongholdType == ESolidStrongholdType::Dungeon)
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

	FSolidWorldChartMark Iglin;
	TestTrue(TEXT("Iglin stays location 0"), DefaultMap->FindTownMarker(0, Iglin));
	FSolidWorldChartMark Gate;
	TestTrue(TEXT("Dragongate stays one town"), DefaultMap->FindTownMarker(SolidTowns::Dragongate, Gate));
	return true;
}

#endif
