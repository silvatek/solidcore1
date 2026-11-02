#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRiverShippedMapTest,
	"SolidCore1.River.ShippedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRiverShippedMapTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	TestTrue(TEXT("default map loads"), Map->LoadDefault());

	int32 RiverPoints = 0;
	for (const ESolidBiome Point : Map->Points)
	{
		if (Point == ESolidBiome::River)
		{
			++RiverPoints;
		}
	}

	TestTrue(TEXT("shipped map has a river"), RiverPoints > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
