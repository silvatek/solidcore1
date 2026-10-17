#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonLevel.h"
#include "SolidStronghold.h"
#include "SolidTowerStronghold.h"
#include "SolidStrongholdGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerStrongholdTestPrivate
{
	void TestKeptHexes(
		FAutomationTestBase& Test,
		const SolidTowerStronghold::FPlan& Plan,
		const int32 Floor,
		const int32 ExpectedCount,
		bool& bSawStair,
		bool& bSawGap)
	{
		TArray<SolidStrongholdGrid::FHex> Kept;
		SolidTowerStronghold::CollectKeptHexes(Plan, Floor, Kept);
		Test.TestEqual(TEXT("kept hexes on this floor"), Kept.Num(), ExpectedCount);

		const float Radius = SolidTowerStronghold::InteriorRadiusCm(Plan, SolidTowerStronghold::FloorZCm(Plan, Floor));
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
				SolidTowerStronghold::HexOnStair(Plan, Floor, Hex.Col, Hex.Row));
			Test.TestFalse(
				TEXT("a kept hex is not in the hole"),
				SolidTowerStronghold::HexOnGap(Plan, Floor, Hex.Col, Hex.Row));
		}

		const int32 Span = SolidTowerStronghold::HexSpan(Plan);
		for (int32 Row = -Span; Row <= Span; ++Row)
		{
			for (int32 Col = -Span; Col <= Span; ++Col)
			{
				const bool bKept = SolidTowerStronghold::HexKept(Plan, Floor, Col, Row);
				const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Col)) << 32) | static_cast<uint32>(Row);
				Test.TestEqual(TEXT("the list is the kept cells"), Listed.Contains(Key), bKept);
				if (SolidTowerStronghold::HexOnStair(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a stair cell is missing from the hex list"), bKept);
					bSawStair = true;
				}
				if (SolidTowerStronghold::HexOnGap(Plan, Floor, Col, Row))
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
					SolidTowerStronghold::HexKept(Plan, Floor, Col, Row));
			}
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerStrongholdMeasureTest,
	"SolidCore1.Stronghold.ImperiousTower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerStrongholdMeasureTest::RunTest(const FString& Parameters)
{
	const SolidTowerStronghold::FPlan& Plan = SolidTowerStronghold::Imperious;

	TestEqual(TEXT("ten floors"), Plan.FloorCount, 10);
	TestTrue(TEXT("each floor is 3 m"), FMath::IsNearlyEqual(Plan.FloorPitchCm, 300.f, 0.01f));
	TestTrue(TEXT("the shaft is 30 m"), FMath::IsNearlyEqual(Plan.HeightCm(), 3000.f, 0.01f));
	TestTrue(
		TEXT("the top floor's ceiling is the cap"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FloorZCm(Plan, Plan.FloorCount - 1) + Plan.FloorPitchCm,
			Plan.HeightCm(),
			0.01f));
	TestTrue(
		TEXT("the ground floor is at the base"),
		FMath::IsNearlyEqual(SolidTowerStronghold::FloorZCm(Plan, 0), 0.f, 0.01f));
	TestTrue(TEXT("the slab is 20 cm"), FMath::IsNearlyEqual(Plan.SlabCm, 20.f, 0.01f));
	TestTrue(
		TEXT("the clear room is the pitch minus the slab"),
		FMath::IsNearlyEqual(Plan.ClearHeightCm(), 280.f, 0.01f));

	TestTrue(
		TEXT("ground exterior diameter is 10 m"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ExteriorRadiusCm(Plan, 0.f) * 2.f, 1000.f, 0.01f));
	TestTrue(
		TEXT("ground interior diameter is 9 m"),
		FMath::IsNearlyEqual(SolidTowerStronghold::InteriorRadiusCm(Plan, 0.f) * 2.f, 900.f, 0.01f));
	TestTrue(
		TEXT("top exterior diameter is 6 m"),
		FMath::IsNearlyEqual(SolidTowerStronghold::ExteriorRadiusCm(Plan, Plan.HeightCm()) * 2.f, 600.f, 0.01f));
	TestTrue(
		TEXT("top interior diameter is 5 m"),
		FMath::IsNearlyEqual(SolidTowerStronghold::InteriorRadiusCm(Plan, Plan.HeightCm()) * 2.f, 500.f, 0.01f));

	const float SampleZ[] = { 0.f, 750.f, 1500.f, 2400.f, 3000.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays 50 cm"),
			FMath::IsNearlyEqual(
				SolidTowerStronghold::ExteriorRadiusCm(Plan, ZCm) - SolidTowerStronghold::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	TestTrue(
		TEXT("the door matches the cave width"),
		FMath::IsNearlyEqual(Plan.DoorWidthCm, SolidDungeonLevel::DoorwayWidthCm, 0.01f));
	TestTrue(
		TEXT("the door matches the cave height"),
		FMath::IsNearlyEqual(Plan.DoorHeightCm, SolidDungeonLevel::MouthHeightCm, 0.01f));
	TestTrue(TEXT("the ground floor has the door"), SolidTowerStronghold::HasDoor(0));
	TestFalse(TEXT("the floor above has no door"), SolidTowerStronghold::HasDoor(1));
	TestFalse(TEXT("the top floor has no door"), SolidTowerStronghold::HasDoor(Plan.FloorCount - 1));
	const FVector2D Outward = SolidTowerStronghold::DoorOutward(Plan);
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
		FMath::IsNearlyEqual(SolidTowerStronghold::FlightSweepRad(Plan, 0), GroundSweep, 0.001f));
	TestTrue(
		TEXT("the last flight is the run along the 2.2 m centerline"),
		FMath::IsNearlyEqual(SolidTowerStronghold::FlightSweepRad(Plan, 8), LastSweep, 0.001f));
	TestTrue(
		TEXT("the last flight sweeps farther than the ground flight"),
		SolidTowerStronghold::FlightSweepRad(Plan, 8) > SolidTowerStronghold::FlightSweepRad(Plan, 0));

	const SolidTowerStronghold::FWedge GroundFlight = SolidTowerStronghold::FlightWedge(Plan, 0);
	const SolidTowerStronghold::FWedge GroundGap = SolidTowerStronghold::GapWedge(Plan, 0);
	const SolidTowerStronghold::FWedge TopFlight = SolidTowerStronghold::FlightWedge(Plan, 9);
	const SolidTowerStronghold::FWedge TopGap = SolidTowerStronghold::GapWedge(Plan, 9);
	TestTrue(TEXT("the ground floor has a flight"), GroundFlight.bOpen);
	TestFalse(TEXT("the ground floor has no hole"), GroundGap.bOpen);
	TestFalse(TEXT("the top floor has no flight"), TopFlight.bOpen);
	TestTrue(TEXT("the top floor has a hole"), TopGap.bOpen);
	TestTrue(
		TEXT("the second flight starts where the first ends"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FlightStartRad(Plan, 1),
			SolidTowerStronghold::FlightStartRad(Plan, 0) + SolidTowerStronghold::FlightSweepRad(Plan, 0),
			0.001f));
	TestTrue(
		TEXT("the hole is the flight below"),
		FMath::IsNearlyEqual(TopGap.StartRad, SolidTowerStronghold::FlightStartRad(Plan, 8), 0.001f)
			&& FMath::IsNearlyEqual(TopGap.SweepRad, SolidTowerStronghold::FlightSweepRad(Plan, 8), 0.001f));

	float Spiral = 0.f;
	for (int32 Floor = 0; Floor < Plan.FloorCount - 1; ++Floor)
	{
		Spiral += SolidTowerStronghold::FlightSweepRad(Plan, Floor);
	}
	const float Turn = 2.f * PI;
	TestTrue(TEXT("the spiral is just over two turns"), Spiral > 2.f * Turn && Spiral < 2.2f * Turn);

	const int32 KeptCounts[] = { 34, 29, 22, 21, 14, 13, 13, 10, 6, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		SolidTowerStrongholdTestPrivate::TestKeptHexes(*this, Plan, Floor, KeptCounts[Floor], bSawStair, bSawGap);
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
	SolidTowerStronghold::CollectKeptHexes(Plan, -1, None);
	TestEqual(TEXT("a floor below the ground is empty"), None.Num(), 0);
	SolidTowerStronghold::CollectKeptHexes(Plan, Plan.FloorCount, None);
	TestEqual(TEXT("a floor above the cap is empty"), None.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerPlanMeasureTest,
	"SolidCore1.Stronghold.TowerPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerPlanMeasureTest::RunTest(const FString& Parameters)
{
	const SolidTowerStronghold::FPlan& Plan = SolidTowerStronghold::Average;

	TestEqual(TEXT("Average is four floors"), Plan.FloorCount, 4);
	TestTrue(TEXT("the shaft is four storeys"), FMath::IsNearlyEqual(Plan.HeightCm(), 1200.f, 0.01f));
	TestTrue(
		TEXT("the ground interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(Plan.GroundInteriorRadiusCm(), 350.f, 0.01f));
	TestTrue(
		TEXT("the top interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(Plan.TopInteriorRadiusCm(), 230.f, 0.01f));
	TestTrue(
		TEXT("Average is shorter than Imperious"),
		Plan.HeightCm() < SolidTowerStronghold::Imperious.HeightCm());
	TestTrue(
		TEXT("Average is narrower than Imperious"),
		Plan.GroundExteriorRadiusCm < SolidTowerStronghold::Imperious.GroundExteriorRadiusCm);
	TestTrue(
		TEXT("Average keeps the Imperious storey"),
		FMath::IsNearlyEqual(Plan.FloorPitchCm, SolidTowerStronghold::Imperious.FloorPitchCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious wall"),
		FMath::IsNearlyEqual(Plan.WallCm, SolidTowerStronghold::Imperious.WallCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious door"),
		FMath::IsNearlyEqual(Plan.DoorWidthCm, SolidTowerStronghold::Imperious.DoorWidthCm, 0.01f)
			&& FMath::IsNearlyEqual(Plan.DoorHeightCm, SolidTowerStronghold::Imperious.DoorHeightCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious stair"),
		Plan.StairRiserCount == SolidTowerStronghold::Imperious.StairRiserCount
			&& FMath::IsNearlyEqual(Plan.StairWidthCm, SolidTowerStronghold::Imperious.StairWidthCm, 0.01f)
			&& FMath::IsNearlyEqual(Plan.StairRunCm(), SolidTowerStronghold::Imperious.StairRunCm(), 0.01f));

	FSolidStronghold AverageTower;
	TestTrue(TEXT("Average Tower resolves"), SolidStronghold::FromToken(TEXT("Average Tower"), AverageTower));
	SolidTowerStronghold::FPlan Resolved;
	TestTrue(TEXT("Average Tower has this plan"), SolidTowerStronghold::PlanFor(AverageTower, Resolved));
	TestEqual(TEXT("the resolved plan is Average"), Resolved.FloorCount, Plan.FloorCount);
	TestTrue(
		TEXT("the resolved ground matches Average"),
		FMath::IsNearlyEqual(Resolved.GroundExteriorRadiusCm, Plan.GroundExteriorRadiusCm, 0.01f));
	TestTrue(
		TEXT("the resolved cap matches Average"),
		FMath::IsNearlyEqual(Resolved.TopExteriorRadiusCm, Plan.TopExteriorRadiusCm, 0.01f));

	FSolidStronghold ImperiousTower;
	TestTrue(TEXT("Imperious Tower resolves"), SolidStronghold::FromToken(TEXT("Imperious Tower"), ImperiousTower));
	TestTrue(TEXT("Imperious Tower has its plan"), SolidTowerStronghold::PlanFor(ImperiousTower, Resolved));
	TestEqual(TEXT("the resolved plan is Imperious"), Resolved.FloorCount, SolidTowerStronghold::Imperious.FloorCount);

	FSolidStronghold HumbleTower;
	TestTrue(TEXT("Humble Tower resolves"), SolidStronghold::FromToken(TEXT("Humble Tower"), HumbleTower));
	Resolved = Plan;
	TestFalse(TEXT("Humble Tower has no plan"), SolidTowerStronghold::PlanFor(HumbleTower, Resolved));
	TestEqual(TEXT("a missing plan leaves the previous value"), Resolved.FloorCount, Plan.FloorCount);
	TestFalse(
		TEXT("a dungeon has no tower plan"),
		SolidTowerStronghold::PlanFor(SolidStronghold::Starting, Resolved));
	TestEqual(TEXT("a dungeon leaves the previous value"), Resolved.FloorCount, Plan.FloorCount);

	const float SampleZ[] = { 0.f, 600.f, 1200.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays the plan thickness"),
			FMath::IsNearlyEqual(
				SolidTowerStronghold::ExteriorRadiusCm(Plan, ZCm) - SolidTowerStronghold::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	const float GroundCenterline = 350.f - Plan.StairWidthCm * 0.5f;
	const float LastInterior = FMath::Lerp(350.f, 230.f, 600.f / 1200.f);
	const float LastCenterline = LastInterior - Plan.StairWidthCm * 0.5f;
	TestTrue(
		TEXT("the ground flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FlightSweepRad(Plan, 0),
			Plan.StairRunCm() / GroundCenterline,
			0.001f));
	TestTrue(
		TEXT("the last flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FlightSweepRad(Plan, 2),
			Plan.StairRunCm() / LastCenterline,
			0.001f));
	TestFalse(
		TEXT("the ground sweep is not the Imperious sweep"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FlightSweepRad(Plan, 0),
			SolidTowerStronghold::FlightSweepRad(SolidTowerStronghold::Imperious, 0),
			0.001f));
	TestTrue(
		TEXT("the second flight starts where the first ends"),
		FMath::IsNearlyEqual(
			SolidTowerStronghold::FlightStartRad(Plan, 1),
			SolidTowerStronghold::FlightStartRad(Plan, 0) + SolidTowerStronghold::FlightSweepRad(Plan, 0),
			0.001f));
	TestTrue(TEXT("the ground floor has a flight"), SolidTowerStronghold::FlightWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the ground floor has no hole"), SolidTowerStronghold::GapWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the top floor has no flight"), SolidTowerStronghold::FlightWedge(Plan, 3).bOpen);
	TestTrue(TEXT("the top floor has a hole"), SolidTowerStronghold::GapWedge(Plan, 3).bOpen);

	const int32 KeptCounts[] = { 16, 10, 6, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		SolidTowerStrongholdTestPrivate::TestKeptHexes(*this, Plan, Floor, KeptCounts[Floor], bSawStair, bSawGap);
		bGroundStairLeftOut |= Floor == 0 && bSawStair;
		bGroundHadGap |= Floor == 0 && bSawGap;
		bTopHadStair |= Floor == 3 && bSawStair;
		bTopGapLeftOut |= Floor == 3 && bSawGap;
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);

	SolidTowerStronghold::FPlan Single = SolidTowerStronghold::Imperious;
	Single.FloorCount = 1;
	Single.GroundExteriorRadiusCm = 350.f;
	Single.TopExteriorRadiusCm = 350.f;
	TestTrue(TEXT("a single floor is one storey"), FMath::IsNearlyEqual(Single.HeightCm(), 300.f, 0.01f));
	TestTrue(TEXT("the single floor has the door"), SolidTowerStronghold::HasDoor(0));
	TestFalse(TEXT("a single floor has no flight"), SolidTowerStronghold::FlightWedge(Single, 0).bOpen);
	TestFalse(TEXT("a single floor has no hole"), SolidTowerStronghold::GapWedge(Single, 0).bOpen);
	bool bIgnoredStair = false;
	bool bIgnoredGap = false;
	SolidTowerStrongholdTestPrivate::TestKeptHexes(*this, Single, 0, 13, bIgnoredStair, bIgnoredGap);
	TestFalse(TEXT("a single floor has no stair cell"), bIgnoredStair);
	TestFalse(TEXT("a single floor has no hole cell"), bIgnoredGap);

	SolidTowerStronghold::FPlan Narrow = SolidTowerStronghold::Imperious;
	Narrow.FloorCount = 2;
	Narrow.GroundExteriorRadiusCm = 100.f;
	Narrow.TopExteriorRadiusCm = 100.f;
	TestTrue(
		TEXT("a stair wider than the room has no sweep"),
		FMath::IsNearlyZero(SolidTowerStronghold::FlightSweepRad(Narrow, 0)));
	TestFalse(TEXT("that flight stays closed"), SolidTowerStronghold::FlightWedge(Narrow, 0).bOpen);
	TArray<SolidStrongholdGrid::FHex> Empty;
	SolidTowerStronghold::CollectKeptHexes(Narrow, 0, Empty);
	TestEqual(TEXT("a room narrower than a hex keeps nothing"), Empty.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerStepMeasureTest,
	"SolidCore1.Stronghold.TowerSteps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerStepMeasureTest::RunTest(const FString& Parameters)
{
	using namespace SolidTowerStronghold;

	TestTrue(TEXT("Average is the low end"), FMath::IsNearlyZero(SizeAlong(ESolidStrongholdSize::Average), 0.001f));
	TestTrue(
		TEXT("Impressive is one quarter"),
		FMath::IsNearlyEqual(SizeAlong(ESolidStrongholdSize::Impressive), 0.25f, 0.001f));
	TestTrue(
		TEXT("Grand is halfway"),
		FMath::IsNearlyEqual(SizeAlong(ESolidStrongholdSize::Grand), 0.5f, 0.001f));
	TestTrue(
		TEXT("Stately is three quarters"),
		FMath::IsNearlyEqual(SizeAlong(ESolidStrongholdSize::Stately), 0.75f, 0.001f));
	TestTrue(
		TEXT("Imperious is the high end"),
		FMath::IsNearlyEqual(SizeAlong(ESolidStrongholdSize::Imperious), 1.f, 0.001f));

	const FPlan Steps[] = { Average, Impressive, Grand, Stately, Imperious };
	const ESolidStrongholdSize Sizes[] = {
		ESolidStrongholdSize::Average,
		ESolidStrongholdSize::Impressive,
		ESolidStrongholdSize::Grand,
		ESolidStrongholdSize::Stately,
		ESolidStrongholdSize::Imperious,
	};
	const TCHAR* Tokens[] = {
		TEXT("Average Tower"),
		TEXT("Impressive Tower"),
		TEXT("Grand Tower"),
		TEXT("Stately Tower"),
		TEXT("Imperious Tower"),
	};
	const int32 FloorCounts[] = { 4, 6, 7, 9, 10 };
	const float GroundRadii[] = { 400.f, 425.f, 450.f, 475.f, 500.f };
	const float TopRadii[] = { 280.f, 285.f, 290.f, 295.f, 300.f };
	const int32 KeptImpressive[] = { 16, 13, 10, 10, 6, 5 };
	const int32 KeptGrand[] = { 26, 14, 14, 13, 10, 6, 5 };
	const int32 KeptStately[] = { 33, 22, 14, 14, 13, 10, 10, 6, 5 };
	const int32* KeptByStep[] = { nullptr, KeptImpressive, KeptGrand, KeptStately, nullptr };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Steps); ++Index)
	{
		const FPlan& Plan = Steps[Index];
		const FPlan Blended = LerpPlan(Average, Imperious, SizeAlong(Sizes[Index]));
		TestEqual(TEXT("the step has the blended floor count"), Plan.FloorCount, Blended.FloorCount);
		TestEqual(TEXT("the step has the listed floor count"), Plan.FloorCount, FloorCounts[Index]);
		TestTrue(
			TEXT("the step has the blended ground radius"),
			FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, Blended.GroundExteriorRadiusCm, 0.01f));
		TestTrue(
			TEXT("the step has the listed ground radius"),
			FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, GroundRadii[Index], 0.01f));
		TestTrue(
			TEXT("the step has the blended cap radius"),
			FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, Blended.TopExteriorRadiusCm, 0.01f));
		TestTrue(
			TEXT("the step has the listed cap radius"),
			FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, TopRadii[Index], 0.01f));
		TestTrue(
			TEXT("the step keeps the shared storey"),
			FMath::IsNearlyEqual(Plan.FloorPitchCm, Imperious.FloorPitchCm, 0.01f));
		TestTrue(
			TEXT("the step keeps the shared wall"),
			FMath::IsNearlyEqual(Plan.WallCm, Imperious.WallCm, 0.01f));
		if (Index > 0)
		{
			TestTrue(TEXT("floors rise toward Imperious"), Plan.FloorCount > Steps[Index - 1].FloorCount);
			TestTrue(
				TEXT("the base widens toward Imperious"),
				Plan.GroundExteriorRadiusCm > Steps[Index - 1].GroundExteriorRadiusCm);
		}

		FSolidStronghold Stronghold;
		TestTrue(TEXT("the size token resolves"), SolidStronghold::FromToken(Tokens[Index], Stronghold));
		FPlan Resolved;
		TestTrue(TEXT("the size has this plan"), PlanFor(Stronghold, Resolved));
		TestEqual(TEXT("PlanFor returns the step"), Resolved.FloorCount, Plan.FloorCount);
		TestTrue(
			TEXT("PlanFor returns the step's base"),
			FMath::IsNearlyEqual(Resolved.GroundExteriorRadiusCm, Plan.GroundExteriorRadiusCm, 0.01f));

		if (KeptByStep[Index] == nullptr)
		{
			continue;
		}
		bool bGroundStair = false;
		bool bTopGap = false;
		bool bGroundGap = false;
		bool bTopStair = false;
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			bool bSawStair = false;
			bool bSawGap = false;
			SolidTowerStrongholdTestPrivate::TestKeptHexes(
				*this, Plan, Floor, KeptByStep[Index][Floor], bSawStair, bSawGap);
			bGroundStair |= Floor == 0 && bSawStair;
			bGroundGap |= Floor == 0 && bSawGap;
			bTopStair |= Floor == Plan.FloorCount - 1 && bSawStair;
			bTopGap |= Floor == Plan.FloorCount - 1 && bSawGap;
		}
		TestTrue(TEXT("the ground stair is left out"), bGroundStair);
		TestFalse(TEXT("the ground floor has no hole cell"), bGroundGap);
		TestTrue(TEXT("the top hole is left out"), bTopGap);
		TestFalse(TEXT("the top floor has no stair cell"), bTopStair);
	}
	return true;
}

#endif
