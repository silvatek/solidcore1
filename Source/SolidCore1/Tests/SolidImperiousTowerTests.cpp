#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonLevel.h"
#include "SolidImperiousTower.h"
#include "SolidStrongholdGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidImperiousTowerTestPrivate
{
	void TestKeptHexes(
		FAutomationTestBase& Test,
		const SolidImperiousTower::FPlan& Plan,
		const int32 Floor,
		const int32 ExpectedCount,
		bool& bSawStair,
		bool& bSawGap)
	{
		TArray<SolidStrongholdGrid::FHex> Kept;
		SolidImperiousTower::CollectKeptHexes(Plan, Floor, Kept);
		Test.TestEqual(TEXT("kept hexes on this floor"), Kept.Num(), ExpectedCount);

		const float Radius = SolidImperiousTower::InteriorRadiusCm(Plan, SolidImperiousTower::FloorZCm(Plan, Floor));
		TSet<uint64> Listed;
		for (const SolidStrongholdGrid::FHex& Hex : Kept)
		{
			Listed.Add((static_cast<uint64>(static_cast<uint32>(Hex.Col)) << 32) | static_cast<uint32>(Hex.Row));
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
			for (const FVector2D& Vertex : Vertices)
			{
				Test.TestTrue(TEXT("a kept vertex stays inside the floor circle"), Vertex.Size() <= Radius + 0.01f);
			}
			Test.TestFalse(
				TEXT("a kept hex is not on the stair"),
				SolidImperiousTower::HexOnStair(Plan, Floor, Hex.Col, Hex.Row));
			Test.TestFalse(
				TEXT("a kept hex is not in the hole"),
				SolidImperiousTower::HexOnGap(Plan, Floor, Hex.Col, Hex.Row));
		}

		const int32 Span = SolidImperiousTower::HexSpan(Plan);
		for (int32 Row = -Span; Row <= Span; ++Row)
		{
			for (int32 Col = -Span; Col <= Span; ++Col)
			{
				const bool bKept = SolidImperiousTower::HexKept(Plan, Floor, Col, Row);
				const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Col)) << 32) | static_cast<uint32>(Row);
				Test.TestEqual(TEXT("the list is the kept cells"), Listed.Contains(Key), bKept);
				if (SolidImperiousTower::HexOnStair(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a stair cell is missing from the hex list"), bKept);
					bSawStair = true;
				}
				if (SolidImperiousTower::HexOnGap(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a hole cell is missing from the hex list"), bKept);
					bSawGap = true;
				}
			}
		}

		const int32 Wide = Span + 2;
		for (int32 Row = -Wide; Row <= Wide; ++Row)
		{
			for (int32 Col = -Wide; Col <= Wide; ++Col)
			{
				if (FMath::Abs(Row) <= Span && FMath::Abs(Col) <= Span)
				{
					continue;
				}
				Test.TestFalse(
					TEXT("a hex past the span is outside this floor"),
					SolidImperiousTower::HexKept(Plan, Floor, Col, Row));
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidImperiousTowerMeasureTest,
	"SolidCore1.Stronghold.ImperiousTower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidImperiousTowerMeasureTest::RunTest(const FString& Parameters)
{
	const SolidImperiousTower::FPlan& Plan = SolidImperiousTower::Imperious;

	TestEqual(TEXT("ten floors"), Plan.FloorCount, 10);
	TestTrue(TEXT("each floor is 3 m"), FMath::IsNearlyEqual(Plan.FloorPitchCm, 300.f, 0.01f));
	TestTrue(TEXT("the shaft is 30 m"), FMath::IsNearlyEqual(Plan.HeightCm(), 3000.f, 0.01f));
	TestTrue(
		TEXT("the top floor's ceiling is the cap"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FloorZCm(Plan, Plan.FloorCount - 1) + Plan.FloorPitchCm,
			Plan.HeightCm(),
			0.01f));
	TestTrue(
		TEXT("the ground floor is at the base"),
		FMath::IsNearlyEqual(SolidImperiousTower::FloorZCm(Plan, 0), 0.f, 0.01f));
	TestTrue(TEXT("the slab is 20 cm"), FMath::IsNearlyEqual(Plan.SlabCm, 20.f, 0.01f));
	TestTrue(
		TEXT("the clear room is the pitch minus the slab"),
		FMath::IsNearlyEqual(Plan.ClearHeightCm(), 280.f, 0.01f));

	TestTrue(
		TEXT("ground exterior diameter is 10 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::ExteriorRadiusCm(Plan, 0.f) * 2.f, 1000.f, 0.01f));
	TestTrue(
		TEXT("ground interior diameter is 9 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::InteriorRadiusCm(Plan, 0.f) * 2.f, 900.f, 0.01f));
	TestTrue(
		TEXT("top exterior diameter is 6 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::ExteriorRadiusCm(Plan, Plan.HeightCm()) * 2.f, 600.f, 0.01f));
	TestTrue(
		TEXT("top interior diameter is 5 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::InteriorRadiusCm(Plan, Plan.HeightCm()) * 2.f, 500.f, 0.01f));

	const float SampleZ[] = { 0.f, 750.f, 1500.f, 2400.f, 3000.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays 50 cm"),
			FMath::IsNearlyEqual(
				SolidImperiousTower::ExteriorRadiusCm(Plan, ZCm) - SolidImperiousTower::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	TestTrue(
		TEXT("the door matches the cave width"),
		FMath::IsNearlyEqual(Plan.DoorWidthCm, SolidDungeonLevel::DoorwayWidthCm, 0.01f));
	TestTrue(
		TEXT("the door matches the cave height"),
		FMath::IsNearlyEqual(Plan.DoorHeightCm, SolidDungeonLevel::MouthHeightCm, 0.01f));
	TestTrue(TEXT("the ground floor has the door"), SolidImperiousTower::HasDoor(0));
	TestFalse(TEXT("the floor above has no door"), SolidImperiousTower::HasDoor(1));
	TestFalse(TEXT("the top floor has no door"), SolidImperiousTower::HasDoor(Plan.FloorCount - 1));
	const FVector2D Outward = SolidImperiousTower::DoorOutward(Plan);
	TestTrue(
		TEXT("the door faces local -X"),
		FMath::IsNearlyEqual(Outward.X, -1.f, 0.01f) && FMath::IsNearlyZero(Outward.Y, 0.01f));

	TestEqual(TEXT("seventeen risers"), Plan.StairRiserCount, 17);
	TestEqual(TEXT("sixteen treads"), Plan.StairTreadCount, 16);
	TestTrue(TEXT("a tread is 26 cm"), FMath::IsNearlyEqual(Plan.StairTreadCm, 26.f, 0.01f));
	TestTrue(TEXT("the run is sixteen treads"), FMath::IsNearlyEqual(Plan.StairRunCm(), 416.f, 0.01f));
	TestTrue(
		TEXT("a riser is the pitch split seventeen ways"),
		FMath::IsNearlyEqual(Plan.RiserCm(), 300.f / 17.f, 0.01f));
	TestTrue(
		TEXT("the stair is as wide as the door"),
		FMath::IsNearlyEqual(Plan.StairWidthCm, Plan.DoorWidthCm, 0.01f));

	const float GroundSweep = 416.f / (450.f - 70.f);
	const float LastSweep = 416.f / (290.f - 70.f);
	TestTrue(
		TEXT("the ground flight is the run along the 3.8 m centerline"),
		FMath::IsNearlyEqual(SolidImperiousTower::FlightSweepRad(Plan, 0), GroundSweep, 0.001f));
	TestTrue(
		TEXT("the last flight is the run along the 2.2 m centerline"),
		FMath::IsNearlyEqual(SolidImperiousTower::FlightSweepRad(Plan, 8), LastSweep, 0.001f));
	TestTrue(
		TEXT("the last flight sweeps farther than the ground flight"),
		SolidImperiousTower::FlightSweepRad(Plan, 8) > SolidImperiousTower::FlightSweepRad(Plan, 0));

	const SolidImperiousTower::FWedge GroundFlight = SolidImperiousTower::FlightWedge(Plan, 0);
	const SolidImperiousTower::FWedge GroundGap = SolidImperiousTower::GapWedge(Plan, 0);
	const SolidImperiousTower::FWedge TopFlight = SolidImperiousTower::FlightWedge(Plan, 9);
	const SolidImperiousTower::FWedge TopGap = SolidImperiousTower::GapWedge(Plan, 9);
	TestTrue(TEXT("the ground floor has a flight"), GroundFlight.bOpen);
	TestFalse(TEXT("the ground floor has no hole"), GroundGap.bOpen);
	TestFalse(TEXT("the top floor has no flight"), TopFlight.bOpen);
	TestTrue(TEXT("the top floor has a hole"), TopGap.bOpen);
	TestTrue(
		TEXT("the second flight starts where the first ends"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightStartRad(Plan, 1),
			SolidImperiousTower::FlightStartRad(Plan, 0) + SolidImperiousTower::FlightSweepRad(Plan, 0),
			0.001f));
	TestTrue(
		TEXT("the hole is the flight below"),
		FMath::IsNearlyEqual(TopGap.StartRad, SolidImperiousTower::FlightStartRad(Plan, 8), 0.001f)
			&& FMath::IsNearlyEqual(TopGap.SweepRad, SolidImperiousTower::FlightSweepRad(Plan, 8), 0.001f));

	float Spiral = 0.f;
	for (int32 Floor = 0; Floor < Plan.FloorCount - 1; ++Floor)
	{
		Spiral += SolidImperiousTower::FlightSweepRad(Plan, Floor);
	}
	TestTrue(TEXT("the spiral is just over two turns"), Spiral > 2.f * PI && Spiral < 2.2f * PI);

	const int32 KeptCounts[] = { 34, 29, 22, 21, 14, 13, 13, 10, 6, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		SolidImperiousTowerTestPrivate::TestKeptHexes(*this, Plan, Floor, KeptCounts[Floor], bSawStair, bSawGap);
		bGroundStairLeftOut |= Floor == 0 && bSawStair;
		bGroundHadGap |= Floor == 0 && bSawGap;
		bTopHadStair |= Floor == 9 && bSawStair;
		bTopGapLeftOut |= Floor == 9 && bSawGap;
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);

	TArray<SolidStrongholdGrid::FHex> None;
	SolidImperiousTower::CollectKeptHexes(Plan, -1, None);
	TestEqual(TEXT("a floor below the ground is empty"), None.Num(), 0);
	SolidImperiousTower::CollectKeptHexes(Plan, Plan.FloorCount, None);
	TestEqual(TEXT("a floor above the cap is empty"), None.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerPlanMeasureTest,
	"SolidCore1.Stronghold.TowerPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerPlanMeasureTest::RunTest(const FString& Parameters)
{
	SolidImperiousTower::FPlan Plan = SolidImperiousTower::Imperious;
	Plan.FloorCount = 4;
	Plan.GroundExteriorRadiusCm = 400.f;
	Plan.TopExteriorRadiusCm = 280.f;

	TestEqual(TEXT("four floors"), Plan.FloorCount, 4);
	TestTrue(TEXT("the shaft is four storeys"), FMath::IsNearlyEqual(Plan.HeightCm(), 1200.f, 0.01f));
	TestTrue(
		TEXT("the ground interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(Plan.GroundInteriorRadiusCm(), 350.f, 0.01f));
	TestTrue(
		TEXT("the top interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(Plan.TopInteriorRadiusCm(), 230.f, 0.01f));
	TestTrue(
		TEXT("this plan is shorter than Imperious"),
		Plan.HeightCm() < SolidImperiousTower::Imperious.HeightCm());
	TestTrue(
		TEXT("this plan is narrower than Imperious"),
		Plan.GroundExteriorRadiusCm < SolidImperiousTower::Imperious.GroundExteriorRadiusCm);

	const float SampleZ[] = { 0.f, 600.f, 1200.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays the plan thickness"),
			FMath::IsNearlyEqual(
				SolidImperiousTower::ExteriorRadiusCm(Plan, ZCm) - SolidImperiousTower::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	const float GroundCenterline = 350.f - Plan.StairWidthCm * 0.5f;
	const float LastInterior = FMath::Lerp(350.f, 230.f, 600.f / 1200.f);
	const float LastCenterline = LastInterior - Plan.StairWidthCm * 0.5f;
	TestTrue(
		TEXT("the ground flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightSweepRad(Plan, 0),
			Plan.StairRunCm() / GroundCenterline,
			0.001f));
	TestTrue(
		TEXT("the last flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightSweepRad(Plan, 2),
			Plan.StairRunCm() / LastCenterline,
			0.001f));
	TestFalse(
		TEXT("the ground sweep is not the Imperious sweep"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightSweepRad(Plan, 0),
			SolidImperiousTower::FlightSweepRad(SolidImperiousTower::Imperious, 0),
			0.001f));
	TestTrue(
		TEXT("the second flight starts where the first ends"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightStartRad(Plan, 1),
			SolidImperiousTower::FlightStartRad(Plan, 0) + SolidImperiousTower::FlightSweepRad(Plan, 0),
			0.001f));
	TestTrue(TEXT("the ground floor has a flight"), SolidImperiousTower::FlightWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the ground floor has no hole"), SolidImperiousTower::GapWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the top floor has no flight"), SolidImperiousTower::FlightWedge(Plan, 3).bOpen);
	TestTrue(TEXT("the top floor has a hole"), SolidImperiousTower::GapWedge(Plan, 3).bOpen);

	const int32 KeptCounts[] = { 16, 10, 6, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		SolidImperiousTowerTestPrivate::TestKeptHexes(*this, Plan, Floor, KeptCounts[Floor], bSawStair, bSawGap);
		bGroundStairLeftOut |= Floor == 0 && bSawStair;
		bGroundHadGap |= Floor == 0 && bSawGap;
		bTopHadStair |= Floor == 3 && bSawStair;
		bTopGapLeftOut |= Floor == 3 && bSawGap;
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);

	SolidImperiousTower::FPlan Single = SolidImperiousTower::Imperious;
	Single.FloorCount = 1;
	Single.GroundExteriorRadiusCm = 350.f;
	Single.TopExteriorRadiusCm = 350.f;
	TestTrue(TEXT("a single floor is one storey"), FMath::IsNearlyEqual(Single.HeightCm(), 300.f, 0.01f));
	TestTrue(TEXT("the single floor has the door"), SolidImperiousTower::HasDoor(0));
	TestFalse(TEXT("a single floor has no flight"), SolidImperiousTower::FlightWedge(Single, 0).bOpen);
	TestFalse(TEXT("a single floor has no hole"), SolidImperiousTower::GapWedge(Single, 0).bOpen);
	bool bIgnoredStair = false;
	bool bIgnoredGap = false;
	SolidImperiousTowerTestPrivate::TestKeptHexes(*this, Single, 0, 13, bIgnoredStair, bIgnoredGap);
	TestFalse(TEXT("a single floor has no stair cell"), bIgnoredStair);
	TestFalse(TEXT("a single floor has no hole cell"), bIgnoredGap);

	SolidImperiousTower::FPlan Narrow = SolidImperiousTower::Imperious;
	Narrow.FloorCount = 2;
	Narrow.GroundExteriorRadiusCm = 100.f;
	Narrow.TopExteriorRadiusCm = 100.f;
	TestTrue(
		TEXT("a stair wider than the room has no sweep"),
		FMath::IsNearlyZero(SolidImperiousTower::FlightSweepRad(Narrow, 0)));
	TestFalse(TEXT("that flight stays closed"), SolidImperiousTower::FlightWedge(Narrow, 0).bOpen);
	TArray<SolidStrongholdGrid::FHex> Empty;
	SolidImperiousTower::CollectKeptHexes(Narrow, 0, Empty);
	TestEqual(TEXT("a room narrower than a hex keeps nothing"), Empty.Num(), 0);
	return true;
}

#endif
