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

	USolidWorldChart* GrassChart()
	{
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		if (!Chart)
		{
			return nullptr;
		}
		Chart->Init(ESolidBiome::Grassland);
		Chart->LoadFromChart(Chart);
		return Chart;
	}

	ESolidBiome SampleAt(const USolidWorldChart& Chart, const FVector2D& World)
	{
		return Chart.SampleBiome(World.X, World.Y, GridMin, GridMax);
	}

	FIntPoint GridCellPoint(const FIntPoint Cell)
	{
		int32 X = 0;
		int32 Y = 0;
		USolidWorldChart::PointUnderCell(Cell.X, Cell.Y, X, Y);
		return FIntPoint(X, Y);
	}

	void GridPointsUnderCells(const TArray<FIntPoint>& Cells, TArray<FIntPoint>& OutPoints)
	{
		OutPoints.Reset();
		for (int32 TerrainY = 0; TerrainY < USolidWorldChart::PointAxis; ++TerrainY)
		{
			for (int32 TerrainX = 0; TerrainX < USolidWorldChart::PointAxis; ++TerrainX)
			{
				const FIntPoint TerrainPoint(TerrainX, TerrainY);
				FIntPoint Cell = FIntPoint::ZeroValue;
				if (USolidWorldChart::CellAtTerrainPoint(TerrainPoint, Cell) && Cells.Contains(Cell))
				{
					OutPoints.Add(TerrainPoint);
				}
			}
		}
	}

	FVector2D CellCentre(const USolidWorldChart& Chart, const int32 Col, const int32 Row)
	{
		FVector2D Centre = FVector2D::ZeroVector;
		Chart.CellToWorldXY(FIntPoint(Col, Row), GridMin, GridMax, Centre);
		return Centre;
	}

	FIntPoint PointAt(const USolidWorldChart& Chart, const FVector2D& World)
	{
		FIntPoint TerrainPoint = FIntPoint::ZeroValue;
		Chart.TerrainPointAt(World.X, World.Y, GridMin, GridMax, TerrainPoint);
		return TerrainPoint;
	}

	FVector2D PointWorld(const USolidWorldChart& Chart, const FIntPoint& TerrainPoint)
	{
		FVector2D World = FVector2D::ZeroVector;
		Chart.TerrainPointToWorld(TerrainPoint, GridMin, GridMax, World);
		return World;
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
	USolidWorldChart* River = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("pond chart"), River))
	{
		return false;
	}
	River->Init(ESolidBiome::Grassland);
	const FIntPoint PondPoint(256, 256);
	const FVector2D PondCentre = PointWorld(*River, PondPoint);
	const FIntPoint Bank = PointAt(*River, PondCentre + FVector2D(400.f, 0.f));
	FString Error;
	if (!TestTrue(TEXT("the pond point paints river"), River->PaintPointRectangle(PondPoint, PondPoint, ESolidBiome::River, Error)))
	{
		return false;
	}
	if (!TestTrue(TEXT("the bank point paints mountain"), River->PaintPointRectangle(Bank, Bank, ESolidBiome::Mountain, Error)))
	{
		return false;
	}
	BiomeIs(*this, TEXT("the pond centre is river"), SampleAt(*River, PondCentre), ESolidBiome::River);
	BiomeIs(*this, TEXT("the point east of the pond is mountain"), SampleAt(*River, PondCentre + FVector2D(400.f, 0.f)), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the pond's centre point is river"), River->GetPointBiome(PondPoint), ESolidBiome::River);

	USolidWorldChart* Lair = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("mouth chart"), Lair))
	{
		return false;
	}
	Lair->Init(ESolidBiome::Grassland);
	const FIntPoint MouthPoint(200, 200);
	FSolidWorldChartMark MouthMark;
	MouthMark.Kind = ESolidWorldChartMarkKind::LairEntrance;
	MouthMark.Name = SolidLair::IglinTunnels.Name;
	MouthMark.LairId = SolidLair::IdIglinTunnels;
	MouthMark.TerrainPoint = MouthPoint;
	Lair->Marks.Add(MouthMark);

	const FVector2D MouthCentre = PointWorld(*Lair, MouthPoint);
	BiomeIs(*this, TEXT("the mouth point stays grassland"), SampleAt(*Lair, MouthCentre), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the point beside the mouth stays grassland"), SampleAt(*Lair, MouthCentre + FVector2D(400.f, 0.f)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the solver reads the mouth point"), Lair->GetPointBiome(MouthPoint), ESolidBiome::Grassland);
	TestFalse(
		TEXT("a trail can enter the mouth point"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, Lair->GetPointBiome(MouthPoint)) == SolidRoad::BannedStep);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridCellPaintTest,
	"SolidCore1.TerrainGrid.CellPaint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridCellPaintTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = GrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}
	TestEqual(TEXT("the chart has a point for every terrain point"), Chart->Points.Num(), USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);

	const FIntPoint Cell(4, 6);
	const FIntPoint Beside(5, 6);
	FString Error;
	TArray<FIntPoint> Covered;
	GridPointsUnderCells({ Cell }, Covered);
	TestTrue(TEXT("the cell paints forest"), Chart->PaintPoints(Covered, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the painted centre is forest"), SampleAt(*Chart, CellCentre(*Chart, Cell.X, Cell.Y)), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the next cell stays grassland"), SampleAt(*Chart, CellCentre(*Chart, Beside.X, Beside.Y)), ESolidBiome::Grassland);
	TestTrue(TEXT("undo restores the paint"), Chart->Undo());
	BiomeIs(*this, TEXT("the centre is grassland again"), SampleAt(*Chart, CellCentre(*Chart, Cell.X, Cell.Y)), ESolidBiome::Grassland);

	GridPointsUnderCells({ Cell }, Covered);
	TestTrue(TEXT("river paint fills the cell"), Chart->PaintPoints(Covered, ESolidBiome::River, Error));
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
	USolidWorldChart* Chart = GrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}

	FString Error;
	const FIntPoint Town(10, 4);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTownAt(GridCellPoint(Town), TEXT("Iglin"), Error));
	const FVector2D TownCentre = CellCentre(*Chart, Town.X, Town.Y);
	BiomeIs(*this, TEXT("the town point stays grassland"), SampleAt(*Chart, TownCentre), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the rest of the town cell stays grassland"), SampleAt(*Chart, TownCentre + FVector2D(400.f, 0.f)), ESolidBiome::Grassland);
	FSolidWorldChartMark TownMark;
	FIntPoint TownCell = FIntPoint::ZeroValue;
	TestTrue(TEXT("the town mark remains"), Chart->FindTownMarker(SolidTowns::Iglin, TownMark));
	TestTrue(TEXT("the town mark is a cell"), USolidWorldChart::CellAtTerrainPoint(TownMark.TerrainPoint, TownCell));
	TestEqual(TEXT("the town mark stands on its cell"), TownCell, Town);
	BiomeIs(*this, TEXT("the solver reads the town point"), Chart->GetPointBiomeAtCell(Town.X, Town.Y), ESolidBiome::Grassland);

	TArray<FIntPoint> TownPoints;
	GridPointsUnderCells({ Town }, TownPoints);
	TestTrue(TEXT("forest paint fills the town cell"), Chart->PaintPoints(TownPoints, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the town point takes the forest"), SampleAt(*Chart, TownCentre), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the other town points take the paint"), SampleAt(*Chart, TownCentre + FVector2D(400.f, 0.f)), ESolidBiome::Forest);

	const FIntPoint Mouth(3, 3);
	TestTrue(TEXT("the mouth places"), Chart->PlaceLairMouthAt(GridCellPoint(Mouth), SolidLair::IglinTunnels.Name, true, Error));
	TArray<FIntPoint> MouthPoints;
	GridPointsUnderCells({ Mouth }, MouthPoints);
	TestTrue(TEXT("desert paint fills the mouth cell"), Chart->PaintPoints(MouthPoints, ESolidBiome::Desert, Error));
	const FVector2D MouthCentre = CellCentre(*Chart, Mouth.X, Mouth.Y);
	BiomeIs(*this, TEXT("the mouth point takes the desert"), SampleAt(*Chart, MouthCentre), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("the other mouth points take the paint"), SampleAt(*Chart, MouthCentre + FVector2D(400.f, 0.f)), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("the mouth point under the cell is desert"), Chart->GetPointBiomeAtCell(Mouth.X, Mouth.Y), ESolidBiome::Desert);
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
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridTownRegionTest,
	"SolidCore1.TerrainGrid.TownRegion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridTownRegionTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = GrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}

	FString Error;
	const FIntPoint Iglin(4, 4);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTownAt(GridCellPoint(Iglin), TEXT("Iglin"), Error));
	const FVector2D IglinCentre = CellCentre(*Chart, Iglin.X, Iglin.Y);
	TestEqual(TEXT("an unpainted marker point is not Iglin"), Chart->SampleLocationId(IglinCentre.X, IglinCentre.Y, GridMin, GridMax), INDEX_NONE);
	TestEqual(
		TEXT("grass beside the marker is not Iglin"),
		Chart->SampleLocationId(IglinCentre.X + 400.f, IglinCentre.Y, GridMin, GridMax),
		INDEX_NONE);
	FSolidWorldChartMark IglinMark;
	FIntPoint IglinCell = FIntPoint::ZeroValue;
	TestTrue(TEXT("the Iglin mark remains"), Chart->FindTownMarker(SolidTowns::Iglin, IglinMark));
	TestTrue(TEXT("the Iglin mark is a cell"), USolidWorldChart::CellAtTerrainPoint(IglinMark.TerrainPoint, IglinCell));
	TestEqual(TEXT("the mark stays on its cell"), IglinCell, Iglin);

	const FIntPoint Far(20, 20);
	TArray<FIntPoint> FarPoints;
	GridPointsUnderCells({ Far }, FarPoints);
	TestTrue(TEXT("a far cell paints town"), Chart->PaintPoints(FarPoints, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("town paint without a marker is not a town"),
		Chart->SampleLocationId(CellCentre(*Chart, Far.X, Far.Y).X, CellCentre(*Chart, Far.X, Far.Y).Y, GridMin, GridMax),
		INDEX_NONE);

	const FIntPoint Relion(8, 8);
	const FIntPoint Second(9, 8);
	TestTrue(TEXT("Relion places first"), Chart->PlaceTownAt(GridCellPoint(Relion), TEXT("Relion"), Error));
	TestTrue(TEXT("Iglin places second"), Chart->PlaceTownAt(GridCellPoint(Second), TEXT("Iglin"), Error));
	TArray<FIntPoint> TownPoints;
	GridPointsUnderCells({ Relion, Second }, TownPoints);
	TestTrue(TEXT("the two cells paint town"), Chart->PaintPoints(TownPoints, ESolidBiome::Town, Error));
	const FVector2D RelionCentre = CellCentre(*Chart, Relion.X, Relion.Y);
	const FVector2D SecondCentre = CellCentre(*Chart, Second.X, Second.Y);
	TestEqual(TEXT("the first marker keeps the region"), Chart->SampleLocationId(RelionCentre.X, RelionCentre.Y, GridMin, GridMax), SolidTowns::Relion);
	TestEqual(TEXT("the second marker stands in that region"), Chart->SampleLocationId(SecondCentre.X, SecondCentre.Y, GridMin, GridMax), SolidTowns::Relion);
	TestTrue(TEXT("the Iglin mark remains"), Chart->FindTownMarker(SolidTowns::Iglin, IglinMark));
	TestTrue(TEXT("the Iglin mark is a cell"), USolidWorldChart::CellAtTerrainPoint(IglinMark.TerrainPoint, IglinCell));
	TestEqual(TEXT("the Iglin mark stands on the second cell"), IglinCell, Second);

	const FIntPoint GateEast(10, 12);
	const FIntPoint GateWest(11, 12);
	TestTrue(TEXT("Dragongate's east mark places"), Chart->PlaceTownAt(GridCellPoint(GateEast), TEXT("Dragongate"), Error));
	TestTrue(TEXT("Dragongate's west mark places"), Chart->PlaceTownAt(GridCellPoint(GateWest), TEXT("Dragongate"), Error));
	TArray<FIntPoint> GatePoints;
	GridPointsUnderCells({ GateEast, GateWest }, GatePoints);
	TestTrue(TEXT("Dragongate paints one region"), Chart->PaintPoints(GatePoints, ESolidBiome::Town, Error));
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
	FIntPoint GateCell;
	if (!TestTrue(TEXT("shipped Dragongate"), Shipped->TownCell(SolidTowns::Dragongate, GateCell)))
	{
		return false;
	}
	const FVector2D Centre = CellCentre(*Shipped, GateCell.X, GateCell.Y);
	TestEqual(
		TEXT("standing on a Dragongate mark is Dragongate"),
		Shipped->SampleLocationId(Centre.X, Centre.Y, GridMin, GridMax),
		SolidTowns::Dragongate);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainGridPointBrushTest,
	"SolidCore1.TerrainGrid.PointBrush",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainGridPointBrushTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = GrassChart();
	if (!TestNotNull(TEXT("grass chart"), Chart))
	{
		return false;
	}

	const FIntPoint Cell(30, 20);
	const FVector2D Centre = CellCentre(*Chart, Cell.X, Cell.Y);
	const FIntPoint Origin = PointAt(*Chart, Centre);
	const FIntPoint East = PointAt(*Chart, Centre + FVector2D(200.f, 0.f));
	const FIntPoint North = PointAt(*Chart, Centre + FVector2D(0.f, 200.f));
	TestEqual(TEXT("one step east is the next point"), East.X, Origin.X + 1);
	TestEqual(TEXT("east stays on the row"), East.Y, Origin.Y);
	TestEqual(TEXT("one step north is the next point"), North.Y, Origin.Y + 1);
	TestEqual(TEXT("north stays on the column"), North.X, Origin.X);

	FString Error;
	Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
	const int32 CellIndex = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
	TestTrue(TEXT("one point stamps forest"), Chart->PaintBrush({ Origin }, 0, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the stamped point is forest"), Chart->GetPointBiome(Origin), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("play reads that point"), SampleAt(*Chart, Centre), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the next point stays grassland"), Chart->GetPointBiome(East), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the cell array stays grassland"), Chart->Cells[CellIndex], ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the centre point is forest"), Chart->GetPointBiomeAtCell(Cell.X, Cell.Y), ESolidBiome::Forest);
	TestTrue(TEXT("undo restores the stamp"), Chart->Undo());
	BiomeIs(*this, TEXT("the stamp is grassland again"), Chart->GetPointBiome(Origin), ESolidBiome::Grassland);

	TestTrue(TEXT("a disc stamps"), Chart->PaintBrush({ Origin }, 2, ESolidBiome::Mountain, Error));
	BiomeIs(*this, TEXT("two steps east is inside the disc"), Chart->GetPointBiome(Origin + FIntPoint(2, 0)), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the diagonal step is inside the disc"), Chart->GetPointBiome(Origin + FIntPoint(1, 1)), ESolidBiome::Mountain);
	BiomeIs(*this, TEXT("the square corner is outside the disc"), Chart->GetPointBiome(Origin + FIntPoint(2, 1)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("three steps east is outside the disc"), Chart->GetPointBiome(Origin + FIntPoint(3, 0)), ESolidBiome::Grassland);
	TestTrue(TEXT("undo restores the disc"), Chart->Undo());

	const FIntPoint DiagonalEnd = Origin + FIntPoint(3, 3);
	TestTrue(TEXT("a diagonal is one stroke"), Chart->PaintBrush({ Origin, DiagonalEnd }, 0, ESolidBiome::Desert, Error));
	for (int32 Step = 0; Step <= 3; ++Step)
	{
		BiomeIs(*this, TEXT("the diagonal point is desert"), Chart->GetPointBiome(Origin + FIntPoint(Step, Step)), ESolidBiome::Desert);
	}
	BiomeIs(*this, TEXT("the point beside the diagonal stays grassland"), Chart->GetPointBiome(Origin + FIntPoint(1, 0)), ESolidBiome::Grassland);
	TestTrue(TEXT("one undo clears the diagonal"), Chart->Undo());
	for (int32 Step = 0; Step <= 3; ++Step)
	{
		BiomeIs(*this, TEXT("the diagonal point is grassland again"), Chart->GetPointBiome(Origin + FIntPoint(Step, Step)), ESolidBiome::Grassland);
	}

	const FIntPoint Bend = Origin + FIntPoint(4, 0);
	const FIntPoint CurveEnd = Origin + FIntPoint(4, 3);
	TestTrue(TEXT("a curve is one stroke"), Chart->PaintBrush({ Origin, Bend, CurveEnd }, 0, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the curve's east run is forest"), Chart->GetPointBiome(Origin + FIntPoint(2, 0)), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the curve's north run is forest"), Chart->GetPointBiome(Origin + FIntPoint(4, 2)), ESolidBiome::Forest);
	BiomeIs(*this, TEXT("the shortcut across the curve stays grassland"), Chart->GetPointBiome(Origin + FIntPoint(2, 2)), ESolidBiome::Grassland);
	TestTrue(TEXT("undo restores the curve"), Chart->Undo());

	const FIntPoint RectEnd = Origin + FIntPoint(2, 1);
	TestTrue(TEXT("the rectangle fills"), Chart->PaintPointRectangle(Origin, RectEnd, ESolidBiome::Sea, Error));
	int32 SeaPoints = 0;
	for (int32 Y = 0; Y <= 1; ++Y)
	{
		for (int32 X = 0; X <= 2; ++X)
		{
			if (Chart->GetPointBiome(Origin + FIntPoint(X, Y)) == ESolidBiome::Sea)
			{
				++SeaPoints;
			}
		}
	}
	TestEqual(TEXT("the rectangle is the inclusive points"), SeaPoints, 6);
	BiomeIs(*this, TEXT("the point east of the rectangle stays grassland"), Chart->GetPointBiome(Origin + FIntPoint(3, 0)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the point north of the rectangle stays grassland"), Chart->GetPointBiome(Origin + FIntPoint(0, 2)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the centre point is sea"), Chart->GetPointBiomeAtCell(Cell.X, Cell.Y), ESolidBiome::Sea);
	TestTrue(TEXT("undo restores the rectangle"), Chart->Undo());

	TestTrue(TEXT("river paint is one point"), Chart->PaintBrush({ Origin }, 0, ESolidBiome::River, Error));
	BiomeIs(*this, TEXT("the painted point is river"), SampleAt(*Chart, Centre), ESolidBiome::River);
	BiomeIs(*this, TEXT("the next point is not a channel"), SampleAt(*Chart, PointWorld(*Chart, East)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the centre point is river"), Chart->GetPointBiomeAtCell(Cell.X, Cell.Y), ESolidBiome::River);
	TestTrue(TEXT("undo restores the river point"), Chart->Undo());

	TestFalse(TEXT("swamp is not a point paint"), Chart->PaintBrush({ Origin }, 0, ESolidBiome::Swamp, Error));
	TestFalse(TEXT("road is not a point paint"), Chart->PaintBrush({ Origin }, 0, ESolidBiome::Road, Error));
	BiomeIs(*this, TEXT("a rejected paint leaves the point"), Chart->GetPointBiome(Origin), ESolidBiome::Grassland);
	TestFalse(TEXT("the rejected paints were not undo steps"), Chart->Undo());

	const FIntPoint Town(10, 4);
	const FIntPoint TownPoint = PointAt(*Chart, CellCentre(*Chart, Town.X, Town.Y));
	const FIntPoint TownBeside = TownPoint + FIntPoint(1, 0);
	const FIntPoint TownDiagonal = TownPoint + FIntPoint(1, 1);
	TestTrue(TEXT("Iglin places"), Chart->PlaceTownAt(GridCellPoint(Town), TEXT("Iglin"), Error));
	TestTrue(TEXT("forest covers the town point"), Chart->PaintBrush({ TownPoint }, 0, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the town point takes the forest"), Chart->GetPointBiome(TownPoint), ESolidBiome::Forest);
	TestEqual(
		TEXT("standing on the forest point is not Iglin"),
		Chart->SampleLocationId(CellCentre(*Chart, Town.X, Town.Y).X, CellCentre(*Chart, Town.X, Town.Y).Y, GridMin, GridMax),
		INDEX_NONE);
	TestTrue(TEXT("the marker point paints town"), Chart->PaintBrush({ TownPoint }, 0, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("standing on the town point is Iglin"),
		Chart->SampleLocationId(CellCentre(*Chart, Town.X, Town.Y).X, CellCentre(*Chart, Town.X, Town.Y).Y, GridMin, GridMax),
		SolidTowns::Iglin);
	TestTrue(TEXT("the diagonal point paints town"), Chart->PaintBrush({ TownDiagonal }, 0, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("town paint that does not meet the marker is not Iglin"),
		Chart->SampleLocationId(PointWorld(*Chart, TownDiagonal).X, PointWorld(*Chart, TownDiagonal).Y, GridMin, GridMax),
		INDEX_NONE);
	TestTrue(TEXT("the point beside the marker paints town"), Chart->PaintBrush({ TownBeside }, 0, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("town paint connected to the marker is Iglin"),
		Chart->SampleLocationId(PointWorld(*Chart, TownBeside).X, PointWorld(*Chart, TownBeside).Y, GridMin, GridMax),
		SolidTowns::Iglin);

	const FIntPoint Far = PointAt(*Chart, CellCentre(*Chart, 40, 30));
	TestTrue(TEXT("a lone point paints town"), Chart->PaintBrush({ Far }, 0, ESolidBiome::Town, Error));
	TestEqual(
		TEXT("a lone town point is not a town"),
		Chart->SampleLocationId(PointWorld(*Chart, Far).X, PointWorld(*Chart, Far).Y, GridMin, GridMax),
		INDEX_NONE);

	const FIntPoint Hold(14, 4);
	const FIntPoint HoldPoint = PointAt(*Chart, CellCentre(*Chart, Hold.X, Hold.Y));
	TestTrue(
		TEXT("the cave places"),
		Chart->PlaceStrongholdAt(GridCellPoint(Hold), ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon, Error));
	TestTrue(TEXT("sea covers the cave"), Chart->PaintBrush({ HoldPoint }, 1, ESolidBiome::Sea, Error));
	BiomeIs(*this, TEXT("the cave point takes the sea"), Chart->GetPointBiome(HoldPoint), ESolidBiome::Sea);
	BiomeIs(*this, TEXT("the point beside the cave takes the sea"), Chart->GetPointBiome(HoldPoint + FIntPoint(1, 0)), ESolidBiome::Sea);
	BiomeIs(*this, TEXT("the diagonal beside the cave stays grassland"), Chart->GetPointBiome(HoldPoint + FIntPoint(1, 1)), ESolidBiome::Grassland);

	const FIntPoint Site(2, 8);
	const FIntPoint SitePoint = PointAt(*Chart, CellCentre(*Chart, Site.X, Site.Y));
	TestTrue(TEXT("a site places"), Chart->PlaceSiteAt(GridCellPoint(Site), ESolidStrongholdSize::Humble, Error));
	TestTrue(TEXT("mountain covers the site"), Chart->PaintBrush({ SitePoint }, 0, ESolidBiome::Mountain, Error));
	BiomeIs(*this, TEXT("the site point takes the mountain"), Chart->GetPointBiome(SitePoint), ESolidBiome::Mountain);

	const FIntPoint Mouth(3, 3);
	const FIntPoint MouthTerrainPoint = PointAt(*Chart, CellCentre(*Chart, Mouth.X, Mouth.Y));
	TestTrue(TEXT("the mouth places"), Chart->PlaceLairMouthAt(GridCellPoint(Mouth), SolidLair::IglinTunnels.Name, true, Error));
	TestTrue(TEXT("desert covers the mouth"), Chart->PaintBrush({ MouthTerrainPoint }, 2, ESolidBiome::Desert, Error));
	BiomeIs(*this, TEXT("the mouth point takes the desert"), Chart->GetPointBiome(MouthTerrainPoint), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("two steps from the mouth are desert"), Chart->GetPointBiome(MouthTerrainPoint + FIntPoint(2, 0)), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("the diagonal step from the mouth is desert"), Chart->GetPointBiome(MouthTerrainPoint + FIntPoint(1, 1)), ESolidBiome::Desert);
	BiomeIs(*this, TEXT("the square corner by the mouth stays grassland"), Chart->GetPointBiome(MouthTerrainPoint + FIntPoint(2, 1)), ESolidBiome::Grassland);
	TestTrue(TEXT("undo restores the mouth paint"), Chart->Undo());
	BiomeIs(*this, TEXT("the mouth ring is grassland again"), Chart->GetPointBiome(MouthTerrainPoint + FIntPoint(2, 0)), ESolidBiome::Grassland);
	BiomeIs(*this, TEXT("the mouth point is grassland again"), Chart->GetPointBiome(MouthTerrainPoint), ESolidBiome::Grassland);

	const FIntPoint Exit(8, 10);
	const FIntPoint ExitPoint = PointAt(*Chart, CellCentre(*Chart, Exit.X, Exit.Y));
	TestTrue(TEXT("the exit places"), Chart->PlaceLairMouthAt(GridCellPoint(Exit), SolidLair::IglinTunnels.Name, false, Error));
	TestTrue(TEXT("sea covers the exit"), Chart->PaintBrush({ ExitPoint }, 0, ESolidBiome::Sea, Error));
	BiomeIs(*this, TEXT("the exit point takes the sea"), Chart->GetPointBiome(ExitPoint), ESolidBiome::Sea);

	const FIntPoint Note(6, 6);
	const FIntPoint NotePoint = PointAt(*Chart, CellCentre(*Chart, Note.X, Note.Y));
	TestTrue(TEXT("a note places"), Chart->PlaceCustomAt(GridCellPoint(Note), TEXT("Lookout"), Error));
	TestTrue(TEXT("forest covers the note"), Chart->PaintBrush({ NotePoint }, 0, ESolidBiome::Forest, Error));
	BiomeIs(*this, TEXT("the note point takes the forest"), Chart->GetPointBiome(NotePoint), ESolidBiome::Forest);
	return true;
}

#endif
