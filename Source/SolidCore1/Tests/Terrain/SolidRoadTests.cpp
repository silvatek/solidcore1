#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRoad.h"
#include "SolidRoads.h"
#include "SolidStronghold.h"
#include "SolidTerrainTypes.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D RoadTestMin(-51200.f, -51200.f);
	const FVector2D RoadTestMax(51200.f, 51200.f);

	bool RoadTestOrthogonal(FAutomationTestBase& Test, const TArray<FIntPoint>& Cells)
	{
		for (int32 Index = 1; Index < Cells.Num(); ++Index)
		{
			const int32 Steps = FMath::Max(
				FMath::Abs(Cells[Index].X - Cells[Index - 1].X),
				FMath::Abs(Cells[Index].Y - Cells[Index - 1].Y));
			if (!Test.TestEqual(TEXT("route steps to a neighbour"), Steps, 1))
			{
				return false;
			}
		}
		return true;
	}

	FVector2D RoadTestCenter(const USolidWorldChart& Map, const int32 Col, const int32 Row)
	{
		FVector2D Center = FVector2D::ZeroVector;
		Map.CellToWorldXY(FIntPoint(Col, Row), RoadTestMin, RoadTestMax, Center);
		return Center;
	}

	bool SurfaceChart(FAutomationTestBase& Test, USolidWorldChart*& OutChart)
	{
		OutChart = NewObject<USolidWorldChart>();
		if (!Test.TestNotNull(TEXT("chart"), OutChart))
		{
			return false;
		}
		OutChart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
		OutChart->Points.Init(ESolidBiome::Grassland, USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);
		return true;
	}

	bool SurfaceLoad(FAutomationTestBase& Test, USolidWorldChart* Chart, USolidWorldChart*& OutMap, const int32 Roads)
	{
		USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
		if (!Test.TestNotNull(TEXT("loaded chart"), Loaded)
			|| !Test.TestTrue(TEXT("the chart loads"), Loaded->LoadFromChart(Chart)))
		{
			return false;
		}
		OutMap = Loaded;
		return Test.TestEqual(TEXT("routed roads"), Loaded->GetRoutedRoads().Num(), Roads);
	}

	bool SurfacePoint(FAutomationTestBase& Test, USolidWorldChart& Chart, const FIntPoint Cell, FIntPoint& OutPoint)
	{
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
		USolidWorldChart::DefaultWorldBounds(Min, Max);
		FVector2D Centre = FVector2D::ZeroVector;
		return Test.TestTrue(TEXT("the cell has a centre"), Chart.CellToWorldXY(Cell, Min, Max, Centre))
			&& Test.TestTrue(TEXT("the cell sits on a point"), Chart.TerrainPointAt(Centre.X, Centre.Y, Min, Max, OutPoint));
	}

	void FillGrassPoints(USolidWorldChart& Chart)
	{
		Chart.Points.Init(ESolidBiome::Grassland, USolidWorldChart::PointAxis * USolidWorldChart::PointAxis);
	}

	bool PlacePointMark(USolidWorldChart& Chart, FSolidWorldChartMark Mark, const FIntPoint Point)
	{
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
		USolidWorldChart::DefaultWorldBounds(Min, Max);
		if (!Chart.TerrainPointToWorld(Point, Min, Max, Mark.PositionCm))
		{
			return false;
		}
		Chart.Marks.Add(Mark);
		return true;
	}

	bool RouteStaysOnRow(const TArray<FIntPoint>& Points, const int32 Row)
	{
		if (Points.Num() < 2)
		{
			return false;
		}
		for (const FIntPoint& Point : Points)
		{
			if (Point.Y != Row)
			{
				return false;
			}
		}
		return true;
	}

	int32 RouteBiomeCount(const USolidWorldChart& Chart, const TArray<FIntPoint>& Points, const ESolidBiome Biome)
	{
		int32 Count = 0;
		for (const FIntPoint& Point : Points)
		{
			if (Chart.GetPointBiome(Point) == Biome)
			{
				++Count;
			}
		}
		return Count;
	}

	bool RouteGoesAround(FAutomationTestBase& Test, const ESolidBiome Patch, const SolidRoad::EType Type)
	{
		const int32 Row = 256;
		const FIntPoint IglinPoint(100, Row);
		const FIntPoint RelionPoint(400, Row);
		USolidWorldChart* Chart = NewObject<USolidWorldChart>();
		if (!Test.TestNotNull(TEXT("patch chart"), Chart))
		{
			return false;
		}
		FillGrassPoints(*Chart);
		if (!Test.TestTrue(TEXT("Iglin places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Iglin), IglinPoint))
			|| !Test.TestTrue(TEXT("Relion places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Relion), RelionPoint))
			|| !Test.TestTrue(TEXT("the patch paints"), Chart->SetPointRectangle(FIntPoint(249, Row - 1), FIntPoint(251, Row + 1), Patch)))
		{
			return false;
		}

		SolidRoad::FSolidRoad Road(
			SolidRoad::TownMark(SolidTowns::Iglin),
			SolidRoad::TownMark(SolidTowns::Relion),
			Type);
		if (!Test.TestTrue(TEXT("both ends are on the chart"), Road.Locate(Chart->Marks))
			|| !Test.TestTrue(TEXT("the route solves"), Road.FindRoute(*Chart)))
		{
			return false;
		}
		Test.TestEqual(TEXT("the route starts at Iglin"), Road.Points[0], IglinPoint);
		Test.TestEqual(TEXT("the route ends at Relion"), Road.Points.Last(), RelionPoint);
		Test.TestEqual(TEXT("the route stays off the patch"), RouteBiomeCount(*Chart, Road.Points, Patch), 0);
		return Test.TestFalse(TEXT("the route leaves the straight row"), RouteStaysOnRow(Road.Points, Row));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadWidthTest,
	"SolidCore1.Road.Widths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadWidthTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("trail is 1 m"), FMath::IsNearlyEqual(SolidRoad::WidthCm(SolidRoad::EType::Trail), 100.f));
	TestTrue(TEXT("track is 2 m"), FMath::IsNearlyEqual(SolidRoad::WidthCm(SolidRoad::EType::Track), 200.f));
	TestTrue(TEXT("road is 3 m"), FMath::IsNearlyEqual(SolidRoad::WidthCm(SolidRoad::EType::Road), 300.f));
	TestTrue(TEXT("highway is 4 m"), FMath::IsNearlyEqual(SolidRoad::WidthCm(SolidRoad::EType::Highway), 400.f));
	TestEqual(TEXT("grass costs one step"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::Grassland), SolidRoad::StepCost);
	TestEqual(TEXT("a town costs nothing"),
		SolidRoad::EnterCost(SolidRoad::EType::Highway, ESolidBiome::Town), 0);
	TestEqual(TEXT("a road biome costs nothing"),
		SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::Road), 0);
	TestEqual(TEXT("a trail pays 8 to enter forest"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::Forest), 8);
	TestEqual(TEXT("a road pays 16 to enter forest"),
		SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::Forest), 16);
	TestEqual(TEXT("a highway pays 32 to enter desert"),
		SolidRoad::EnterCost(SolidRoad::EType::Highway, ESolidBiome::Desert), 32);
	TestEqual(TEXT("a road pays 16 to enter swamp"),
		SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::Swamp), 16);
	TestEqual(TEXT("a mountain is banned"),
		SolidRoad::EnterCost(SolidRoad::EType::Track, ESolidBiome::Mountain), SolidRoad::BannedStep);
	TestEqual(TEXT("the sea is banned"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::Sea), SolidRoad::BannedStep);
	TestEqual(TEXT("a trail pays 16 to cross a river"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::River, true), 16);
	TestEqual(TEXT("a road pays 32 to cross a river"),
		SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::River, true), 32);
	TestEqual(TEXT("a highway pays 64 to cross a river"),
		SolidRoad::EnterCost(SolidRoad::EType::Highway, ESolidBiome::River, true), 64);
	TestEqual(TEXT("a step along the river is banned"),
		SolidRoad::EnterCost(SolidRoad::EType::Track, ESolidBiome::River, false), SolidRoad::BannedStep);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadNamedEndTest,
	"SolidCore1.Road.NamedEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadNamedEndTest::RunTest(const FString& Parameters)
{
	const FIntPoint CavePoint(80, 200);
	const FIntPoint TownPoint(160, 200);
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("named map"), Map);
	FillGrassPoints(*Map);
	if (!TestTrue(TEXT("the cave places"), PlacePointMark(*Map, SolidRoad::StrongholdMark(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon), CavePoint))
		|| !TestTrue(TEXT("Iglin places"), PlacePointMark(*Map, SolidRoad::TownMark(SolidTowns::Iglin), TownPoint)))
	{
		return false;
	}

	SolidRoad::FSolidRoad Trail(
		SolidRoad::StrongholdMark(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon),
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::EType::Trail);
	if (!TestTrue(TEXT("both ends are on the chart"), Trail.Locate(Map->Marks))
		|| !TestTrue(TEXT("the trail solves"), Trail.FindRoute(*Map)))
	{
		return false;
	}
	TestEqual(TEXT("the trail starts on the cave"), Trail.Points[0], CavePoint);
	TestEqual(TEXT("the trail ends on Iglin"), Trail.Points.Last(), TownPoint);
	return TestTrue(TEXT("the trail stays on that row"), RouteStaysOnRow(Trail.Points, CavePoint.Y));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadForestHighwayTest,
	"SolidCore1.Road.ForestHighway",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadForestHighwayTest::RunTest(const FString& Parameters)
{
	const int32 Row = 256;
	const FIntPoint IglinPoint(100, Row);
	const FIntPoint RelionPoint(400, Row);
	const FIntPoint ForestPoint(250, Row);
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("forest chart"), Chart);
	FillGrassPoints(*Chart);
	if (!TestTrue(TEXT("Iglin places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Iglin), IglinPoint))
		|| !TestTrue(TEXT("Relion places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Relion), RelionPoint)))
	{
		return false;
	}

	const FIntPoint WallTop(ForestPoint.X, 0);
	const FIntPoint WallBottom(ForestPoint.X, USolidWorldChart::PointAxis - 1);
	const FIntPoint Gap(ForestPoint.X, 40);
	if (!TestTrue(TEXT("the forest wall paints"), Chart->SetPointRectangle(WallTop, WallBottom, ESolidBiome::Forest))
		|| !TestTrue(TEXT("the gap stays grass"), Chart->SetPointRectangle(Gap, Gap, ESolidBiome::Grassland)))
	{
		return false;
	}

	SolidRoad::FSolidRoad Highway(
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::TownMark(SolidTowns::Relion),
		SolidRoad::EType::Highway);
	if (!TestTrue(TEXT("both ends are on the chart"), Highway.Locate(Chart->Marks))
		|| !TestTrue(TEXT("the highway solves"), Highway.FindRoute(*Chart)))
	{
		return false;
	}
	TestTrue(TEXT("highway stays on the straight row"), RouteStaysOnRow(Highway.Points, Row));
	int32 ForestPoints = 0;
	for (const FIntPoint& Point : Highway.Points)
	{
		if (Chart->GetPointBiome(Point) == ESolidBiome::Forest)
		{
			++ForestPoints;
		}
	}
	TestEqual(TEXT("highway crosses one forest point"), ForestPoints, 1);
	return TestTrue(TEXT("that point is the wall"), Highway.Points.Contains(ForestPoint));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadForestTrailTest,
	"SolidCore1.Road.ForestTrail",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadForestTrailTest::RunTest(const FString& Parameters)
{
	const int32 Row = 256;
	const FIntPoint IglinPoint(100, Row);
	const FIntPoint RelionPoint(400, Row);
	const FIntPoint ForestPoint(250, Row);
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("forest chart"), Chart);
	FillGrassPoints(*Chart);
	if (!TestTrue(TEXT("Iglin places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Iglin), IglinPoint))
		|| !TestTrue(TEXT("Relion places"), PlacePointMark(*Chart, SolidRoad::TownMark(SolidTowns::Relion), RelionPoint)))
	{
		return false;
	}

	const FIntPoint WallTop(ForestPoint.X, 0);
	const FIntPoint WallBottom(ForestPoint.X, USolidWorldChart::PointAxis - 1);
	const FIntPoint Gap(ForestPoint.X, 40);
	if (!TestTrue(TEXT("the forest wall paints"), Chart->SetPointRectangle(WallTop, WallBottom, ESolidBiome::Forest))
		|| !TestTrue(TEXT("the gap stays grass"), Chart->SetPointRectangle(Gap, Gap, ESolidBiome::Grassland)))
	{
		return false;
	}

	SolidRoad::FSolidRoad Trail(
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::TownMark(SolidTowns::Relion),
		SolidRoad::EType::Trail);
	if (!TestTrue(TEXT("both ends are on the chart"), Trail.Locate(Chart->Marks))
		|| !TestTrue(TEXT("the trail solves"), Trail.FindRoute(*Chart)))
	{
		return false;
	}
	TestTrue(TEXT("trail stays on the straight row"), RouteStaysOnRow(Trail.Points, Row));
	int32 ForestPoints = 0;
	for (const FIntPoint& Point : Trail.Points)
	{
		if (Chart->GetPointBiome(Point) == ESolidBiome::Forest)
		{
			++ForestPoints;
		}
	}
	TestEqual(TEXT("trail crosses one forest point"), ForestPoints, 1);
	return TestTrue(TEXT("that point is the wall"), Trail.Points.Contains(ForestPoint));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadMountainPatchTest,
	"SolidCore1.Road.MountainPatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadMountainPatchTest::RunTest(const FString& Parameters)
{
	return RouteGoesAround(*this, ESolidBiome::Mountain, SolidRoad::EType::Track);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadSeaPatchTest,
	"SolidCore1.Road.SeaPatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadSeaPatchTest::RunTest(const FString& Parameters)
{
	return RouteGoesAround(*this, ESolidBiome::Sea, SolidRoad::EType::Trail);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadRiverTest,
	"SolidCore1.Road.River",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadRiverTest::RunTest(const FString& Parameters)
{
	const int32 Row = 256;
	const int32 RiverX = 250;
	const FIntPoint IglinPoint(100, Row);
	const FIntPoint RelionPoint(400, Row);
	const int32 LastPoint = USolidWorldChart::PointAxis - 1;

	auto Prepare = [this, IglinPoint, RelionPoint, RiverX, LastPoint](USolidWorldChart& Chart, const bool bGap) -> bool
	{
		FillGrassPoints(Chart);
		if (!TestTrue(TEXT("Iglin places"), PlacePointMark(Chart, SolidRoad::TownMark(SolidTowns::Iglin), IglinPoint))
			|| !TestTrue(TEXT("Relion places"), PlacePointMark(Chart, SolidRoad::TownMark(SolidTowns::Relion), RelionPoint))
			|| !TestTrue(TEXT("the river paints"), Chart.SetPointRectangle(FIntPoint(RiverX, 0), FIntPoint(RiverX, LastPoint), ESolidBiome::River)))
		{
			return false;
		}
		if (!bGap)
		{
			return true;
		}
		return TestTrue(TEXT("the river leaves a gap"),
			Chart.SetPointRectangle(FIntPoint(RiverX, 400), FIntPoint(RiverX, 402), ESolidBiome::Grassland));
	};

	auto Solve = [this, IglinPoint, RelionPoint](const USolidWorldChart& Chart, const SolidRoad::EType Type, SolidRoad::FSolidRoad& Road) -> bool
	{
		Road = SolidRoad::FSolidRoad(
			SolidRoad::TownMark(SolidTowns::Iglin),
			SolidRoad::TownMark(SolidTowns::Relion),
			Type);
		return TestTrue(TEXT("both ends are on the chart"), Road.Locate(Chart.Marks))
			&& TestTrue(TEXT("the route solves"), Road.FindRoute(Chart))
			&& TestEqual(TEXT("the route starts at Iglin"), Road.Points[0], IglinPoint)
			&& TestEqual(TEXT("the route ends at Relion"), Road.Points.Last(), RelionPoint);
	};

	USolidWorldChart* Gapped = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("gapped map"), Gapped);
	if (!Gapped || !Prepare(*Gapped, true))
	{
		return false;
	}

	SolidRoad::FSolidRoad Road(
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::TownMark(SolidTowns::Relion),
		SolidRoad::EType::Road);
	if (!Solve(*Gapped, SolidRoad::EType::Road, Road))
	{
		return false;
	}
	TestEqual(TEXT("a road crosses the river once"), RouteBiomeCount(*Gapped, Road.Points, ESolidBiome::River), 1);

	SolidRoad::FSolidRoad Track(
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::TownMark(SolidTowns::Relion),
		SolidRoad::EType::Track);
	if (!Solve(*Gapped, SolidRoad::EType::Track, Track))
	{
		return false;
	}
	TestTrue(TEXT("track crosses the river"), RouteBiomeCount(*Gapped, Track.Points, ESolidBiome::River) >= 1);

	USolidWorldChart* Blocked = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("blocked map"), Blocked);
	if (!Blocked || !Prepare(*Blocked, false))
	{
		return false;
	}
	SolidRoad::FSolidRoad Forced(
		SolidRoad::TownMark(SolidTowns::Iglin),
		SolidRoad::TownMark(SolidTowns::Relion),
		SolidRoad::EType::Road);
	if (!Solve(*Blocked, SolidRoad::EType::Road, Forced))
	{
		return false;
	}
	TestEqual(TEXT("a blocking river is crossed once"), RouteBiomeCount(*Blocked, Forced.Points, ESolidBiome::River), 1);
	TestEqual(TEXT("the crossing stays off the sea"), RouteBiomeCount(*Blocked, Forced.Points, ESolidBiome::Sea), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadSurfaceTest,
	"SolidCore1.Road.Surface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadSurfaceTest::RunTest(const FString& Parameters)
{
	const FLinearColor RoadBrown = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Road);
	const FLinearColor TownBrown = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Town);
	TestTrue(TEXT("road brown sits near the town brown"),
		RoadBrown.R > RoadBrown.G && RoadBrown.G > RoadBrown.B
		&& TownBrown.R > TownBrown.G && TownBrown.G > TownBrown.B
		&& FMath::Abs(RoadBrown.R - TownBrown.R) < 0.2f);

	FString Error;
	USolidWorldChart* TrackChart = nullptr;
	USolidWorldChart* Track = nullptr;
	FIntPoint ForestPoint = FIntPoint::ZeroValue;
	FIntPoint RiverPoint = FIntPoint::ZeroValue;
	if (!SurfaceChart(*this, TrackChart)
		|| !TestTrue(TEXT("Iglin places"), TrackChart->PlaceTown(FIntPoint(4, 12), TEXT("Iglin"), Error))
		|| !TestTrue(TEXT("Relion places"), TrackChart->PlaceTown(FIntPoint(16, 12), TEXT("Relion"), Error))
		|| !SurfacePoint(*this, *TrackChart, FIntPoint(8, 12), ForestPoint)
		|| !TestTrue(TEXT("forest paints on the track"), TrackChart->SetPointRectangle(ForestPoint, ForestPoint, ESolidBiome::Forest))
		|| !SurfacePoint(*this, *TrackChart, FIntPoint(12, 12), RiverPoint)
		|| !TestTrue(TEXT("the river paints"), TrackChart->SetPointRectangle(
			FIntPoint(RiverPoint.X, 0), FIntPoint(RiverPoint.X, USolidWorldChart::PointAxis - 1), ESolidBiome::River))
		|| !SurfaceLoad(*this, TrackChart, Track, 1))
	{
		return false;
	}

	const FVector2D Grass = RoadTestCenter(*Track, 6, 12);
	const FVector2D Forest = RoadTestCenter(*Track, 8, 12);
	const float TrackHalf = SolidRoad::WidthCm(SolidRoad::EType::Track) * 0.5f;
	TestEqual(TEXT("the cell under the track stays grass"),
		static_cast<uint8>(Track->GetBiomeAtCell(6, 12)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("track centre uses the road texture"),
		static_cast<uint8>(Track->SampleBiome(Grass.X, Grass.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("track rim is road"),
		static_cast<uint8>(Track->SampleBiome(Grass.X, Grass.Y + TrackHalf - 10.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("past the track is grass"),
		static_cast<uint8>(Track->SampleBiome(Grass.X, Grass.Y + TrackHalf + 10.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("forest on the track uses the road texture"),
		static_cast<uint8>(Track->SampleBiome(Forest.X, Forest.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestFalse(TEXT("grass under a track is not a town"),
		Track->IsMarkedTown(Grass.X, Grass.Y, RoadTestMin, RoadTestMax));
	const FVector2D Iglin = RoadTestCenter(*Track, 4, 12);
	TestTrue(TEXT("Iglin is a town"),
		Track->IsMarkedTown(Iglin.X, Iglin.Y, RoadTestMin, RoadTestMax));

	const FVector2D River = RoadTestCenter(*Track, 12, 12);
	TestEqual(TEXT("the channel stays river"),
		static_cast<uint8>(Track->SampleBiome(River.X, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(TEXT("the bank of a crossing uses the road texture"),
		static_cast<uint8>(Track->SampleBiome(River.X + 400.f, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));

	USolidWorldChart* CaveChart = nullptr;
	USolidWorldChart* Cave = nullptr;
	if (!SurfaceChart(*this, CaveChart)
		|| !TestTrue(TEXT("the cave places"), CaveChart->PlaceStronghold(
			FIntPoint(4, 8), ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon, Error))
		|| !TestTrue(TEXT("Iglin places by the cave"), CaveChart->PlaceTown(FIntPoint(8, 8), TEXT("Iglin"), Error))
		|| !SurfaceLoad(*this, CaveChart, Cave, 1))
	{
		return false;
	}
	const FVector2D CaveCell = RoadTestCenter(*Cave, 4, 8);
	const FSolidStronghold MeagreDungeon(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre);
	ESolidBiome CaveShelf = ESolidBiome::Grassland;
	TestTrue(TEXT("the cave names a shelf"), SolidStronghold::BiomeFor(MeagreDungeon, CaveShelf));
	TestEqual(TEXT("the cave keeps its shelf"),
		static_cast<uint8>(Cave->SampleBiome(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(CaveShelf));
	TestFalse(TEXT("the cave is not a town"),
		Cave->IsMarkedTown(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax));

	USolidWorldChart* HighwayChart = nullptr;
	USolidWorldChart* Highway = nullptr;
	if (!SurfaceChart(*this, HighwayChart)
		|| !TestTrue(TEXT("Relion places"), HighwayChart->PlaceTown(FIntPoint(8, 20), TEXT("Relion"), Error))
		|| !TestTrue(TEXT("Kanfold places"), HighwayChart->PlaceTown(FIntPoint(16, 20), TEXT("Kanfold"), Error))
		|| !SurfaceLoad(*this, HighwayChart, Highway, 1))
	{
		return false;
	}
	const FVector2D Wide = RoadTestCenter(*Highway, 12, 20);
	const float HighwayHalf = SolidRoad::WidthCm(SolidRoad::EType::Highway) * 0.5f;
	TestEqual(TEXT("inside a highway"),
		static_cast<uint8>(Highway->SampleBiome(Wide.X, Wide.Y + HighwayHalf - 10.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("outside a highway"),
		static_cast<uint8>(Highway->SampleBiome(Wide.X, Wide.Y + HighwayHalf + 20.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Grassland));

	USolidWorldChart* LoneChart = nullptr;
	USolidWorldChart* Lone = nullptr;
	if (!SurfaceChart(*this, LoneChart)
		|| !TestTrue(TEXT("Iglin places alone"), LoneChart->PlaceTown(FIntPoint(4, 4), TEXT("Iglin"), Error))
		|| !SurfaceLoad(*this, LoneChart, Lone, 0))
	{
		return false;
	}
	TestEqual(TEXT("a road needs both ends"), Lone->GetRoads().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadShippedTest,
	"SolidCore1.Road.ShippedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadShippedTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("map"), Map);
	TestTrue(TEXT("default map loads"), Map->LoadDefault());

	const TArray<SolidRoad::FRoute>& Roads = Map->GetRoads();
	TestEqual(TEXT("five roads"), Roads.Num(), 5);
	if (Roads.Num() < 5)
	{
		return false;
	}

	const SolidRoad::EType Types[] = {
		SolidRoad::EType::Trail,
		SolidRoad::EType::Track,
		SolidRoad::EType::Highway,
		SolidRoad::EType::Road,
		SolidRoad::EType::Track,
	};
	const int32 FromIds[] = { INDEX_NONE, 0, 1, 1, 5 };
	const int32 ToIds[] = { 0, 1, 2, 5, 4 };
	for (int32 Index = 0; Index < 5; ++Index)
	{
		TestEqual(TEXT("road type"), static_cast<uint8>(Roads[Index].Definition.Type), static_cast<uint8>(Types[Index]));
		TestEqual(TEXT("road starts where the line says"), Roads[Index].Definition.From.LocationId, FromIds[Index]);
		TestEqual(TEXT("road ends where the line says"), Roads[Index].Definition.To.LocationId, ToIds[Index]);
		TestTrue(TEXT("route has cells"), Roads[Index].Cells.Num() >= 2);
		RoadTestOrthogonal(*this, Roads[Index].Cells);
	}

	TestTrue(TEXT("the trail starts at a stronghold"), Roads[0].Definition.From.bStronghold);
	TestEqual(TEXT("the trail starts at 9,17"), Roads[0].Definition.From.Cell, FIntPoint(16, 8));
	TestEqual(
		TEXT("the trail starts on the cave"),
		Map->GetMarkerAtCell(Roads[0].Cells[0].X, Roads[0].Cells[0].Y),
		USolidWorldChart::LocationMarker);
	TestEqual(TEXT("trail ends at Iglin"), Map->GetLocationIdAtCell(Roads[0].Cells.Last().X, Roads[0].Cells.Last().Y), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadDefinitionsTest,
	"SolidCore1.Road.Definitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadDefinitionsTest::RunTest(const FString& Parameters)
{
	const int32 FromIds[] = {
		SolidTowns::Iglin,
		SolidTowns::Relion,
		SolidTowns::Relion,
		SolidTowns::Dragongate,
	};
	const int32 ToIds[] = {
		SolidTowns::Relion,
		SolidTowns::Kanfold,
		SolidTowns::Dragongate,
		SolidTowns::Jethan,
	};
	const SolidRoad::EType Types[] = {
		SolidRoad::EType::Track,
		SolidRoad::EType::Highway,
		SolidRoad::EType::Road,
		SolidRoad::EType::Track,
	};
	const int32 TownRoads = UE_ARRAY_COUNT(FromIds);
	TestEqual(TEXT("five roads"), SolidRoad::DefinitionCount, TownRoads + 1);
	if (SolidRoad::DefinitionCount < TownRoads + 1)
	{
		return false;
	}

	const SolidRoad::FSolidRoad& Cave = SolidRoad::Roads[0];
	TestEqual(TEXT("the trail starts at a stronghold"), static_cast<uint8>(Cave.From.Kind), static_cast<uint8>(ESolidWorldChartMarkKind::Stronghold));
	TestEqual(TEXT("the trail starts at Meagre"), static_cast<uint8>(Cave.From.StrongholdSize), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("the trail starts at a dungeon"), static_cast<uint8>(Cave.From.StrongholdType), static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the trail starts at Meagre Dungeon"), Cave.From.Name, SolidStronghold::Label(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the trail ends at a town"), static_cast<uint8>(Cave.To.Kind), static_cast<uint8>(ESolidWorldChartMarkKind::Town));
	TestEqual(TEXT("the trail ends at Iglin"), Cave.To.Name, FString(SolidTowns::Definitions[SolidTowns::Iglin].Name));
	TestEqual(TEXT("trail"), static_cast<uint8>(Cave.Type), static_cast<uint8>(SolidRoad::EType::Trail));

	for (int32 Index = 0; Index < TownRoads; ++Index)
	{
		const SolidRoad::FSolidRoad& Road = SolidRoad::Roads[Index + 1];
		TestEqual(TEXT("starts at a town"), static_cast<uint8>(Road.From.Kind), static_cast<uint8>(ESolidWorldChartMarkKind::Town));
		TestEqual(TEXT("ends at a town"), static_cast<uint8>(Road.To.Kind), static_cast<uint8>(ESolidWorldChartMarkKind::Town));
		TestEqual(TEXT("start town"), Road.From.Name, FString(SolidTowns::Definitions[FromIds[Index]].Name));
		TestEqual(TEXT("end town"), Road.To.Name, FString(SolidTowns::Definitions[ToIds[Index]].Name));
		TestEqual(TEXT("road type"), static_cast<uint8>(Road.Type), static_cast<uint8>(Types[Index]));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadLocationsTest,
	"SolidCore1.Road.Locations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadLocationsTest::RunTest(const FString& Parameters)
{
	auto Placed = [](const TArray<FSolidWorldChartMark>& ChartMarks, const FSolidWorldChartMark& End)
	{
		for (const FSolidWorldChartMark& ChartMark : ChartMarks)
		{
			if (ChartMark.PositionCm.Equals(End.PositionCm) && ChartMark.Kind == End.Kind)
			{
				return true;
			}
		}
		return false;
	};

	FSolidWorldChartMark Iglin = SolidRoad::TownMark(SolidTowns::Iglin);
	Iglin.PositionCm = FVector2D(1000.f, 2000.f);
	FSolidWorldChartMark Cave = SolidRoad::StrongholdMark(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon);
	Cave.Name = TEXT("MeagreDungeon");
	Cave.PositionCm = FVector2D(3000.f, 4000.f);
	TArray<FSolidWorldChartMark> Marks;
	Marks.Add(Iglin);
	Marks.Add(Cave);

	SolidRoad::FSolidRoad Trail = SolidRoad::Roads[0];
	Trail.Locate(Marks);
	TestTrue(TEXT("the cave takes the chart place"), Trail.From.PositionCm.Equals(Cave.PositionCm));
	TestTrue(TEXT("Iglin takes the chart place"), Trail.To.PositionCm.Equals(Iglin.PositionCm));
	TestTrue(TEXT("the list itself stays unplaced"), SolidRoad::Roads[0].From.PositionCm.IsNearlyZero());

	FSolidWorldChartMark Missing = SolidRoad::TownMark(SolidTowns::Relion);
	SolidRoad::LocateMark(Marks, Missing);
	TestTrue(TEXT("a missing end stays unplaced"), Missing.PositionCm.IsNearlyZero());

	const USolidWorldChart* Chart = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	if (!TestNotNull(TEXT("shipped chart"), Chart))
	{
		return false;
	}
	for (const SolidRoad::FSolidRoad& Definition : SolidRoad::Roads)
	{
		SolidRoad::FSolidRoad Road = Definition;
		Road.Locate(Chart->Marks);
		TestTrue(TEXT("the shipped start is on the chart"), Placed(Chart->Marks, Road.From));
		TestTrue(TEXT("the shipped end is on the chart"), Placed(Chart->Marks, Road.To));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadRoutedTest,
	"SolidCore1.Road.Routed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadRoutedTest::RunTest(const FString& Parameters)
{
	const USolidWorldChart* Shipped = LoadObject<USolidWorldChart>(nullptr, USolidWorldChart::ShippedPath());
	UTEST_NOT_NULL(TEXT("shipped chart"), Shipped);

	USolidWorldChart* Loaded = NewObject<USolidWorldChart>();
	UTEST_NOT_NULL(TEXT("loaded chart"), Loaded);
	if (!TestTrue(TEXT("the chart loads"), Loaded->LoadFromChart(Shipped)))
	{
		return false;
	}

	const TArray<SolidRoad::FSolidRoad>& Routed = Loaded->GetRoutedRoads();
	TestTrue(TEXT("the chart keeps a routed road"), Routed.Num() > 0);
	for (const SolidRoad::FRoute& Route : Loaded->GetRoads())
	{
		if (Route.Points.Num() <= 1)
		{
			continue;
		}
		bool bKept = false;
		for (const SolidRoad::FSolidRoad& Road : Routed)
		{
			if (Road.Points == Route.Points)
			{
				bKept = true;
				break;
			}
		}
		TestTrue(TEXT("a point route stays on its road"), bKept);
	}
	for (const SolidRoad::FSolidRoad& Road : Routed)
	{
		TestTrue(TEXT("a routed road keeps its points"), Road.Points.Num() > 1);
	}
	TestEqual(TEXT("the shipped list stays unrouted"), SolidRoad::Roads[0].Points.Num(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
