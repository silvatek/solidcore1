#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidRoad.h"
#include "SolidTerrainTypes.h"
#include "SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const FVector2D RoadTestMin(-51200.f, -51200.f);
	const FVector2D RoadTestMax(51200.f, 51200.f);

	struct FRoadTestMark
	{
		int32 Col = 0;
		int32 Row = 0;
		TCHAR Marker = USolidWorldMap::GrassMarker;
	};

	FString RoadTestGrid(const TArray<FRoadTestMark>& Marks, const FString& Roads)
	{
		FString Text;
		Text.Reserve((USolidWorldMap::MapWidth + 1) * USolidWorldMap::MapHeight + Roads.Len());
		for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
		{
			for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
			{
				TCHAR Marker = USolidWorldMap::GrassMarker;
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

	bool RoadTestLoad(FAutomationTestBase& Test, USolidWorldMap& Map, const FString& Text)
	{
		return Test.TestTrue(TEXT("map loads"), Map.LoadFromString(Text));
	}

	const SolidRoad::FRoute* RoadTestRoute(const USolidWorldMap& Map, const int32 Index)
	{
		const TArray<SolidRoad::FRoute>& Roads = Map.GetRoads();
		return Roads.IsValidIndex(Index) ? &Roads[Index] : nullptr;
	}

	bool RoadTestOrthogonal(FAutomationTestBase& Test, const TArray<FIntPoint>& Cells)
	{
		for (int32 Index = 1; Index < Cells.Num(); ++Index)
		{
			const int32 Steps = FMath::Abs(Cells[Index].X - Cells[Index - 1].X)
				+ FMath::Abs(Cells[Index].Y - Cells[Index - 1].Y);
			if (!Test.TestEqual(TEXT("route steps to a neighbour"), Steps, 1))
			{
				return false;
			}
		}
		return true;
	}

	int32 RoadTestCountBiome(const USolidWorldMap& Map, const TArray<FIntPoint>& Cells, const ESolidBiome Biome)
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

	FVector2D RoadTestCenter(const USolidWorldMap& Map, const int32 Col, const int32 Row)
	{
		FVector2D Center = FVector2D::ZeroVector;
		Map.CellToWorldXY(FIntPoint(Col, Row), RoadTestMin, RoadTestMax, Center);
		return Center;
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
	TestTrue(TEXT("roads and highways avoid hard ground"),
		SolidRoad::AvoidsHardGround(SolidRoad::EType::Road)
		&& SolidRoad::AvoidsHardGround(SolidRoad::EType::Highway));
	TestFalse(TEXT("a trail does not avoid hard ground"), SolidRoad::AvoidsHardGround(SolidRoad::EType::Trail));
	TestFalse(TEXT("a track does not avoid hard ground"), SolidRoad::AvoidsHardGround(SolidRoad::EType::Track));

	TestTrue(TEXT("a trail treats a mountain as a wall"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::Mountain) > SolidRoad::StepCost * 64);
	TestTrue(TEXT("a track treats a mountain as a wall"),
		SolidRoad::EnterCost(SolidRoad::EType::Track, ESolidBiome::Mountain) > SolidRoad::StepCost * 64);
	TestTrue(TEXT("a highway treats forest as a wall"),
		SolidRoad::EnterCost(SolidRoad::EType::Highway, ESolidBiome::Forest) > SolidRoad::StepCost * 64);
	TestTrue(TEXT("a road pays more for a river than for grass"),
		SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::River)
		> SolidRoad::EnterCost(SolidRoad::EType::Road, ESolidBiome::Grassland) + SolidRoad::StepCost);
	TestTrue(TEXT("sea is closed to a trail"),
		SolidRoad::EnterCost(SolidRoad::EType::Trail, ESolidBiome::Sea) > SolidRoad::StepCost * 64);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadParseTest,
	"SolidCore1.Road.Parse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadParseTest::RunTest(const FString& Parameters)
{
	SolidRoad::FDefinition Def;
	TestTrue(TEXT("cave trail"), SolidRoad::Parse(TEXT("Road: C -> 0, trail"), Def));
	TestTrue(TEXT("from the cave"), Def.From.bCave);
	TestFalse(TEXT("to a town"), Def.To.bCave);
	TestEqual(TEXT("town 0"), Def.To.LocationId, 0);
	TestEqual(TEXT("trail"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Trail));

	TestTrue(TEXT("spaced track"), SolidRoad::Parse(TEXT("  Road: 5 -> 6, track  "), Def));
	TestEqual(TEXT("from 5"), Def.From.LocationId, 5);
	TestEqual(TEXT("to 6"), Def.To.LocationId, 6);
	TestEqual(TEXT("track"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Track));

	TestTrue(TEXT("highway"), SolidRoad::Parse(TEXT("Road: 1 -> 2, highway"), Def));
	TestEqual(TEXT("highway type"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Highway));
	TestTrue(TEXT("road type ignores case"), SolidRoad::Parse(TEXT("Road: 1 -> 5, Road"), Def));
	TestEqual(TEXT("road type"), static_cast<uint8>(Def.Type), static_cast<uint8>(SolidRoad::EType::Road));

	TestFalse(TEXT("missing type"), SolidRoad::Parse(TEXT("Road: 1 -> 5"), Def));
	TestFalse(TEXT("bad end"), SolidRoad::Parse(TEXT("Road: nowhere -> 1, trail"), Def));
	TestFalse(TEXT("a key line is not a road"), SolidRoad::Parse(TEXT("G = Grassland (light green)"), Def));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadAvoidTest,
	"SolidCore1.Road.Avoid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadAvoidTest::RunTest(const FString& Parameters)
{
	TArray<FRoadTestMark> ForestWall;
	ForestWall.Add({ 2, 8, TEXT('0') });
	ForestWall.Add({ 20, 8, TEXT('1') });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		if (Row != 2)
		{
			ForestWall.Add({ 10, Row, USolidWorldMap::ForestMarker });
		}
	}

	USolidWorldMap* Highway = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("highway map"), Highway);
	if (!RoadTestLoad(*this, *Highway, RoadTestGrid(ForestWall, TEXT("Road: 0 -> 1, highway\n"))))
	{
		return false;
	}
	const SolidRoad::FRoute* HighwayRoute = RoadTestRoute(*Highway, 0);
	TestNotNull(TEXT("highway route"), HighwayRoute);
	if (!HighwayRoute)
	{
		return false;
	}
	TestTrue(TEXT("highway reaches the far town"), HighwayRoute->Cells.Num() > 19);
	TestEqual(TEXT("highway stays off the forest"), RoadTestCountBiome(*Highway, HighwayRoute->Cells, ESolidBiome::Forest), 0);
	RoadTestOrthogonal(*this, HighwayRoute->Cells);

	USolidWorldMap* Trail = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("trail map"), Trail);
	if (!RoadTestLoad(*this, *Trail, RoadTestGrid(ForestWall, TEXT("Road: 0 -> 1, trail\n"))))
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
	MountainTie.Add({ 5, 5, TEXT('0') });
	MountainTie.Add({ 10, 10, TEXT('1') });
	for (int32 Col = 6; Col <= 10; ++Col)
	{
		MountainTie.Add({ Col, 5, TEXT('M') });
	}
	USolidWorldMap* Track = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("track map"), Track);
	if (!RoadTestLoad(*this, *Track, RoadTestGrid(MountainTie, TEXT("Road: 0 -> 1, track\n"))))
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
	SeaBay.Add({ 2, 10, TEXT('0') });
	SeaBay.Add({ 14, 10, TEXT('1') });
	for (int32 Col = 6; Col <= 10; ++Col)
	{
		for (int32 Row = 0; Row <= 20; ++Row)
		{
			SeaBay.Add({ Col, Row, USolidWorldMap::SeaMarker });
		}
	}
	USolidWorldMap* SeaTrail = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("sea map"), SeaTrail);
	if (!RoadTestLoad(*this, *SeaTrail, RoadTestGrid(SeaBay, TEXT("Road: 0 -> 1, trail\n"))))
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
	MountainWall.Add({ 2, 8, TEXT('0') });
	MountainWall.Add({ 20, 8, TEXT('1') });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		if (Row != 2)
		{
			MountainWall.Add({ 10, Row, TEXT('M') });
		}
	}
	USolidWorldMap* MountainTrail = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("mountain map"), MountainTrail);
	if (!RoadTestLoad(*this, *MountainTrail, RoadTestGrid(MountainWall, TEXT("Road: 0 -> 1, trail\n"))))
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
	Marks.Add({ 4, 10, TEXT('0') });
	Marks.Add({ 22, 10, TEXT('1') });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		if (Row != 22)
		{
			Marks.Add({ 12, Row, USolidWorldMap::RiverMarker });
		}
	}

	USolidWorldMap* Road = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("road map"), Road);
	if (!RoadTestLoad(*this, *Road, RoadTestGrid(Marks, TEXT("Road: 0 -> 1, road\n"))))
	{
		return false;
	}
	const SolidRoad::FRoute* RoadRoute = RoadTestRoute(*Road, 0);
	TestNotNull(TEXT("road route"), RoadRoute);
	if (!RoadRoute)
	{
		return false;
	}
	TestEqual(TEXT("road uses the ford"), RoadTestCountBiome(*Road, RoadRoute->Cells, ESolidBiome::River), 0);

	USolidWorldMap* Track = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("track map"), Track);
	if (!RoadTestLoad(*this, *Track, RoadTestGrid(Marks, TEXT("Road: 0 -> 1, track\n"))))
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
	Blocked.Add({ 4, 20, TEXT('0') });
	Blocked.Add({ 20, 20, TEXT('1') });
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		Blocked.Add({ 12, Row, USolidWorldMap::RiverMarker });
	}
	USolidWorldMap* Forced = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("forced map"), Forced);
	if (!RoadTestLoad(*this, *Forced, RoadTestGrid(Blocked, TEXT("Road: 0 -> 1, road\n"))))
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
	USolidWorldMap* Trail = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("trail map"), Trail);
	if (!RoadTestLoad(*this, *Trail, RoadTestGrid({
		{ 4, 12, TEXT('0') },
		{ 4, 14, USolidWorldMap::TownMarker },
		{ 4, 16, TEXT('Z') },
		{ 7, 12, USolidWorldMap::ForestMarker },
		{ 12, 12, TEXT('1') },
	}, TEXT("Road: 0 -> 1, trail\n"))))
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
	TestTrue(TEXT("a location digit is a town"),
		Trail->IsMarkedTown(TownDigit.X, TownDigit.Y, RoadTestMin, RoadTestMax));
	TestTrue(TEXT("T is a town"),
		Trail->IsMarkedTown(TownLetter.X, TownLetter.Y, RoadTestMin, RoadTestMax));
	TestTrue(TEXT("Z is a town"),
		Trail->IsMarkedTown(LegacyTown.X, LegacyTown.Y, RoadTestMin, RoadTestMax));
	const FLinearColor RoadBrown = USolidWorldMap::DefaultColorForBiome(ESolidBiome::Road);
	const FLinearColor TownBrown = USolidWorldMap::DefaultColorForBiome(ESolidBiome::Town);
	TestTrue(TEXT("road brown sits near the town brown"),
		RoadBrown.R > RoadBrown.G && RoadBrown.G > RoadBrown.B
		&& TownBrown.R > TownBrown.G && TownBrown.G > TownBrown.B
		&& FMath::Abs(RoadBrown.R - TownBrown.R) < 0.2f);

	USolidWorldMap* Cave = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("cave map"), Cave);
	if (!RoadTestLoad(*this, *Cave, RoadTestGrid({
		{ 4, 8, TEXT('C') },
		{ 8, 8, TEXT('0') },
	}, TEXT("Road: C -> 0, trail\n"))))
	{
		return false;
	}
	const FVector2D CaveCell = RoadTestCenter(*Cave, 4, 8);
	TestEqual(TEXT("the cave cell keeps the mountain shelf"),
		static_cast<uint8>(Cave->SampleBiome(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Mountain));
	TestFalse(TEXT("the cave is not a town"),
		Cave->IsMarkedTown(CaveCell.X, CaveCell.Y, RoadTestMin, RoadTestMax));

	USolidWorldMap* Crossing = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("crossing map"), Crossing);
	if (!RoadTestLoad(*this, *Crossing, RoadTestGrid({
		{ 4, 16, TEXT('0') },
		{ 10, 15, USolidWorldMap::RiverMarker },
		{ 10, 16, USolidWorldMap::RiverMarker },
		{ 10, 17, USolidWorldMap::RiverMarker },
		{ 16, 16, TEXT('1') },
	}, TEXT("Road: 0 -> 1, track\n"))))
	{
		return false;
	}
	const FVector2D River = RoadTestCenter(*Crossing, 10, 16);
	TestEqual(TEXT("the channel stays river"),
		static_cast<uint8>(Crossing->SampleBiome(River.X, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::River));
	TestEqual(TEXT("the bank of a crossing uses the road texture"),
		static_cast<uint8>(Crossing->SampleBiome(River.X + 250.f, River.Y, RoadTestMin, RoadTestMax)),
		static_cast<uint8>(ESolidBiome::Road));

	USolidWorldMap* Highway = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("highway map"), Highway);
	if (!RoadTestLoad(*this, *Highway, RoadTestGrid({
		{ 8, 20, TEXT('0') },
		{ 12, 20, TEXT('1') },
	}, TEXT("Road: 0 -> 1, highway\n"))))
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

	USolidWorldMap* Missing = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("missing map"), Missing);
	if (!RoadTestLoad(*this, *Missing, RoadTestGrid({
		{ 4, 4, TEXT('0') },
	}, TEXT("Road: 0 -> 9, trail\n"))))
	{
		return false;
	}
	const SolidRoad::FRoute* MissingRoute = RoadTestRoute(*Missing, 0);
	TestNotNull(TEXT("missing end is still listed"), MissingRoute);
	if (MissingRoute)
	{
		TestEqual(TEXT("missing end has no cells"), MissingRoute->Cells.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadShippedTest,
	"SolidCore1.Road.ShippedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadShippedTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("map"), Map);
	TestTrue(TEXT("default map loads"), Map->LoadDefault());

	const TArray<SolidRoad::FRoute>& Roads = Map->GetRoads();
	TestEqual(TEXT("six roads"), Roads.Num(), 6);
	if (Roads.Num() < 6)
	{
		return false;
	}

	const SolidRoad::EType Types[] = {
		SolidRoad::EType::Trail,
		SolidRoad::EType::Track,
		SolidRoad::EType::Highway,
		SolidRoad::EType::Road,
		SolidRoad::EType::Track,
		SolidRoad::EType::Track,
	};
	const int32 FromIds[] = { INDEX_NONE, 0, 1, 1, 5, 5 };
	const int32 ToIds[] = { 0, 1, 2, 5, 4, 6 };
	for (int32 Index = 0; Index < 6; ++Index)
	{
		TestEqual(TEXT("road type"), static_cast<uint8>(Roads[Index].Definition.Type), static_cast<uint8>(Types[Index]));
		TestEqual(TEXT("road starts where the line says"),
			Roads[Index].Definition.From.bCave ? INDEX_NONE : Roads[Index].Definition.From.LocationId,
			FromIds[Index]);
		TestEqual(TEXT("road ends where the line says"), Roads[Index].Definition.To.LocationId, ToIds[Index]);
		TestTrue(TEXT("route has cells"), Roads[Index].Cells.Num() >= 2);
		RoadTestOrthogonal(*this, Roads[Index].Cells);
	}

	TestTrue(TEXT("the trail leaves the cave"), Roads[0].Definition.From.bCave);
	TestEqual(TEXT("trail starts on C"), Map->GetMarkerAtCell(Roads[0].Cells[0].X, Roads[0].Cells[0].Y), TEXT('C'));
	TestEqual(TEXT("trail ends at Iglin"), Map->GetLocationIdAtCell(Roads[0].Cells.Last().X, Roads[0].Cells.Last().Y), 0);
	for (const SolidRoad::FRoute& Road : Roads)
	{
		int32 MountainCells = 0;
		int32 SeaCells = 0;
		for (const FIntPoint& Cell : Road.Cells)
		{
			if (Map->GetMarkerAtCell(Cell.X, Cell.Y) == TEXT('M'))
			{
				++MountainCells;
			}
			if (Map->GetBiomeAtCell(Cell.X, Cell.Y) == ESolidBiome::Sea)
			{
				++SeaCells;
			}
		}
		TestEqual(TEXT("route stays off the mountains"), MountainCells, 0);
		TestEqual(TEXT("route stays off the sea"), SeaCells, 0);
	}

	TestTrue(TEXT("Iglin to Relion crosses the river"),
		RoadTestCountBiome(*Map, Roads[1].Cells, ESolidBiome::River) >= 1);

	const int32 OpenRoads[] = { 2, 3 };
	for (const int32 Index : OpenRoads)
	{
		TestEqual(TEXT("road stays off the forest"), RoadTestCountBiome(*Map, Roads[Index].Cells, ESolidBiome::Forest), 0);
		TestEqual(TEXT("road stays off the mountain"), RoadTestCountBiome(*Map, Roads[Index].Cells, ESolidBiome::Mountain), 0);
		TestEqual(TEXT("road stays off the desert"), RoadTestCountBiome(*Map, Roads[Index].Cells, ESolidBiome::Desert), 0);
		TestEqual(TEXT("road stays off the sea"), RoadTestCountBiome(*Map, Roads[Index].Cells, ESolidBiome::Sea), 0);
		TestEqual(TEXT("road stays off the river"), RoadTestCountBiome(*Map, Roads[Index].Cells, ESolidBiome::River), 0);
		TestTrue(TEXT("highway and road use the near town"), Roads[Index].Cells.Num() < 30);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidRoadCrossingTest,
	"SolidCore1.Road.Crossing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidRoadCrossingTest::RunTest(const FString& Parameters)
{
	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("map"), Map);
	if (!TestTrue(TEXT("default map loads"), Map && Map->LoadDefault()))
	{
		return false;
	}

	const TArray<SolidRoad::FRoute>& Roads = Map->GetRoads();
	if (!TestTrue(TEXT("the track is listed"), Roads.Num() > 1))
	{
		return false;
	}

	const TArray<FIntPoint>& Cells = Roads[1].Cells;
	int32 River = INDEX_NONE;
	for (int32 Index = 0; Index < Cells.Num(); ++Index)
	{
		if (Map->GetMarkerAtCell(Cells[Index].X, Cells[Index].Y) == USolidWorldMap::RiverMarker)
		{
			River = Index;
			break;
		}
	}
	if (!TestTrue(TEXT("Iglin to Relion crosses one river cell"), River > 2 && River + 2 < Cells.Num()))
	{
		return false;
	}

	const int32 Row = Cells[River].Y;
	TestTrue(TEXT("the road runs straight into the river"),
		Cells[River - 1].Y == Row && Cells[River - 2].Y == Row);
	TestTrue(TEXT("the road runs straight off the river"),
		Cells[River + 1].Y == Row && Cells[River + 2].Y == Row);
	TestTrue(TEXT("the crossing does not step sideways"),
		Cells[River - 1].X != Cells[River].X && Cells[River + 1].X != Cells[River].X
		&& Cells[River - 1].Y == Cells[River + 1].Y);

	int32 End = River;
	while (End + 1 < Cells.Num() && Cells[End + 1].Y == Row)
	{
		++End;
	}
	TestTrue(TEXT("the far bank bends off that line"), End + 1 < Cells.Num() && Cells[End + 1].Y != Row);
	const int32 StepX = Cells[End].X - Cells[End - 1].X;
	const FIntPoint Spur(Cells[End].X + StepX, Row);
	TestFalse(TEXT("the far bank has no spur past the bend"), Cells.Contains(Spur));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
