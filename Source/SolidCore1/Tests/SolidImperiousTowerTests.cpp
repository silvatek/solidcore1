#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonLevel.h"
#include "SolidImperiousTower.h"
#include "SolidStrongholdGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidImperiousTowerMeasureTest,
	"SolidCore1.Stronghold.ImperiousTower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidImperiousTowerMeasureTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("ten floors"), SolidImperiousTower::FloorCount, 10);
	TestTrue(
		TEXT("each floor is 3 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::FloorPitchCm, 300.f, 0.01f));
	TestTrue(
		TEXT("the shaft is 30 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::HeightCm, 3000.f, 0.01f));
	TestTrue(
		TEXT("the top floor's ceiling is the cap"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FloorZCm(SolidImperiousTower::FloorCount - 1) + SolidImperiousTower::FloorPitchCm,
			SolidImperiousTower::HeightCm,
			0.01f));
	TestTrue(
		TEXT("the ground floor is at the base"),
		FMath::IsNearlyEqual(SolidImperiousTower::FloorZCm(0), 0.f, 0.01f));
	TestTrue(
		TEXT("the slab is 20 cm"),
		FMath::IsNearlyEqual(SolidImperiousTower::SlabCm, 20.f, 0.01f));
	TestTrue(
		TEXT("the clear room is the pitch minus the slab"),
		FMath::IsNearlyEqual(SolidImperiousTower::ClearHeightCm, 280.f, 0.01f));

	TestTrue(
		TEXT("ground exterior diameter is 10 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::ExteriorRadiusCm(0.f) * 2.f, 1000.f, 0.01f));
	TestTrue(
		TEXT("ground interior diameter is 9 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::InteriorRadiusCm(0.f) * 2.f, 900.f, 0.01f));
	TestTrue(
		TEXT("top exterior diameter is 6 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::ExteriorRadiusCm(SolidImperiousTower::HeightCm) * 2.f, 600.f, 0.01f));
	TestTrue(
		TEXT("top interior diameter is 5 m"),
		FMath::IsNearlyEqual(SolidImperiousTower::InteriorRadiusCm(SolidImperiousTower::HeightCm) * 2.f, 500.f, 0.01f));

	const float SampleZ[] = { 0.f, 750.f, 1500.f, 2400.f, 3000.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays 50 cm"),
			FMath::IsNearlyEqual(
				SolidImperiousTower::ExteriorRadiusCm(ZCm) - SolidImperiousTower::InteriorRadiusCm(ZCm),
				SolidImperiousTower::WallCm,
				0.01f));
	}

	TestTrue(
		TEXT("the door matches the cave width"),
		FMath::IsNearlyEqual(SolidImperiousTower::DoorWidthCm, SolidDungeonLevel::DoorwayWidthCm, 0.01f));
	TestTrue(
		TEXT("the door matches the cave height"),
		FMath::IsNearlyEqual(SolidImperiousTower::DoorHeightCm, SolidDungeonLevel::MouthHeightCm, 0.01f));
	TestTrue(TEXT("the ground floor has the door"), SolidImperiousTower::HasDoor(0));
	TestFalse(TEXT("the floor above has no door"), SolidImperiousTower::HasDoor(1));
	TestFalse(TEXT("the top floor has no door"), SolidImperiousTower::HasDoor(SolidImperiousTower::FloorCount - 1));
	const FVector2D Outward = SolidImperiousTower::DoorOutward();
	TestTrue(
		TEXT("the door faces local -X"),
		FMath::IsNearlyEqual(Outward.X, -1.f, 0.01f) && FMath::IsNearlyZero(Outward.Y, 0.01f));

	TestEqual(TEXT("seventeen risers"), SolidImperiousTower::StairRiserCount, 17);
	TestEqual(TEXT("sixteen treads"), SolidImperiousTower::StairTreadCount, 16);
	TestTrue(
		TEXT("a tread is 26 cm"),
		FMath::IsNearlyEqual(SolidImperiousTower::StairTreadCm, 26.f, 0.01f));
	TestTrue(
		TEXT("the run is sixteen treads"),
		FMath::IsNearlyEqual(SolidImperiousTower::StairRunCm, 416.f, 0.01f));
	TestTrue(
		TEXT("a riser is the pitch split seventeen ways"),
		FMath::IsNearlyEqual(SolidImperiousTower::RiserCm(), 300.f / 17.f, 0.01f));
	TestTrue(
		TEXT("the stair is as wide as the door"),
		FMath::IsNearlyEqual(SolidImperiousTower::StairWidthCm, SolidImperiousTower::DoorWidthCm, 0.01f));

	const float GroundSweep = 416.f / (450.f - 70.f);
	const float LastSweep = 416.f / (290.f - 70.f);
	TestTrue(
		TEXT("the ground flight is the run along the 3.8 m centerline"),
		FMath::IsNearlyEqual(SolidImperiousTower::FlightSweepRad(0), GroundSweep, 0.001f));
	TestTrue(
		TEXT("the last flight is the run along the 2.2 m centerline"),
		FMath::IsNearlyEqual(SolidImperiousTower::FlightSweepRad(8), LastSweep, 0.001f));
	TestTrue(
		TEXT("the last flight sweeps farther than the ground flight"),
		SolidImperiousTower::FlightSweepRad(8) > SolidImperiousTower::FlightSweepRad(0));

	const SolidImperiousTower::FWedge GroundFlight = SolidImperiousTower::FlightWedge(0);
	const SolidImperiousTower::FWedge GroundGap = SolidImperiousTower::GapWedge(0);
	const SolidImperiousTower::FWedge TopFlight = SolidImperiousTower::FlightWedge(9);
	const SolidImperiousTower::FWedge TopGap = SolidImperiousTower::GapWedge(9);
	TestTrue(TEXT("the ground floor has a flight"), GroundFlight.bOpen);
	TestFalse(TEXT("the ground floor has no hole"), GroundGap.bOpen);
	TestFalse(TEXT("the top floor has no flight"), TopFlight.bOpen);
	TestTrue(TEXT("the top floor has a hole"), TopGap.bOpen);
	TestTrue(
		TEXT("the second flight starts where the first ends"),
		FMath::IsNearlyEqual(
			SolidImperiousTower::FlightStartRad(1),
			SolidImperiousTower::FlightStartRad(0) + SolidImperiousTower::FlightSweepRad(0),
			0.001f));
	TestTrue(
		TEXT("the hole is the flight below"),
		FMath::IsNearlyEqual(TopGap.StartRad, SolidImperiousTower::FlightStartRad(8), 0.001f)
			&& FMath::IsNearlyEqual(TopGap.SweepRad, SolidImperiousTower::FlightSweepRad(8), 0.001f));

	float Spiral = 0.f;
	for (int32 Floor = 0; Floor < SolidImperiousTower::FloorCount - 1; ++Floor)
	{
		Spiral += SolidImperiousTower::FlightSweepRad(Floor);
	}
	TestTrue(TEXT("the spiral is just over two turns"), Spiral > 2.f * PI && Spiral < 2.2f * PI);

	const int32 KeptCounts[] = { 34, 29, 22, 21, 14, 13, 13, 10, 6, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	for (int32 Floor = 0; Floor < SolidImperiousTower::FloorCount; ++Floor)
	{
		TArray<SolidStrongholdGrid::FHex> Kept;
		SolidImperiousTower::CollectKeptHexes(Floor, Kept);
		TestEqual(TEXT("kept hexes on this floor"), Kept.Num(), KeptCounts[Floor]);

		const float Radius = SolidImperiousTower::InteriorRadiusCm(SolidImperiousTower::FloorZCm(Floor));
		TSet<uint64> Listed;
		for (const SolidStrongholdGrid::FHex& Hex : Kept)
		{
			Listed.Add((static_cast<uint64>(static_cast<uint32>(Hex.Col)) << 32) | static_cast<uint32>(Hex.Row));
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
			for (const FVector2D& Vertex : Vertices)
			{
				TestTrue(TEXT("a kept vertex stays inside the floor circle"), Vertex.Size() <= Radius + 0.01f);
			}
			TestFalse(TEXT("a kept hex is not on the stair"), SolidImperiousTower::HexOnStair(Floor, Hex.Col, Hex.Row));
			TestFalse(TEXT("a kept hex is not in the hole"), SolidImperiousTower::HexOnGap(Floor, Hex.Col, Hex.Row));
		}

		for (int32 Row = -12; Row <= 12; ++Row)
		{
			for (int32 Col = -12; Col <= 12; ++Col)
			{
				const bool bKept = SolidImperiousTower::HexKept(Floor, Col, Row);
				const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Col)) << 32) | static_cast<uint32>(Row);
				TestEqual(TEXT("the list is the kept cells"), Listed.Contains(Key), bKept);
				if (SolidImperiousTower::HexOnStair(Floor, Col, Row))
				{
					TestFalse(TEXT("a stair cell is missing from the hex list"), bKept);
					bGroundStairLeftOut |= Floor == 0;
					bTopHadStair |= Floor == 9;
				}
				if (SolidImperiousTower::HexOnGap(Floor, Col, Row))
				{
					TestFalse(TEXT("a hole cell is missing from the hex list"), bKept);
					bTopGapLeftOut |= Floor == 9;
					bGroundHadGap |= Floor == 0;
				}
			}
		}
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);

	TArray<SolidStrongholdGrid::FHex> None;
	SolidImperiousTower::CollectKeptHexes(-1, None);
	TestEqual(TEXT("a floor below the ground is empty"), None.Num(), 0);
	SolidImperiousTower::CollectKeptHexes(SolidImperiousTower::FloorCount, None);
	TestEqual(TEXT("a floor above the cap is empty"), None.Num(), 0);
	return true;
}

#endif
