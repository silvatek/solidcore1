#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool RiverTestBiome(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const ESolidBiome Actual,
		const ESolidBiome Expected)
	{
		return Test.TestEqual(What, static_cast<uint8>(Actual), static_cast<uint8>(Expected));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRiverShippedMapTest,
	"SolidCore1.River.ShippedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRiverShippedMapTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	TestTrue(TEXT("default map loads"), Map->LoadDefault());

	int32 RiverCells = 0;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (Map->GetMarkerAtCell(Col, Row) != SolidBiome::Of(ESolidBiome::River).Marker())
			{
				continue;
			}
			++RiverCells;
			RiverTestBiome(
				*this,
				*FString::Printf(TEXT("river cell (%d,%d) marker"), Col, Row),
				Map->GetBiomeAtCell(Col, Row),
				ESolidBiome::River);
		}
	}

	TestTrue(TEXT("shipped map has a river"), RiverCells > 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
