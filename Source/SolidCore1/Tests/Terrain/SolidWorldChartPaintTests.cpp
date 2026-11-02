#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	USolidWorldChart* PaintGrassChart()
	{
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		if (Chart)
		{
			Chart->Init(ESolidBiome::Grassland);
		}
		return Chart;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldChartPaintTest,
	"SolidCore1.WorldChart.Paint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSolidWorldChartPaintTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = PaintGrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart)
		|| !TestEqual(TEXT("the chart covers the point grid"), Chart->Points.Num(), USolidWorldChart::PointAxis * USolidWorldChart::PointAxis))
	{
		return false;
	}

	FString Error;
	const FIntPoint Point(40, 60);
	TestFalse(TEXT("swamp is not a paint"), Chart->PaintPoints({ Point }, ESolidBiome::Swamp, Error));
	TestFalse(TEXT("the swamp error names the palette"), Error.IsEmpty());
	TestFalse(TEXT("road is not a paint"), Chart->PaintPoints({ Point }, ESolidBiome::Road, Error));
	TestEqual(
		TEXT("a rejected paint leaves grassland"),
		static_cast<uint8>(Chart->GetPointBiome(Point)),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("one point paints"), Chart->PaintPoints({ Point }, ESolidBiome::Forest, Error));
	TestEqual(
		TEXT("the point is forest"),
		static_cast<uint8>(Chart->GetPointBiome(Point)),
		static_cast<uint8>(ESolidBiome::Forest));
	TestTrue(TEXT("undo restores the point"), Chart->Undo());
	TestEqual(
		TEXT("the point is grassland again"),
		static_cast<uint8>(Chart->GetPointBiome(Point)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("a second undo has nothing to put back"), Chart->Undo());

	const TArray<FIntPoint> Run = { FIntPoint(10, 20), FIntPoint(30, 20), FIntPoint(80, 20) };
	TestTrue(TEXT("a drag is one paint"), Chart->PaintPoints(Run, ESolidBiome::Desert, Error));
	TestEqual(TEXT("the run's first point"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(10, 20))), static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("a gap in the run stays grassland"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(20, 20))), static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the run's last point"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(80, 20))), static_cast<uint8>(ESolidBiome::Desert));
	TestTrue(TEXT("one undo clears the whole run"), Chart->Undo());
	TestEqual(TEXT("the run is grassland"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(10, 20))), static_cast<uint8>(ESolidBiome::Grassland));

	TArray<FIntPoint> Rectangle;
	USolidWorldChart::PointRectangle(FIntPoint(0, 0), FIntPoint(2, 1), Rectangle);
	TestTrue(TEXT("a rectangle paints"), Chart->PaintPoints(Rectangle, ESolidBiome::Sea, Error));
	TestEqual(TEXT("the rectangle's far corner"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(2, 1))), static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("outside the rectangle stays grassland"), static_cast<uint8>(Chart->GetPointBiome(FIntPoint(2, 2))), static_cast<uint8>(ESolidBiome::Grassland));
	int32 SeaPoints = 0;
	for (int32 TerrainY = 0; TerrainY <= 1; ++TerrainY)
	{
		for (int32 TerrainX = 0; TerrainX <= 2; ++TerrainX)
		{
			if (Chart->GetPointBiome(FIntPoint(TerrainX, TerrainY)) == ESolidBiome::Sea)
			{
				++SeaPoints;
			}
		}
	}
	TestEqual(TEXT("the rectangle is six points"), SeaPoints, 6);
	TestTrue(TEXT("one undo clears the rectangle"), Chart->Undo());
	return true;
}

#endif
