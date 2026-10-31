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

	struct FRoadTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldChart::GrassMarker;
	};

	FString RoadTestGrid(const TArray<FRoadTestMark>& Marks, const FString& Roads)
	{
		FString Text;
		Text.Reserve((USolidWorldChart::MapWidth + 1) * USolidWorldChart::MapHeight + Roads.Len());
		for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldChart::GrassMarker;
				for (const FRoadTestMark& Mark : Marks)
				{
					if (Mark.Col == Col && Mark.Row == Row)
					{
						Marker = Mark.Marker;
						break;
					}
				}
				Text.AppendChar(Marker);
			}
			Text.AppendChar(TEXT('\n'));
		}
		Text += Roads;
		return Text;
	}

	bool RoadTestLoad(FAutomationTestBase& Test, USolidWorldChart& Map, const FString& Text)
	{
		return Test.TestTrue(TEXT("map loads"), Map.LoadFromString(Text));
	}

	const SolidRoad::FRoute* RoadTestRoute(const USolidWorldChart& Map, const int32 Index)
	{
		const TArray<SolidRoad::FRoute>& Roads = Map.GetRoads();
		return Roads.IsValidIndex(Index) ? &Roads[Index] : nullptr;
	}

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

	int32 RoadTestCountBiome(const USolidWorldChart& Map, const TArray<FIntPoint>& Cells, const ESolidBiome Biome)
	{
		int32 Count = 0;
		for (const FIntPoint& Cell : Cells)
		{
			if (Map.GetBiomeAtCell(Cell.X, Cell.Y) == Biome)
			{
				++Count;
			}
		}
		return Count;
	}

	FVector2D RoadTestCenter(const USolidWorldChart& Map, const int32 Col, const int32 Row)
	{
		FVector2D Center = FVector2D::ZeroVector;
		Map.CellToWorldXY(FIntPoint(Col, Row), RoadTestMin, RoadTestMax, Center);
		return Center;
	}

	FString RoadTestNamed(
		const int32 FromCol,
		const int32 FromRow,
		const TCHAR* FromTown,
		const int32 ToCol,
		const int32 ToRow,
		const TCHAR* ToTown,
		const TCHAR* Type)
	{
		return FString::Printf(
			TEXT("! %d,%d = Town %s\n! %d,%d = Town %s\nRoad: %s -> %s, %s\n"),
			FromRow + 1, FromCol + 1, FromTown,
			ToRow + 1, ToCol + 1, ToTown,
			FromTown, ToTown, Type);
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
		UTEST_NOT_NULL(TEXT("patch chart"), Chart);
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
	FSolidRoadParseTest,
	"SolidCore1.Road.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadParseTest::RunTest(const FString& Parameters)
{
	SolidRoad::FDefinition Def;
	TestFalse(TEXT("a letter is not an end"), SolidRoad::Parse(TEXT("Road: C -> Iglin, trail"), Def));
	TestFalse(TEXT("a number is not an end"), SolidRoad::Parse(TEXT("Road: 0 -> 1, trail"), Def));

	TestTrue(TEXT("town names"), SolidRoad::Parse(TEXT("Road: Iglin -> Relion, trail"), Def));
	TestFalse(TEXT("town names are not a stronghold"), Def.From.bStronghold || Def.To.bStronghold);
	TestEqual(TEXT("Iglin id"), Def.From.LocationId, SolidTowns::Iglin);
	TestEqual(TEXT("Relion id"), Def.To.LocationId, SolidTowns::Relion);
	TestEqual(TEXT("trail"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Trail));

	TestTrue(TEXT("spaced track"), SolidRoad::Parse(TEXT("  Road: Dragongate -> Rathen, track  "), Def));
	TestEqual(TEXT("from Dragongate"), Def.From.LocationId, SolidTowns::Dragongate);
	TestEqual(TEXT("to Rathen"), Def.To.LocationId, SolidTowns::Rathen);
	TestEqual(TEXT("track"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Track));

	TestTrue(TEXT("highway"), SolidRoad::Parse(TEXT("Road: Relion -> Kanfold, highway"), Def));
	TestEqual(TEXT("highway type"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Highway));
	TestTrue(TEXT("road type ignores case"), SolidRoad::Parse(TEXT("Road: Relion -> Dragongate, Road"), Def));
	TestEqual(TEXT("road type"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Road));

	TestFalse(TEXT("missing type"), SolidRoad::Parse(TEXT("Road: Relion -> Dragongate"), Def));
	TestFalse(TEXT("bad end"), SolidRoad::Parse(TEXT("Road: nowhere -> Iglin, trail"), Def));
	TestFalse(TEXT("a key line is not a road"), SolidRoad::Parse(TEXT("G = Grassland (light green)"), Def));

	TestTrue(TEXT("stronghold name"), SolidRoad::Parse(TEXT("Road: (MeagreDungeon) -> (Iglin), trail"), Def));
	TestTrue(TEXT("from the stronghold"), Def.From.bStronghold);
	TestEqual(TEXT("MeagreDungeon"), Def.From.StrongholdName, FString(TEXT("MeagreDungeon")));
	TestEqual(TEXT("to Iglin"), Def.To.LocationId, SolidTowns::Iglin);

	TestTrue(TEXT("spaced stronghold name"), SolidRoad::Parse(TEXT("Road: Meagre Dungeon -> iglin, trail"), Def));
	TestTrue(TEXT("spaces still name the stronghold"), Def.From.bStronghold);
	TestEqual(TEXT("the spaced name is kept"), Def.From.StrongholdName, FString(TEXT("Meagre Dungeon")));
	TestEqual(TEXT("iglin ignores case"), Def.To.LocationId, SolidTowns::Iglin);
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
	FSolidRoadAvoidTest,
	"SolidCore1.Road.Avoid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadAvoidTest::RunTest(const FString& Parameters)
{
	TArray<FRoadTestMark> ForestWall;
	ForestWall.Add({ 2, 8, USolidWorldChart::LocationMarker });
	ForestWall.Add({ 20, 8, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		if (Row != 2)
		{
			ForestWall.Add({ 10, Row, USolidWorldChart::ForestMarker });
		}
	}

	USolidWorldChart* Highway = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("highway map"), Highway);
	if (!RoadTestLoad(*this, *Highway, RoadTestGrid(ForestWall, RoadTestNamed(2, 8, TEXT("Iglin"), 20, 8, TEXT("Relion"), TEXT("highway")))))
	{
		return false;
	}
	const SolidRoad::FRoute* HighwayRoute = RoadTestRoute(*Highway, 0);
	TestNotNull(TEXT("highway route"), HighwayRoute);
	if (!HighwayRoute)
	{
		return false;
	}
	TestEqual(TEXT("highway stays on the straight row"), HighwayRoute->Cells.Num(), 19);
	TestEqual(TEXT("highway crosses one forest cell"), RoadTestCountBiome(*Highway, HighwayRoute->Cells, ESolidBiome::Forest), 1);
	RoadTestOrthogonal(*this, HighwayRoute->Cells);

	USolidWorldChart* Trail = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("trail map"), Trail);
	if (!RoadTestLoad(*this, *Trail, RoadTestGrid(ForestWall, RoadTestNamed(2, 8, TEXT("Iglin"), 20, 8, TEXT("Relion"), TEXT("trail")))))
	{
		return false;
	}
	const SolidRoad::FRoute* TrailRoute = RoadTestRoute(*Trail, 0);
	TestNotNull(TEXT("trail route"), TrailRoute);
	if (!TrailRoute)
	{
		return false;
	}
	TestTrue(TEXT("trail takes the forest"), RoadTestCountBiome(*Trail, TrailRoute->Cells, ESolidBiome::Forest) > 0);

	TArray<FRoadTestMark> MountainTie;
	MountainTie.Add({ 5, 5, USolidWorldChart::LocationMarker });
	MountainTie.Add({ 10, 10, USolidWorldChart::LocationMarker });
	for (int32 Col = 6; Col <= 10; ++Col)
	{
		MountainTie.Add({ Col, 5, TEXT('M') });
	}
	USolidWorldChart* Track = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("track map"), Track);
	if (!RoadTestLoad(*this, *Track, RoadTestGrid(MountainTie, RoadTestNamed(5, 5, TEXT("Iglin"), 10, 10, TEXT("Relion"), TEXT("track")))))
	{
		return false;
	}
	const SolidRoad::FRoute* TrackRoute = RoadTestRoute(*Track, 0);
	TestNotNull(TEXT("track route"), TrackRoute);
	if (!TrackRoute)
	{
		return false;
	}
	TestEqual(TEXT("equal-length track stays off the mountain"),
		RoadTestCountBiome(*Track, TrackRoute->Cells, ESolidBiome::Mountain), 0);

	TArray<FRoadTestMark> SeaBay;
	SeaBay.Add({ 2, 10, USolidWorldChart::LocationMarker });
	SeaBay.Add({ 14, 10, USolidWorldChart::LocationMarker });
	for (int32 Col = 6; Col <= 10; ++Col)
	{
		for (int32 Row = 0; Row <= 20; ++Row)
		{
			SeaBay.Add({ Col, Row, USolidWorldChart::SeaMarker });
		}
	}
	USolidWorldChart* SeaTrail = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("sea map"), SeaTrail);
	if (!RoadTestLoad(*this, *SeaTrail, RoadTestGrid(SeaBay, RoadTestNamed(2, 10, TEXT("Iglin"), 14, 10, TEXT("Relion"), TEXT("trail")))))
	{
		return false;
	}
	const SolidRoad::FRoute* SeaRoute = RoadTestRoute(*SeaTrail, 0);
	TestNotNull(TEXT("sea route"), SeaRoute);
	if (!SeaRoute)
	{
		return false;
	}
	TestEqual(TEXT("trail stays off the sea"), RoadTestCountBiome(*SeaTrail, SeaRoute->Cells, ESolidBiome::Sea), 0);

	TArray<FRoadTestMark> MountainWall;
	MountainWall.Add({ 2, 8, USolidWorldChart::LocationMarker });
	MountainWall.Add({ 20, 8, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		if (Row != 2)
		{
			MountainWall.Add({ 10, Row, TEXT('M') });
		}
	}
	USolidWorldChart* MountainTrail = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("mountain map"), MountainTrail);
	if (!RoadTestLoad(*this, *MountainTrail, RoadTestGrid(MountainWall, RoadTestNamed(2, 8, TEXT("Iglin"), 20, 8, TEXT("Relion"), TEXT("trail")))))
	{
		return false;
	}
	const SolidRoad::FRoute* MountainRoute = RoadTestRoute(*MountainTrail, 0);
	TestNotNull(TEXT("mountain route"), MountainRoute);
	if (!MountainRoute)
	{
		return false;
	}
	TestTrue(TEXT("trail goes around the mountains"), MountainRoute->Cells.Num() > 19);
	TestEqual(TEXT("trail stays off the mountains"),
		RoadTestCountBiome(*MountainTrail, MountainRoute->Cells, ESolidBiome::Mountain), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadRiverTest,
	"SolidCore1.Road.River",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadRiverTest::RunTest(const FString& Parameters)
{
	TArray<FRoadTestMark> Marks;
	Marks.Add({ 4, 10, USolidWorldChart::LocationMarker });
	Marks.Add({ 22, 10, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		if (Row != 22)
		{
			Marks.Add({ 12, Row, USolidWorldChart::RiverMarker });
		}
	}

	USolidWorldChart* Road = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("road map"), Road);
	if (!RoadTestLoad(*this, *Road, RoadTestGrid(Marks, RoadTestNamed(4, 10, TEXT("Iglin"), 22, 10, TEXT("Relion"), TEXT("road")))))
	{
		return false;
	}
	const SolidRoad::FRoute* RoadRoute = RoadTestRoute(*Road, 0);
	TestNotNull(TEXT("road route"), RoadRoute);
	if (!RoadRoute)
	{
		return false;
	}
	TestEqual(TEXT("a road crosses the river once"), RoadTestCountBiome(*Road, RoadRoute->Cells, ESolidBiome::River), 1);

	USolidWorldChart* Track = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("track map"), Track);
	if (!RoadTestLoad(*this, *Track, RoadTestGrid(Marks, RoadTestNamed(4, 10, TEXT("Iglin"), 22, 10, TEXT("Relion"), TEXT("track")))))
	{
		return false;
	}
	const SolidRoad::FRoute* TrackRoute = RoadTestRoute(*Track, 0);
	TestNotNull(TEXT("track route"), TrackRoute);
	if (!TrackRoute)
	{
		return false;
	}
	TestTrue(TEXT("track crosses the river"), RoadTestCountBiome(*Track, TrackRoute->Cells, ESolidBiome::River) >= 1);

	TArray<FRoadTestMark> Blocked;
	Blocked.Add({ 4, 20, USolidWorldChart::LocationMarker });
	Blocked.Add({ 20, 20, USolidWorldChart::LocationMarker });
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		Blocked.Add({ 12, Row, USolidWorldChart::RiverMarker });
	}
	USolidWorldChart* Forced = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("forced map"), Forced);
	if (!RoadTestLoad(*this, *Forced, RoadTestGrid(Blocked, RoadTestNamed(4, 20, TEXT("Iglin"), 20, 20, TEXT("Relion"), TEXT("road")))))
	{
		return false;
	}
	const SolidRoad::FRoute* ForcedRoute = RoadTestRoute(*Forced, 0);
	TestNotNull(TEXT("forced route"), ForcedRoute);
	if (!ForcedRoute)
	{
		return false;
	}
	TestEqual(TEXT("a blocking river is crossed once"),
		RoadTestCountBiome(*Forced, ForcedRoute->Cells, ESolidBiome::River), 1);
	TestEqual(TEXT("the crossing stays off the sea"),
		RoadTestCountBiome(*Forced, ForcedRoute->Cells, ESolidBiome::Sea), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadSurfaceTest,
	"SolidCore1.Road.Surface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadSurfaceTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Trail = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("trail map"), Trail);
	if (!RoadTestLoad(*this, *Trail, RoadTestGrid({
		{ 4, 12, USolidWorldChart::LocationMarker },
		{ 4, 14, USolidWorldChart::TownMarker },
		{ 4, 16, TEXT('Z') },
		{ 7, 12, USolidWorldChart::ForestMarker },
		{ 12, 12, USolidWorldChart::LocationMarker },
	}, RoadTestNamed(4, 12, TEXT("Iglin"), 12, 12, TEXT("Relion"), TEXT("trail")))))
	{
		return false;
	}

	const FVector2D Grass = RoadTestCenter(*Trail, 6, 12);
	const FVector2D Forest = RoadTestCenter(*Trail, 7, 12);
	TestEqual(TEXT("marker under the trail stays grass"),
		static_cast<uint8>(Trail->GetBiomeAtCell(6, 12)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("trail centre uses the road texture"),
		static_cast<uint8>(Trail->SampleBiome(Grass.X, Grass.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("trail rim is road"),
		static_cast<uint8>(Trail->SampleBiome(Grass.X, Grass.Y + 50.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("past the trail is grass"),
		static_cast<uint8>(Trail->SampleBiome(Grass.X, Grass.Y + 70.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Grassland));
	TestEqual(TEXT("forest on the trail uses the road texture"),
		static_cast<uint8>(Trail->SampleBiome(Forest.X, Forest.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestFalse(TEXT("grass under a trail is not a town marker"),
		Trail->IsMarkedTown(Grass.X, Grass.Y, RoadTestMin, RoadTestMax));
	const FVector2D TownDigit = RoadTestCenter(*Trail, 4, 12);
	const FVector2D TownLetter = RoadTestCenter(*Trail, 4, 14);
	const FVector2D LegacyTown = RoadTestCenter(*Trail, 4, 16);
	TestTrue(TEXT("a named town is a town"),
		Trail->IsMarkedTown(TownDigit.X, TownDigit.Y, RoadTestMin, RoadTestMax));
	TestTrue(TEXT("a town circle is a town"),
		Trail->IsMarkedTown(TownLetter.X, TownLetter.Y, RoadTestMin, RoadTestMax));
	TestFalse(TEXT("Z is not a town"),
		Trail->IsMarkedTown(LegacyTown.X, LegacyTown.Y, RoadTestMin, RoadTestMax));
	const FLinearColor RoadBrown = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Road);
	const FLinearColor TownBrown = USolidWorldChart::DefaultColorForBiome(ESolidBiome::Town);
	TestTrue(TEXT("road brown sits near the town brown"),
		RoadBrown.R > RoadBrown.G && RoadBrown.G > RoadBrown.B
		&& TownBrown.R > TownBrown.G && TownBrown.G > TownBrown.B
		&& FMath::Abs(RoadBrown.R - TownBrown.R) < 0.2f);

	USolidWorldChart* Cave = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("cave map"), Cave);
	if (!RoadTestLoad(*this, *Cave, RoadTestGrid({
		{ 4, 8, USolidWorldChart::LocationMarker },
		{ 8, 8, USolidWorldChart::LocationMarker },
	}, TEXT("! 9,5 = Stronghold (MeagreDungeon)\n! 9,9 = Town Iglin\nRoad: MeagreDungeon -> Iglin, trail\n"))))
	{
		return false;
	}
	const FVector2D CaveCell = RoadTestCenter(*Cave, 4, 8);
	const FSolidStronghold MeagreDungeon(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre);
	ESolidBiome CaveShelf = ESolidBiome::Grassland;
	TestTrue(TEXT("the cave names a shelf"), SolidStronghold::BiomeFor(MeagreDungeon, CaveShelf));
	TestEqual(TEXT("the cave cell keeps its shelf"),
		static_cast<uint8>(Cave->SampleBiome(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(CaveShelf));
	TestFalse(TEXT("the cave is not a town"),
		Cave->IsMarkedTown(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax));

	USolidWorldChart* Crossing = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("crossing map"), Crossing);
	if (!RoadTestLoad(*this, *Crossing, RoadTestGrid({
		{ 4, 16, USolidWorldChart::LocationMarker },
		{ 10, 15, USolidWorldChart::RiverMarker },
		{ 10, 16, USolidWorldChart::RiverMarker },
		{ 10, 17, USolidWorldChart::RiverMarker },
		{ 16, 16, USolidWorldChart::LocationMarker },
	}, RoadTestNamed(4, 16, TEXT("Iglin"), 16, 16, TEXT("Relion"), TEXT("track")))))
	{
		return false;
	}
	const FVector2D River = RoadTestCenter(*Crossing, 10, 16);
	TestEqual(TEXT("the channel stays river"),
		static_cast<uint8>(Crossing->SampleBiome(River.X, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(TEXT("the bank of a crossing uses the road texture"),
		static_cast<uint8>(Crossing->SampleBiome(River.X + 400.f, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));

	USolidWorldChart* Highway = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("highway map"), Highway);
	if (!RoadTestLoad(*this, *Highway, RoadTestGrid({
		{ 8, 20, USolidWorldChart::LocationMarker },
		{ 12, 20, USolidWorldChart::LocationMarker },
	}, RoadTestNamed(8, 20, TEXT("Iglin"), 12, 20, TEXT("Relion"), TEXT("highway")))))
	{
		return false;
	}
	const FVector2D Wide = RoadTestCenter(*Highway, 10, 20);
	TestEqual(TEXT("inside a highway"),
		static_cast<uint8>(Highway->SampleBiome(Wide.X, Wide.Y + 190.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));
	TestEqual(TEXT("outside a highway"),
		static_cast<uint8>(Highway->SampleBiome(Wide.X, Wide.Y + 220.f, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Grassland));

	USolidWorldChart* Missing = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("missing map"), Missing);
	if (!RoadTestLoad(*this, *Missing, RoadTestGrid({
		{ 4, 4, USolidWorldChart::LocationMarker },
	}, TEXT("! 5,5 = Town Iglin\nRoad: Iglin -> Nowhere, trail\n"))))
	{
		return false;
	}
	TestEqual(TEXT("an unknown end is ignored"), Missing->GetRoads().Num(), 0);
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

#endif // WITH_DEV_AUTOMATION_TESTS
