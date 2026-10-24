#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidLair.h"
#include "SolidRoad.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D GridMin(-51200.f, -51200.f);
	const FVector2D GridMax(51200.f, 51200.f);

	FString GrassGrid()
	{
		FString Text;
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			Text += FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
			Text += TEXT("\n");
		}
		return Text;
	}

	void SetCell(FString& Grid, const int32 Col, const int32 Row, const TCHAR Marker)
	{
		Grid[Row * (USolidWorldChart::MapWidth + 1) + Col] = Marker;
	}

	ESolidBiome SampleAt(const USolidWorldChart& Chart, const FVector2D& World)
	{
		return Chart.SampleBiome(World.X, World.Y, GridMin, GridMax);
	}

	FVector2D CellCentre(const USolidWorldChart& Chart, const int32 Col, const int32 Row)
	{
		FVector2D Centre = FVector2D::ZeroVector;
		Chart.CellToWorldXY(FIntPoint(Col, Row), GridMin, GridMax, Centre);
		return Centre;
	}

	bool BiomeIs(
		FAutomationTestBase& Test,
		const TCHAR* What,
		const ESolidBiome Actual,
		const ESolidBiome Expected)
	{
		return Test.TestEqual(What, static_cast<uint8>(Actual), static_cast<uint8>(Expected));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridPointsDifferTest,
	"SolidCore1.TerrainGrid.PointsDiffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridPointsDifferTest::RunTest(const FString& Parameters)
{
	FString Pond = GrassGrid();
	SetCell(Pond, 10, 10, USolidWorldChart::RiverMarker);
	SetCell(Pond, 9, 10, TEXT('M'));
	USolidWorldChart* River = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("pond chart"), River) || !TestTrue(TEXT("pond loads"), River->LoadFromString(Pond)))
	{
		return false;
	}

	const FVector2D PondCentre = CellCentre(*River, 10, 10);
	BiomeIs(*this, TEXT("the pond centre is river"), SampleAt(*River, PondCentre), ESolidBiome::River);
	BiomeIs(*this, TEXT("the point east of the pond is mountain"), SampleAt(*River, PondCentre + FVector2D(400.f, 0.f)), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the pond cell stays river"), River->GetBiomeAtCell(10, 10), ESolidBiome::River);
	BiomeIs(*this, TEXT("the pond's centre point is river"), River->GetPointBiomeAtCell(10, 10), ESolidBiome::River);

	FString Mouth = GrassGrid();
	const int32 MouthCol = 10;
	const int32 MouthRow = 10;
	SetCell(Mouth, MouthCol, MouthRow, USolidWorldChart::LocationMarker);
	Mouth += TEXT("! 11,11 = LairEntrance (Iglin tunnels)\n");
	USolidWorldChart* Lair = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("mouth chart"), Lair) || !TestTrue(TEXT("mouth loads"), Lair->LoadFromString(Mouth)))
	{
		return false;
	}

	const FVector2D MouthCentre = CellCentre(*Lair, MouthCol, MouthRow);
	BiomeIs(*this, TEXT("the mouth cell is mountain"), Lair->GetBiomeAtCell(MouthCol, MouthRow), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the mouth point is grassland"), SampleAt(*Lair, MouthCentre), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the point beside the mouth stays mountain"), SampleAt(*Lair, MouthCentre + FVector2D(400.f, 0.f)), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the solver reads the mouth point"), Lair->GetPointBiomeAtCell(MouthCol, MouthRow), ESolidBiome::Grassland);
	TestTrue(
		TEXT("a trail can enter that grassland point"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, Lair->GetPointBiomeAtCell(MouthCol, MouthRow)) < SolidRoad::AvoidCost);
	TestTrue(
		TEXT("the mouth cell is still closed to a trail"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, Lair->GetBiomeAtCell(MouthCol, MouthRow)) >= SolidRoad::AvoidCost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridCellPaintTest,
	"SolidCore1.TerrainGrid.CellPaint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridCellPaintTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("grass chart"), Chart) || !TestTrue(TEXT("grass imports"), Chart->ImportFromString(GrassGrid())))
	{
		return false;
	}
	TestEqual(TEXT("import keeps a point for every terrain point"), Chart->Points.Num(), USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);

	const FIntPoint Cell(4, 6);
	const FIntPoint Beside(5, 6);
	FString Error;
	TestTrue(TEXT("the cell paints forest"), Chart->PaintRun({ Cell }, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the painted centre is forest"), SampleAt(*Chart, CellCentre(*Chart, Cell.X, Cell.Y)), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the next cell stays grassland"), SampleAt(*Chart, CellCentre(*Chart, Beside.X, Beside.Y)), ESolidBiome::Grassland);
	TestTrue(TEXT("undo restores the paint"), Chart->Undo());
	BiomeIs(*this, TEXT("the centre is grassland again"), SampleAt(*Chart, CellCentre(*Chart, Cell.X, Cell.Y)), ESolidBiome::Grassland);

	TestTrue(TEXT("river paint fills the cell"), Chart->PaintRun({ Cell }, ESolidBiome::River, Error));
	const FVector2D RiverCentre = CellCentre(*Chart, Cell.X, Cell.Y);
	BiomeIs(*this, TEXT("the painted centre is river"), SampleAt(*Chart, RiverCentre), ESolidBiome::River);
	BiomeIs(*this, TEXT("the point beside the centre is river"), SampleAt(*Chart, RiverCentre + FVector2D(400.f, 0.f)), ESolidBiome::River);
	BiomeIs(*this, TEXT("the next cell is still grassland"), SampleAt(*Chart, CellCentre(*Chart, Beside.X, Beside.Y)), ESolidBiome::Grassland);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridMarkerPointTest,
	"SolidCore1.TerrainGrid.MarkerPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridMarkerPointTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("grass chart"), Chart) || !TestTrue(TEXT("grass imports"), Chart->ImportFromString(GrassGrid())))
	{
		return false;
	}

	FString Error;
	const FIntPoint Town(10, 4);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTown(Town, TEXT("Iglin"), Error));
	const FVector2D TownCentre = CellCentre(*Chart, Town.X, Town.Y);
	BiomeIs(*this, TEXT("the town point is town"), SampleAt(*Chart, TownCentre), ESolidBiome::Town);
	BiomeIs(*this, TEXT("the rest of the town cell stays grassland"), SampleAt(*Chart, TownCentre + FVector2D(400.f, 0.f)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the town cell still resolves as town"), Chart->GetBiomeAtCell(Town.X, Town.Y), ESolidBiome::Town);
	BiomeIs(*this, TEXT("the solver reads the town point"), Chart->GetPointBiomeAtCell(Town.X, Town.Y), ESolidBiome::Town);

	TestTrue(TEXT("forest paint fills the town cell"), Chart->PaintRun({ Town }, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the town point stays town"), SampleAt(*Chart, TownCentre), ESolidBiome::Town);
	BiomeIs(*this, TEXT("the other town points take the paint"), SampleAt(*Chart, TownCentre + FVector2D(400.f, 0.f)), ESolidBiome::Forest);

	const FIntPoint Mouth(3, 3);
	TestTrue(TEXT("the mouth places"), Chart->PlaceLairMouth(Mouth, SolidLair::IglinTunnels.Name, true, Error));
	TestTrue(TEXT("desert paint fills the mouth cell"), Chart->PaintRun({ Mouth }, ESolidBiome::Desert, Error));
	const FVector2D MouthCentre = CellCentre(*Chart, Mouth.X, Mouth.Y);
	BiomeIs(*this, TEXT("the mouth point stays grassland"), SampleAt(*Chart, MouthCentre), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the other mouth points take the paint"), SampleAt(*Chart, MouthCentre + FVector2D(400.f, 0.f)), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("the mouth cell still resolves as mountain"), Chart->GetBiomeAtCell(Mouth.X, Mouth.Y), ESolidBiome::Mountain);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridRoadStripTest,
	"SolidCore1.TerrainGrid.RoadStrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridRoadStripTest::RunTest(const FString& Parameters)
{
	FString Grid = GrassGrid();
	SetCell(Grid, 4, 12, USolidWorldChart::LocationMarker);
	SetCell(Grid, 12, 12, USolidWorldChart::LocationMarker);
	Grid += TEXT("! 13,5 = Town Iglin\n! 13,13 = Town Relion\nRoad: Iglin -> Relion, trail\n");
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("trail chart"), Chart) || !TestTrue(TEXT("trail loads"), Chart->LoadFromString(Grid)))
	{
		return false;
	}

	int32 RoadPoints = 0;
	for (const ESolidBiome Point : Chart->Points)
	{
		if (Point == ESolidBiome::Road)
		{
			++RoadPoints;
		}
	}
	TestEqual(TEXT("the copy leaves the road strip off the points"), RoadPoints, 0);
	BiomeIs(*this, TEXT("the trail centre still samples as road"), SampleAt(*Chart, CellCentre(*Chart, 6, 12)), ESolidBiome::Road);
	BiomeIs(*this, TEXT("the point under the trail stays grassland"), Chart->GetPointBiomeAtCell(6, 12), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the town point stays town"), Chart->GetPointBiomeAtCell(4, 12), ESolidBiome::Town);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridShippedChartTest,
	"SolidCore1.TerrainGrid.ShippedChart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridShippedChartTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("shipped chart"), Chart) || !TestTrue(TEXT("the chart asset loads"), Chart->LoadDefault()))
	{
		return false;
	}

	TestEqual(
		TEXT("loading fills every terrain point"),
		Chart->Points.Num(),
		USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);
	int32 RoadPoints = 0;
	for (const ESolidBiome Point : Chart->Points)
	{
		if (Point == ESolidBiome::Road)
		{
			++RoadPoints;
		}
	}
	TestEqual(TEXT("the shipped copy has no road points"), RoadPoints, 0);

	FSolidWorldLocation Start;
	if (!TestTrue(TEXT("Iglin is on the chart"), Chart->FindLocation(SolidTowns::Iglin, Start)) || Start.Cells.Num() == 0)
	{
		return false;
	}
	const FIntPoint Town = Start.Cells[0];
	BiomeIs(*this, TEXT("the start town point is town"), Chart->GetPointBiomeAtCell(Town.X, Town.Y), ESolidBiome::Town);
	BiomeIs(*this, TEXT("the north-east sea point is sea"), Chart->GetPointBiomeAtCell(0, 0), ESolidBiome::Sea);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridTownRegionTest,
	"SolidCore1.TerrainGrid.TownRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridTownRegionTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("grass chart"), Chart) || !TestTrue(TEXT("grass imports"), Chart->ImportFromString(GrassGrid())))
	{
		return false;
	}

	FString Error;
	const FIntPoint Iglin(4, 4);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTown(Iglin, TEXT("Iglin"), Error));
	const FVector2D IglinCentre = CellCentre(*Chart, Iglin.X, Iglin.Y);
	TestEqual(TEXT("the marker point is Iglin"), Chart->SampleLocationId(IglinCentre.X, IglinCentre.Y, GridMin, GridMax), SolidTowns::Iglin);
	TestEqual(
		TEXT("grass beside the marker is not Iglin"),
		Chart->SampleLocationId(IglinCentre.X + 400.f, IglinCentre.Y, GridMin, GridMax),
		INDEX_NONE);
	TestEqual(TEXT("the cell is still Iglin"), Chart->GetLocationIdAtCell(Iglin.X, Iglin.Y), SolidTowns::Iglin);

	const FIntPoint Far(20, 20);
	TestTrue(TEXT("a far cell paints town"), Chart->PaintRun({ Far }, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("town paint without a marker is not a town"),
		Chart->SampleLocationId(CellCentre(*Chart, Far.X, Far.Y).X, CellCentre(*Chart, Far.X, Far.Y).Y, GridMin, GridMax),
		INDEX_NONE);

	const FIntPoint Relion(8, 8);
	const FIntPoint Second(9, 8);
	TestTrue(TEXT("Relion places first"), Chart->PlaceTown(Relion, TEXT("Relion"), Error));
	TestTrue(TEXT("Iglin places second"), Chart->PlaceTown(Second, TEXT("Iglin"), Error));
	TestTrue(TEXT("the two cells paint town"), Chart->PaintRectangle(Relion, Second, ESolidBiome::Town, Error));
	const FVector2D RelionCentre = CellCentre(*Chart, Relion.X, Relion.Y);
	const FVector2D SecondCentre = CellCentre(*Chart, Second.X, Second.Y);
	TestEqual(TEXT("the first marker keeps the region"), Chart->SampleLocationId(RelionCentre.X, RelionCentre.Y, GridMin, GridMax), SolidTowns::Relion);
	TestEqual(TEXT("the second marker stands in that region"), Chart->SampleLocationId(SecondCentre.X, SecondCentre.Y, GridMin, GridMax), SolidTowns::Relion);
	TestEqual(TEXT("the second cell still names Iglin"), Chart->GetLocationIdAtCell(Second.X, Second.Y), SolidTowns::Iglin);

	const FIntPoint GateEast(10, 12);
	const FIntPoint GateWest(11, 12);
	TestTrue(TEXT("Dragongate's east mark places"), Chart->PlaceTown(GateEast, TEXT("Dragongate"), Error));
	TestTrue(TEXT("Dragongate's west mark places"), Chart->PlaceTown(GateWest, TEXT("Dragongate"), Error));
	TestTrue(TEXT("Dragongate paints one region"), Chart->PaintRectangle(GateEast, GateWest, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("the east mark is Dragongate"),
		Chart->SampleLocationId(CellCentre(*Chart, GateEast.X, GateEast.Y).X, CellCentre(*Chart, GateEast.X, GateEast.Y).Y, GridMin, GridMax),
		SolidTowns::Dragongate);
	TestEqual(
		TEXT("the west mark is Dragongate"),
		Chart->SampleLocationId(CellCentre(*Chart, GateWest.X, GateWest.Y).X, CellCentre(*Chart, GateWest.X, GateWest.Y).Y, GridMin, GridMax),
		SolidTowns::Dragongate);

	USolidWorldChart* Shipped = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("the shipped chart loads"), Shipped && Shipped->LoadDefault()))
	{
		return false;
	}
	FSolidWorldLocation Gate;
	if (!TestTrue(TEXT("shipped Dragongate"), Shipped->FindLocation(SolidTowns::Dragongate, Gate))
		|| !TestEqual(TEXT("shipped Dragongate is two cells"), Gate.Cells.Num(), 2))
	{
		return false;
	}
	for (const FIntPoint& Cell : Gate.Cells)
	{
		const FVector2D Centre = CellCentre(*Shipped, Cell.X, Cell.Y);
		TestEqual(
			TEXT("standing on a Dragongate mark is Dragongate"),
			Shipped->SampleLocationId(Centre.X, Centre.Y, GridMin, GridMax),
			SolidTowns::Dragongate);
	}
	return true;
}

#endif
