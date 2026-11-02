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
			Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
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
		|| !TestTrue(TEXT("the chart covers the map"), Chart->Cells.Num() == USolidWorldChart::MapWidth * USolidWorldChart::MapHeight))
	{
		return false;
	}

	FString Error;
	const FIntPoint Cell(4, 6);
	TestFalse(TEXT("swamp is not a paint"), Chart->PaintRun({ Cell }, ESolidBiome::Swamp, Error));
	TestFalse(TEXT("the swamp error names the palette"), Error.IsEmpty());
	TestFalse(TEXT("road is not a paint"), Chart->PaintRun({ Cell }, ESolidBiome::Road, Error));
	TestEqual(
		TEXT("a rejected paint leaves grassland"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("one cell paints"), Chart->PaintRun({ Cell }, ESolidBiome::Forest, Error));
	TestEqual(
		TEXT("the cell is forest"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Forest));
	TestTrue(TEXT("undo restores the cell"), Chart->Undo());
	TestEqual(
		TEXT("the cell is grassland again"),
		static_cast<uint8>(Chart->Cells[Cell.Y * USolidWorldChart::MapWidth + Cell.X]),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestFalse(TEXT("a second undo has nothing to put back"), Chart->Undo());

	const TArray<FIntPoint> Run = { FIntPoint(1, 2), FIntPoint(3, 2), FIntPoint(8, 2) };
	TestTrue(TEXT("a drag is one paint"), Chart->PaintRun(Run, ESolidBiome::Desert, Error));
	TestEqual(TEXT("the run's first cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Desert));
	TestEqual(TEXT("a gap in the run stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("the run's last cell"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 8]), static_cast<uint8>(ESolidBiome::Desert));
	TestTrue(TEXT("one undo clears the whole run"), Chart->Undo());
	TestEqual(TEXT("the run is grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 1]), static_cast<uint8>(ESolidBiome::Grassland));

	TestTrue(TEXT("a rectangle paints"), Chart->PaintRectangle(FIntPoint(0, 0), FIntPoint(2, 1), ESolidBiome::Sea, Error));
	TestEqual(TEXT("the rectangle's far corner"), static_cast<uint8>(Chart->Cells[1 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Sea));
	TestEqual(TEXT("outside the rectangle stays grassland"), static_cast<uint8>(Chart->Cells[2 * USolidWorldChart::MapWidth + 2]), static_cast<uint8>(ESolidBiome::Grassland));
	int32 SeaCells = 0;
	for (int32 CellY = 0; CellY <= 1; ++CellY)
	{
		for (int32 CellX = 0; CellX <= 2; ++CellX)
		{
			if (Chart->Cells[CellY * USolidWorldChart::MapWidth + CellX] == ESolidBiome::Sea)
			{
				++SeaCells;
			}
		}
	}
	TestEqual(TEXT("the rectangle is six cells"), SeaCells, 6);
	TestTrue(TEXT("one undo clears the rectangle"), Chart->Undo());
	return true;
}

#endif
