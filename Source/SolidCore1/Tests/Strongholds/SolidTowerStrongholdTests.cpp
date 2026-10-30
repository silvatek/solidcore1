#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidStronghold.h"
#include "Strongholds/Furnishings/SolidFloorSign.h"
#include "Strongholds/Furnishings/SolidHandrail.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerStrongholdTestPrivate
{
	void TestKeptHexes(
		FAutomationTestBase& Test,
		const SolidStronghold::FPlan& Plan,
		const int32 Floor,
		const int32 ExpectedCount,
		const int32 CenteredCount,
		bool& bSawStair,
		bool& bSawGap,
		bool& bSawLanding)
	{
		TArray<SolidStrongholdGrid::FHex> Kept;
		SolidStronghold::CollectKeptHexes(Plan, Floor, Kept);
		Test.TestEqual(TEXT("kept hexes on this floor"), Kept.Num(), ExpectedCount);

		TArray<SolidStrongholdGrid::FHex> Centered;
		SolidStronghold::CollectKeptHexes(Plan, Floor, Centered, FVector2D::ZeroVector);
		Test.TestEqual(TEXT("the centered grid keeps its count"), Centered.Num(), CenteredCount);
		Test.TestTrue(
			TEXT("the shift keeps at least as many hexes as the centered grid"),
			Kept.Num() >= Centered.Num());
		const FVector2D Nudge = SolidStronghold::GridNudgeCm(Plan, Floor);
		Test.TestTrue(TEXT("the hex shift is at most 30 cm"), Nudge.Size() <= 30.01f);
		if (Nudge.IsNearlyZero(0.01f))
		{
			Test.TestEqual(TEXT("a centered floor keeps the centered count"), Kept.Num(), Centered.Num());
		}
		else
		{
			Test.TestTrue(TEXT("a shifted floor keeps another hex"), Kept.Num() > Centered.Num());
		}

		const float Radius = SolidStronghold::InteriorRadiusCm(Plan, SolidStronghold::FloorZCm(Plan, Floor));
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
				SolidStronghold::HexOnStair(Plan, Floor, Hex.Col, Hex.Row));
			Test.TestFalse(
				TEXT("a kept hex is not in the hole"),
				SolidStronghold::HexOnGap(Plan, Floor, Hex.Col, Hex.Row));
			Test.TestFalse(
				TEXT("a kept hex is not on the landing"),
				SolidStronghold::HexOnLanding(Plan, Floor, Hex.Col, Hex.Row));
		}

		const int32 Span = SolidStronghold::HexSpan(Plan);
		for (int32 Row = -Span; Row <= Span; ++Row)
		{
			for (int32 Col = -Span; Col <= Span; ++Col)
			{
				const bool bKept = SolidStronghold::HexKept(Plan, Floor, Col, Row);
				const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Col)) << 32) | static_cast<uint32>(Row);
				Test.TestEqual(TEXT("the list is the kept cells"), Listed.Contains(Key), bKept);
				if (SolidStronghold::HexOnStair(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a stair cell is missing from the hex list"), bKept);
					bSawStair = true;
				}
				if (SolidStronghold::HexOnGap(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a hole cell is missing from the hex list"), bKept);
					bSawGap = true;
				}
				if (SolidStronghold::HexOnLanding(Plan, Floor, Col, Row))
				{
					Test.TestFalse(TEXT("a landing cell is missing from the hex list"), bKept);
					bSawLanding = true;
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
					SolidStronghold::HexKept(Plan, Floor, Col, Row));
			}
		}
	}

	void TestRails(FAutomationTestBase& Test, const SolidStronghold::FPlan& Plan)
	{
		auto AngleOf = [](const SolidStronghold::FSolidHandrail::FPiece& Piece)
		{
			return FMath::Atan2(Piece.Location.Y, Piece.Location.X);
		};
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			TArray<SolidStronghold::FSolidHandrail::FPiece> Hole;
			TArray<SolidStronghold::FSolidHandrail::FPiece> Flight;
			SolidStronghold::CollectHoleRail(Plan, Floor, Hole);
			SolidStronghold::CollectFlightRail(Plan, Floor, Flight);
			const SolidStronghold::FWedge Gap = SolidStronghold::GapWedge(Plan, Floor);
			const SolidStronghold::FWedge Climb = SolidStronghold::FlightWedge(Plan, Floor);
			const SolidStronghold::FWedge Landing = SolidStronghold::LandingWedge(Plan, Floor);
			const float FloorZ = SolidStronghold::FloorZCm(Plan, Floor);
			const float Inner = SolidStronghold::RailInnerRadiusCm(Plan, FloorZ);

			if (!Gap.bOpen)
			{
				Test.TestEqual(TEXT("a floor with no hole has no hole rail"), Hole.Num(), 0);
			}
			else
			{
				Test.TestTrue(TEXT("an arrival hole has a rail"), Hole.Num() > 0);
				bool bClosesFarEnd = false;
				float HolePostMinZ = TNumericLimits<float>::Max();
				float HolePostMaxZ = TNumericLimits<float>::Lowest();
				int32 HolePosts = 0;
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Hole)
				{
					const float Angle = AngleOf(Piece);
					const float AlongHole = SolidStronghold::WrapTau(Angle - Gap.StartRad);
					Test.TestTrue(
						TEXT("the hole rail stays on the hole"),
						AlongHole <= Gap.SweepRad + 0.002f);
					if (Landing.bOpen)
					{
						const float Into = SolidStronghold::WrapTau(Angle - Landing.StartRad);
						const float Margin = (SolidStronghold::FSolidHandrail::BarCm * 0.5f) / FMath::Max(Inner, 1.f) + 0.02f;
						Test.TestFalse(
							TEXT("the hole rail stops for the landing"),
							Into > Margin && Into < Landing.SweepRad - Margin);
					}
					if (Piece.bHoleEnd)
					{
						bClosesFarEnd = true;
						Test.TestTrue(
							TEXT("the hole's far edge is the end away from the landing"),
							SolidStronghold::WrapTau(Angle - Gap.StartRad) < 0.05f);
						Test.TestTrue(
							TEXT("the landing end of the hole stays open"),
							SolidStronghold::WrapTau(Angle - (Gap.StartRad + Gap.SweepRad)) > 0.2f);
					}
					else if (!Piece.bPost)
					{
						Test.TestTrue(
							TEXT("the hole rail is waist height"),
							FMath::IsNearlyEqual(Piece.Location.Z, FloorZ + SolidStronghold::FSolidHandrail::HeightCm, 0.5f));
						Test.TestTrue(
							TEXT("the hole rail follows the inner edge"),
							FMath::IsNearlyEqual(FVector2D(Piece.Location.X, Piece.Location.Y).Size(), Inner, SolidStronghold::FSolidHandrail::BarCm));
					}
					if (Piece.bPost)
					{
						HolePostMinZ = FMath::Min(HolePostMinZ, Piece.Location.Z);
						HolePostMaxZ = FMath::Max(HolePostMaxZ, Piece.Location.Z);
						++HolePosts;
						Test.TestTrue(
							TEXT("a hole post reaches the rail"),
							FMath::IsNearlyEqual(
								Piece.SizeCm.Z,
								SolidStronghold::FSolidHandrail::HeightCm - SolidStronghold::FSolidHandrail::BarCm * 0.5f,
								0.5f));
					}
				}
				Test.TestTrue(TEXT("the hole's far radial edge is closed"), bClosesFarEnd);
				Test.TestTrue(TEXT("the hole rail has posts"), HolePosts > 0);
				Test.TestTrue(TEXT("the hole rail stays level"), HolePostMaxZ - HolePostMinZ < 0.5f);
				const float Mid = Gap.StartRad + Gap.SweepRad * 0.5f;
				const FVector StepOff = SolidStronghold::FSolidHandrail::Point(
					Mid, Inner, FloorZ + SolidStronghold::FSolidHandrail::HeightCm);
				float Nearest = TNumericLimits<float>::Max();
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Hole)
				{
					if (!Piece.bPost && !Piece.bHoleEnd)
					{
						Nearest = FMath::Min(Nearest, FVector::Dist(Piece.Location, StepOff));
					}
				}
				Test.TestTrue(TEXT("the hole rail blocks a step off the hole"), Nearest < SolidStronghold::FSolidHandrail::SpanCm);
			}

			if (!Climb.bOpen)
			{
				Test.TestEqual(TEXT("a floor with no flight has no flight rail"), Flight.Num(), 0);
			}
			else
			{
				Test.TestTrue(TEXT("a climbing flight has a rail"), Flight.Num() > 0);
				const float ClimbRise = static_cast<float>(Plan.StairTreadCount) * SolidStronghold::RiserCm(Plan);
				const SolidStronghold::FSolidHandrail::FPiece* FirstPost = nullptr;
				const SolidStronghold::FSolidHandrail::FPiece* LastPost = nullptr;
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Flight)
				{
					Test.TestFalse(TEXT("the flight rail is not the hole's end"), Piece.bHoleEnd);
					const float Angle = AngleOf(Piece);
					const float AlongFlight = SolidStronghold::WrapTau(Angle - Climb.StartRad);
					Test.TestTrue(
						TEXT("the flight rail stays on the flight"),
						AlongFlight <= Climb.SweepRad + 0.002f);
					if (Landing.bOpen)
					{
						const float Into = SolidStronghold::WrapTau(Angle - Landing.StartRad);
						const float Margin = (SolidStronghold::FSolidHandrail::BarCm * 0.5f) / FMath::Max(Inner, 1.f) + 0.02f;
						Test.TestFalse(
							TEXT("the flight rail stops for the landing"),
							Into > Margin && Into < Landing.SweepRad - Margin);
					}
					if (!Piece.bPost)
					{
						Test.TestTrue(
							TEXT("the flight rail follows the inner edge"),
							FMath::IsNearlyEqual(FVector2D(Piece.Location.X, Piece.Location.Y).Size(), Inner, SolidStronghold::FSolidHandrail::BarCm));
					}
					else
					{
						if (!FirstPost)
						{
							FirstPost = &Piece;
						}
						LastPost = &Piece;
					}
				}
				Test.TestTrue(TEXT("the flight rail has a first post"), FirstPost != nullptr);
				Test.TestTrue(TEXT("the flight rail has a last post"), LastPost != nullptr);
				if (FirstPost && LastPost)
				{
					Test.TestTrue(
						TEXT("the flight rail rises with the treads"),
						FMath::IsNearlyEqual(LastPost->Location.Z - FirstPost->Location.Z, ClimbRise, 0.5f));
					Test.TestTrue(
						TEXT("the flight rail starts at waist height"),
						FMath::IsNearlyEqual(
							FirstPost->Location.Z + FirstPost->SizeCm.Z * 0.5f,
							FloorZ + SolidStronghold::FSolidHandrail::HeightCm - SolidStronghold::FSolidHandrail::BarCm * 0.5f,
							0.5f));
				}
				const float Mid = Climb.StartRad + Climb.SweepRad * 0.5f;
				const float MidSurface = FloorZ + ClimbRise * 0.5f;
				const FVector StepOff = SolidStronghold::FSolidHandrail::Point(
					Mid, Inner, MidSurface + SolidStronghold::FSolidHandrail::HeightCm);
				float Nearest = TNumericLimits<float>::Max();
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Flight)
				{
					if (!Piece.bPost)
					{
						Nearest = FMath::Min(Nearest, FVector::Dist(Piece.Location, StepOff));
					}
				}
				Test.TestTrue(TEXT("the flight rail blocks a step off the inner edge"), Nearest < SolidStronghold::FSolidHandrail::SpanCm);
			}

			if (Landing.bOpen)
			{
				const float Mid = Landing.StartRad + Landing.SweepRad * 0.5f;
				const FVector Stand = SolidStronghold::FSolidHandrail::Point(
					Mid, Inner, FloorZ + SolidStronghold::FSolidHandrail::HeightCm);
				float Nearest = TNumericLimits<float>::Max();
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Hole)
				{
					Nearest = FMath::Min(Nearest, FVector::Dist(Piece.Location, Stand));
				}
				for (const SolidStronghold::FSolidHandrail::FPiece& Piece : Flight)
				{
					Nearest = FMath::Min(Nearest, FVector::Dist(Piece.Location, Stand));
				}
				Test.TestTrue(TEXT("the landing stays open onto the floor"), Nearest > 40.f);
			}
		}
	}

	void TestLandings(FAutomationTestBase& Test, const SolidStronghold::FPlan& Plan)
	{
		Test.TestFalse(TEXT("the ground floor has no landing"), SolidStronghold::HasLanding(Plan, 0));
		Test.TestFalse(TEXT("the ground landing wedge stays shut"), SolidStronghold::LandingWedge(Plan, 0).bOpen);
		Test.TestFalse(TEXT("the ground floor has no number"), SolidStronghold::FloorSignFor(Plan, 0).IsPresent());
		Test.TestEqual(TEXT("ground is floor 1"), SolidStronghold::FloorNumber(0), 1);
		for (int32 Floor = 1; Floor < Plan.FloorCount; ++Floor)
		{
			const bool bArrives = SolidStronghold::FlightSweepRad(Plan, Floor - 1) > 0.f;
			if (!bArrives)
			{
				Test.TestFalse(TEXT("a floor with no arriving flight has no landing"), SolidStronghold::HasLanding(Plan, Floor));
				Test.TestFalse(TEXT("a floor with no landing has no number"), SolidStronghold::FloorSignFor(Plan, Floor).IsPresent());
				continue;
			}

			Test.TestTrue(TEXT("an arrival floor has a landing"), SolidStronghold::HasLanding(Plan, Floor));
			const SolidStronghold::FWedge Landing = SolidStronghold::LandingWedge(Plan, Floor);
			Test.TestTrue(TEXT("the landing wedge is open"), Landing.bOpen);
			const float Centerline = SolidStronghold::InteriorRadiusCm(Plan, SolidStronghold::FloorZCm(Plan, Floor))
				- Plan.StairWidthCm * 0.5f;
			Test.TestTrue(
				TEXT("the landing is two metres along the centerline"),
				FMath::IsNearlyEqual(Landing.SweepRad * Centerline, SolidStronghold::LandingRunCm, 0.05f));
			const float FlightEnd = SolidStronghold::FlightStartRad(Plan, Floor - 1)
				+ SolidStronghold::FlightSweepRad(Plan, Floor - 1);
			Test.TestTrue(
				TEXT("the landing starts where the flight below ends"),
				FMath::IsNearlyEqual(Landing.StartRad, FlightEnd, 0.001f));
			const SolidStronghold::FWedge Gap = SolidStronghold::GapWedge(Plan, Floor);
			Test.TestTrue(
				TEXT("the hole is the flight below, not the landing"),
				Gap.bOpen
					&& FMath::IsNearlyEqual(Gap.StartRad, SolidStronghold::FlightStartRad(Plan, Floor - 1), 0.001f)
					&& FMath::IsNearlyEqual(Gap.SweepRad, SolidStronghold::FlightSweepRad(Plan, Floor - 1), 0.001f)
					&& Gap.SweepRad > Landing.SweepRad + 0.05f);
			const SolidStronghold::FSolidFloorSign Sign = SolidStronghold::FloorSignFor(Plan, Floor);
			Test.TestTrue(TEXT("an arrival floor has a number"), Sign.IsPresent());
			Test.TestEqual(
				TEXT("the number is this tower's floor"),
				Sign.Number(),
				SolidStronghold::FloorNumber(Floor));
			if (Floor < Plan.FloorCount - 1)
			{
				Test.TestTrue(
					TEXT("the next flight starts at the far end of the landing"),
					FMath::IsNearlyEqual(
						SolidStronghold::FlightStartRad(Plan, Floor),
						Landing.StartRad + Landing.SweepRad,
						0.001f));
			}
			else
			{
				Test.TestFalse(TEXT("the top floor has no flight up"), SolidStronghold::FlightWedge(Plan, Floor).bOpen);
				Test.TestEqual(
					TEXT("the top sign is this tower's top floor"),
					Sign.Number(),
					SolidStronghold::FloorNumber(Plan.FloorCount - 1));
			}

			const float SignZ = SolidStronghold::FloorZCm(Plan, Floor) + SolidStronghold::FloorSignLiftCm;
			const float WallRadius = SolidStronghold::InteriorRadiusCm(Plan, SignZ)
				- SolidStronghold::FloorSignInsetCm;
			const FVector2D SignXY(Sign.Location().X, Sign.Location().Y);
			Test.TestTrue(
				TEXT("the number stands just inside the wall"),
				FMath::IsNearlyEqual(SignXY.Size(), WallRadius, 0.05f));
			Test.TestTrue(
				TEXT("the number is above the landing"),
				FMath::IsNearlyEqual(
					Sign.Location().Z,
					SolidStronghold::FloorZCm(Plan, Floor) + SolidStronghold::FloorSignLiftCm,
					0.05f));
			const float Mid = Landing.StartRad + Landing.SweepRad * 0.5f;
			Test.TestTrue(
				TEXT("the number is beside the middle of the landing"),
				FMath::IsNearlyEqual(
					SolidStronghold::WrapTau(FMath::Atan2(SignXY.Y, SignXY.X)),
					SolidStronghold::WrapTau(Mid),
					0.001f));
			const FVector Facing = FRotator(0.f, Sign.YawDeg(), 0.f).Vector();
			const FVector Inward = FVector(-SignXY.X, -SignXY.Y, 0.f).GetSafeNormal();
			Test.TestTrue(TEXT("the number faces the room"), Facing.Equals(Inward, 0.01f));
			const FVector Plate = Sign.PlateLocation();
			const FVector PlateSize = SolidStronghold::FSolidFloorSign::PlateSizeCm();
			const float WallAtTop = SolidStronghold::InteriorRadiusCm(
				Plan,
				SignZ + PlateSize.Z * 0.5f);
			Test.TestTrue(
				TEXT("the plate sits toward the wall from the number"),
				Plate.SizeSquared2D() > SignXY.SizeSquared());
			const float HalfPlate = PlateSize.Y * 0.5f;
			const float PlateBack = Plate.Size2D() + PlateSize.X * 0.5f;
			const float PlateEnd = FMath::Sqrt(PlateBack * PlateBack + HalfPlate * HalfPlate);
			Test.TestTrue(TEXT("the plate's top corners stay inside the curved wall"), PlateEnd < WallAtTop - 1.f);
		}
		TestRails(Test, Plan);
	}

	void TestFloorNumberReads(FAutomationTestBase& Test)
	{
		SolidStronghold::FSolidFloorSign TwoSign;
		TwoSign.Place(2, FVector::ZeroVector, 0.f);
		TArray<SolidStronghold::FSolidFloorSign::FStroke> Two;
		TwoSign.CollectStrokes(Two);
		Test.TestEqual(TEXT("a two is five strokes"), Two.Num(), 5);
		bool bTop = false;
		bool bUpperRight = false;
		bool bLowerLeft = false;
		bool bUpperLeft = false;
		for (const SolidStronghold::FSolidFloorSign::FStroke& Stroke : Two)
		{
			const bool bWide = Stroke.SizeCm.Y > Stroke.SizeCm.Z;
			if (bWide && Stroke.CenterCm.Z > 0.f && FMath::IsNearlyZero(Stroke.CenterCm.Y))
			{
				bTop = true;
			}
			if (!bWide && Stroke.CenterCm.Y < 0.f && Stroke.CenterCm.Z > 0.f)
			{
				bUpperRight = true;
			}
			if (!bWide && Stroke.CenterCm.Y > 0.f && Stroke.CenterCm.Z < 0.f)
			{
				bLowerLeft = true;
			}
			if (!bWide && Stroke.CenterCm.Y > 0.f && Stroke.CenterCm.Z > 0.f)
			{
				bUpperLeft = true;
			}
		}
		Test.TestTrue(TEXT("two has a top bar"), bTop);
		Test.TestTrue(TEXT("two has an upper-right bar"), bUpperRight);
		Test.TestTrue(TEXT("two has a lower-left bar"), bLowerLeft);
		Test.TestFalse(TEXT("two has no upper-left bar"), bUpperLeft);

		SolidStronghold::FSolidFloorSign TenSign;
		TenSign.Place(10, FVector::ZeroVector, 0.f);
		SolidStronghold::FSolidFloorSign OneSign;
		OneSign.Place(1, FVector::ZeroVector, 0.f);
		SolidStronghold::FSolidFloorSign ZeroSign;
		ZeroSign.Place(0, FVector::ZeroVector, 0.f);
		TArray<SolidStronghold::FSolidFloorSign::FStroke> Ten;
		TArray<SolidStronghold::FSolidFloorSign::FStroke> One;
		TArray<SolidStronghold::FSolidFloorSign::FStroke> Zero;
		TenSign.CollectStrokes(Ten);
		OneSign.CollectStrokes(One);
		ZeroSign.CollectStrokes(Zero);
		Test.TestEqual(TEXT("ten is a one beside a zero"), Ten.Num(), One.Num() + Zero.Num());
		int32 LeftStrokes = 0;
		int32 RightStrokes = 0;
		bool bLeftIsNarrow = true;
		for (const SolidStronghold::FSolidFloorSign::FStroke& Stroke : Ten)
		{
			if (Stroke.CenterCm.Y > 0.f)
			{
				++LeftStrokes;
				if (Stroke.SizeCm.Y > Stroke.SizeCm.Z)
				{
					bLeftIsNarrow = false;
				}
			}
			else
			{
				++RightStrokes;
			}
		}
		Test.TestEqual(TEXT("the one in ten is on the viewer's left"), LeftStrokes, One.Num());
		Test.TestEqual(TEXT("the zero in ten is on the viewer's right"), RightStrokes, Zero.Num());
		Test.TestTrue(TEXT("that left digit has no wide bar"), bLeftIsNarrow);
	}

	/** Imperious floors 1–3 are large, 4–7 medium, 8–10 small. StoryFloor counts from 1. */
	ESolidStrongholdRoom RoomOnImperiousFloor(const int32 StoryFloor)
	{
		if (StoryFloor <= 3)
		{
			return ESolidStrongholdRoom::Large;
		}
		if (StoryFloor <= 7)
		{
			return ESolidStrongholdRoom::Medium;
		}
		return ESolidStrongholdRoom::Small;
	}

	/**
	 * One floor is one room. Its hexes are that room's size from SolidStronghold.h,
	 * and the floors together are the rooms for this stronghold size.
	 */
	void CheckTowerRooms(
		FAutomationTestBase& Test,
		const ESolidStrongholdSize Size,
		const SolidStronghold::FPlan& Plan)
	{
		const SolidStronghold::FRooms Rooms = SolidStronghold::RoomsFor(Size);
		const FString Name = SolidStronghold::SizeName(Size);
		int32 Large = 0;
		int32 Medium = 0;
		int32 Small = 0;
		int32 Sum = 0;
		Test.TestEqual(
			*FString::Printf(TEXT("%s tower has one floor per room"), *Name),
			Plan.FloorCount,
			Rooms.Large + Rooms.Medium + Rooms.Small);
		SolidStronghold::FSolidTower Tower;
		Tower.Size = Size;
		Tower.BindPlan();
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			const int32 StoryFloor = Plan.SourceFloor + Floor + 1;
			const ESolidStrongholdRoom Room = RoomOnImperiousFloor(StoryFloor);
			TArray<SolidStrongholdGrid::FHex> Tiles;
			Tower.CollectFloorHexes(Floor, Tiles);
			Test.TestEqual(
				*FString::Printf(TEXT("%s floor %d is its room"), *Name, StoryFloor),
				Tiles.Num(),
				SolidStronghold::RoomHexes(Room));
			Sum += Tiles.Num();
			if (Room == ESolidStrongholdRoom::Large)
			{
				++Large;
			}
			else if (Room == ESolidStrongholdRoom::Medium)
			{
				++Medium;
			}
			else
			{
				++Small;
			}
		}
		Test.TestEqual(*FString::Printf(TEXT("%s tower large rooms"), *Name), Large, Rooms.Large);
		Test.TestEqual(*FString::Printf(TEXT("%s tower medium rooms"), *Name), Medium, Rooms.Medium);
		Test.TestEqual(*FString::Printf(TEXT("%s tower small rooms"), *Name), Small, Rooms.Small);
		Test.TestEqual(
			*FString::Printf(TEXT("%s tower hexes match its rooms"), *Name),
			Sum,
			SolidStronghold::Hexes(Rooms));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerStrongholdMeasureTest,
	"SolidCore1.Stronghold.ImperiousTower",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerStrongholdMeasureTest::RunTest(const FString& Parameters)
{
	const SolidStronghold::FPlan& Plan = SolidStronghold::Imperious;
	SolidStronghold::FSolidTower Tower;
	Tower.Size = ESolidStrongholdSize::Imperious;
	Tower.BindPlan();

	TestEqual(TEXT("ten floors"), Plan.FloorCount, 10);
	TestTrue(TEXT("each floor is 3 m"), FMath::IsNearlyEqual(Plan.FloorPitchCm, 300.f, 0.01f));
	TestTrue(TEXT("the shaft is 30 m"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Plan), 3000.f, 0.01f));
	TestTrue(
		TEXT("the top floor's ceiling is the cap"),
		FMath::IsNearlyEqual(
			SolidStronghold::FloorZCm(Plan, Plan.FloorCount - 1) + Plan.FloorPitchCm,
			SolidStronghold::HeightCm(Plan),
			0.01f));
	TestTrue(
		TEXT("the ground floor is at the base"),
		FMath::IsNearlyEqual(SolidStronghold::FloorZCm(Plan, 0), 0.f, 0.01f));
	TestTrue(TEXT("the slab is 20 cm"), FMath::IsNearlyEqual(Plan.SlabCm, 20.f, 0.01f));
	TestTrue(
		TEXT("the clear room is the pitch minus the slab"),
		FMath::IsNearlyEqual(SolidStronghold::ClearHeightCm(Plan), 280.f, 0.01f));

	TestTrue(
		TEXT("ground exterior diameter is 10 m"),
		FMath::IsNearlyEqual(Tower.ExteriorRadiusCm(0.f) * 2.f, 1000.f, 0.01f));
	TestTrue(
		TEXT("ground interior diameter is 9 m"),
		FMath::IsNearlyEqual(SolidStronghold::InteriorRadiusCm(Plan, 0.f) * 2.f, 900.f, 0.01f));
	TestTrue(
		TEXT("top exterior diameter is 6 m"),
		FMath::IsNearlyEqual(Tower.ExteriorRadiusCm(SolidStronghold::HeightCm(Plan)) * 2.f, 600.f, 0.01f));
	TestTrue(
		TEXT("top interior diameter is 5 m"),
		FMath::IsNearlyEqual(SolidStronghold::InteriorRadiusCm(Plan, SolidStronghold::HeightCm(Plan)) * 2.f, 500.f, 0.01f));

	const float SampleZ[] = { 0.f, 750.f, 1500.f, 2400.f, 3000.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays 50 cm"),
			FMath::IsNearlyEqual(
				Tower.ExteriorRadiusCm(ZCm) - SolidStronghold::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	TestTrue(
		TEXT("the door matches the cave width"),
		FMath::IsNearlyEqual(Plan.DoorWidthCm, SolidDungeonLevel::DoorwayWidthCm, 0.01f));
	TestTrue(
		TEXT("the door matches the cave height"),
		FMath::IsNearlyEqual(Plan.DoorHeightCm, SolidDungeonLevel::MouthHeightCm, 0.01f));
	TestTrue(TEXT("the ground floor has the door"), Tower.HasDoor(0));
	TestFalse(TEXT("the floor above has no door"), Tower.HasDoor(1));
	TestFalse(TEXT("the top floor has no door"), Tower.HasDoor(Plan.FloorCount - 1));
	const FVector2D Outward = Tower.DoorOutward();
	TestTrue(
		TEXT("the door faces local -X"),
		FMath::IsNearlyEqual(Outward.X, -1.f, 0.01f) && FMath::IsNearlyZero(Outward.Y, 0.01f));

	TestEqual(TEXT("seventeen risers"), Plan.StairRiserCount, 17);
	TestEqual(TEXT("sixteen treads"), Plan.StairTreadCount, 16);
	TestTrue(TEXT("a tread is 26 cm"), FMath::IsNearlyEqual(Plan.StairTreadCm, 26.f, 0.01f));
	TestTrue(TEXT("the run is sixteen treads"), FMath::IsNearlyEqual(SolidStronghold::StairRunCm(Plan), 416.f, 0.01f));
	TestTrue(
		TEXT("a riser is the pitch split seventeen ways"),
		FMath::IsNearlyEqual(SolidStronghold::RiserCm(Plan), 300.f / 17.f, 0.01f));
	TestTrue(
		TEXT("the stair is as wide as the door"),
		FMath::IsNearlyEqual(Plan.StairWidthCm, Plan.DoorWidthCm, 0.01f));

	const float GroundSweep = 416.f / (450.f - 70.f);
	const float LastSweep = 416.f / (290.f - 70.f);
	TestTrue(
		TEXT("the ground flight is the run along the 3.8 m centerline"),
		FMath::IsNearlyEqual(SolidStronghold::FlightSweepRad(Plan, 0), GroundSweep, 0.001f));
	TestTrue(
		TEXT("the last flight is the run along the 2.2 m centerline"),
		FMath::IsNearlyEqual(SolidStronghold::FlightSweepRad(Plan, 8), LastSweep, 0.001f));
	TestTrue(
		TEXT("the last flight sweeps farther than the ground flight"),
		SolidStronghold::FlightSweepRad(Plan, 8) > SolidStronghold::FlightSweepRad(Plan, 0));

	const SolidStronghold::FWedge GroundFlight = SolidStronghold::FlightWedge(Plan, 0);
	const SolidStronghold::FWedge GroundGap = SolidStronghold::GapWedge(Plan, 0);
	const SolidStronghold::FWedge TopFlight = SolidStronghold::FlightWedge(Plan, 9);
	const SolidStronghold::FWedge TopGap = SolidStronghold::GapWedge(Plan, 9);
	TestTrue(TEXT("the ground floor has a flight"), GroundFlight.bOpen);
	TestFalse(TEXT("the ground floor has no hole"), GroundGap.bOpen);
	TestFalse(TEXT("the top floor has no flight"), TopFlight.bOpen);
	TestTrue(TEXT("the top floor has a hole"), TopGap.bOpen);
	SolidTowerStrongholdTestPrivate::TestLandings(*this, Plan);
	SolidTowerStrongholdTestPrivate::TestFloorNumberReads(*this);
	TestTrue(
		TEXT("the hole is the flight below"),
		FMath::IsNearlyEqual(TopGap.StartRad, SolidStronghold::FlightStartRad(Plan, 8), 0.001f)
			&& FMath::IsNearlyEqual(TopGap.SweepRad, SolidStronghold::FlightSweepRad(Plan, 8), 0.001f));

	float Spiral = 0.f;
	for (int32 Floor = 0; Floor < Plan.FloorCount - 1; ++Floor)
	{
		Spiral += SolidStronghold::FlightSweepRad(Plan, Floor);
	}
	const float Turn = 2.f * PI;
	TestTrue(TEXT("the spiral is just over two turns"), Spiral > 2.f * Turn && Spiral < 2.2f * Turn);

	// The second row is the centered grid, from before this floor's shift.
	const int32 KeptCounts[] = { 35, 28, 22, 19, 16, 12, 12, 8, 6, 7 };
	const int32 CenteredCounts[] = { 34, 26, 19, 19, 12, 12, 12, 7, 4, 5 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	bool bGroundHadLanding = false;
	bool bLandingLeftOut = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		bool bSawLanding = false;
		SolidTowerStrongholdTestPrivate::TestKeptHexes(
			*this, Plan, Floor, KeptCounts[Floor], CenteredCounts[Floor], bSawStair, bSawGap, bSawLanding);
		bGroundStairLeftOut |= Floor == 0 && bSawStair;
		bGroundHadGap |= Floor == 0 && bSawGap;
		bGroundHadLanding |= Floor == 0 && bSawLanding;
		bTopHadStair |= Floor == 9 && bSawStair;
		bTopGapLeftOut |= Floor == 9 && bSawGap;
		bLandingLeftOut |= Floor > 0 && bSawLanding;
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestFalse(TEXT("the ground floor has no landing cell"), bGroundHadLanding);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);
	TestTrue(TEXT("a landing cell is left out"), bLandingLeftOut);

	TArray<SolidStrongholdGrid::FHex> None;
	SolidStronghold::CollectKeptHexes(Plan, -1, None);
	TestEqual(TEXT("a floor below the ground is empty"), None.Num(), 0);
	SolidStronghold::CollectKeptHexes(Plan, Plan.FloorCount, None);
	TestEqual(TEXT("a floor above the cap is empty"), None.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerPlanMeasureTest,
	"SolidCore1.Stronghold.TowerPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerPlanMeasureTest::RunTest(const FString& Parameters)
{
	const SolidStronghold::FPlan& Plan = SolidStronghold::Average;

	TestEqual(TEXT("Average is Imperious floors 7 to 9"), Plan.FloorCount, 3);
	TestEqual(TEXT("Average starts at Imperious floor 7"), Plan.SourceFloor, 6);
	TestTrue(TEXT("the shaft is three storeys"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Plan), 900.f, 0.01f));
	TestTrue(
		TEXT("the ground radius is Imperious at floor 7"),
		FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, 380.f, 0.01f));
	TestTrue(
		TEXT("the cap radius is Imperious at floor 10's walk"),
		FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, 320.f, 0.01f));
	TestTrue(
		TEXT("the ground interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(SolidStronghold::GroundInteriorRadiusCm(Plan), 330.f, 0.01f));
	TestTrue(
		TEXT("the top interior is the exterior minus the wall"),
		FMath::IsNearlyEqual(SolidStronghold::TopInteriorRadiusCm(Plan), 270.f, 0.01f));
	TestTrue(
		TEXT("Average is shorter than Imperious"),
		SolidStronghold::HeightCm(Plan) < SolidStronghold::HeightCm(SolidStronghold::Imperious));
	TestTrue(
		TEXT("Average is narrower than Imperious"),
		Plan.GroundExteriorRadiusCm < SolidStronghold::Imperious.GroundExteriorRadiusCm);
	TestTrue(
		TEXT("Average keeps the Imperious storey"),
		FMath::IsNearlyEqual(Plan.FloorPitchCm, SolidStronghold::Imperious.FloorPitchCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious wall"),
		FMath::IsNearlyEqual(Plan.WallCm, SolidStronghold::Imperious.WallCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious door"),
		FMath::IsNearlyEqual(Plan.DoorWidthCm, SolidStronghold::Imperious.DoorWidthCm, 0.01f)
			&& FMath::IsNearlyEqual(Plan.DoorHeightCm, SolidStronghold::Imperious.DoorHeightCm, 0.01f));
	TestTrue(
		TEXT("Average keeps the Imperious stair"),
		Plan.StairRiserCount == SolidStronghold::Imperious.StairRiserCount
			&& FMath::IsNearlyEqual(Plan.StairWidthCm, SolidStronghold::Imperious.StairWidthCm, 0.01f)
			&& FMath::IsNearlyEqual(SolidStronghold::StairRunCm(Plan), SolidStronghold::StairRunCm(SolidStronghold::Imperious), 0.01f));

	ESolidStrongholdType TokenType = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize TokenSize = ESolidStrongholdSize::Meagre;
	TestTrue(TEXT("Average Tower resolves"), SolidStronghold::FromToken(TEXT("Average Tower"), TokenType, TokenSize));
	SolidStronghold::FSolidTower AverageTower;
	AverageTower.Size = TokenSize;
	AverageTower.BindPlan();
	TestEqual(TEXT("the tower plan is Average"), AverageTower.Plan().FloorCount, Plan.FloorCount);
	TestTrue(
		TEXT("the tower ground matches Average"),
		FMath::IsNearlyEqual(AverageTower.Plan().GroundExteriorRadiusCm, Plan.GroundExteriorRadiusCm, 0.01f));
	TestTrue(
		TEXT("the tower cap matches Average"),
		FMath::IsNearlyEqual(AverageTower.Plan().TopExteriorRadiusCm, Plan.TopExteriorRadiusCm, 0.01f));

	TestTrue(TEXT("Imperious Tower resolves"), SolidStronghold::FromToken(TEXT("Imperious Tower"), TokenType, TokenSize));
	SolidStronghold::FSolidTower ImperiousTower;
	ImperiousTower.Size = TokenSize;
	ImperiousTower.BindPlan();
	TestEqual(TEXT("the tower plan is Imperious"), ImperiousTower.Plan().FloorCount, SolidStronghold::Imperious.FloorCount);

	TestTrue(TEXT("Humble Tower resolves"), SolidStronghold::FromToken(TEXT("Humble Tower"), TokenType, TokenSize));
	SolidStronghold::FSolidTower HumbleTower;
	HumbleTower.Size = TokenSize;
	TestFalse(TEXT("Humble tower is not supported"), HumbleTower.Supported(TokenSize));

	const float SampleZ[] = { 0.f, 450.f, 900.f };
	for (const float ZCm : SampleZ)
	{
		TestTrue(
			TEXT("the wall stays the plan thickness"),
			FMath::IsNearlyEqual(
				AverageTower.ExteriorRadiusCm(ZCm) - SolidStronghold::InteriorRadiusCm(Plan, ZCm),
				Plan.WallCm,
				0.01f));
	}

	const float GroundCenterline = SolidStronghold::GroundInteriorRadiusCm(Plan) - Plan.StairWidthCm * 0.5f;
	const float MidInterior = SolidStronghold::InteriorRadiusCm(Plan, SolidStronghold::FloorZCm(Plan, 1));
	const float MidCenterline = MidInterior - Plan.StairWidthCm * 0.5f;
	TestTrue(
		TEXT("the ground flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidStronghold::FlightSweepRad(Plan, 0),
			SolidStronghold::StairRunCm(Plan) / GroundCenterline,
			0.001f));
	TestTrue(
		TEXT("the last flight uses this plan's centerline"),
		FMath::IsNearlyEqual(
			SolidStronghold::FlightSweepRad(Plan, 1),
			SolidStronghold::StairRunCm(Plan) / MidCenterline,
			0.001f));
	TestTrue(
		TEXT("the ground flight has the Imperious floor 7 sweep"),
		FMath::IsNearlyEqual(
			SolidStronghold::FlightSweepRad(Plan, 0),
			SolidStronghold::FlightSweepRad(SolidStronghold::Imperious, Plan.SourceFloor),
			0.001f));
	TestTrue(
		TEXT("the ground flight starts just past the door"),
		FMath::IsNearlyEqual(
			SolidStronghold::FlightStartRad(Plan, 0),
			Plan.DoorAngleRad + Plan.StairPastDoorRad,
			0.001f));
	const float DoorHalf = FMath::Asin(
		FMath::Clamp((Plan.DoorWidthCm * 0.5f) / FMath::Max(SolidStronghold::GroundInteriorRadiusCm(Plan), 1.f), 0.f, 1.f));
	const SolidStronghold::FWedge GroundFlight = SolidStronghold::FlightWedge(Plan, 0);
	TestFalse(
		TEXT("the ground flight leaves the door clear"),
		SolidStronghold::AngleInSweep(Plan.DoorAngleRad - DoorHalf, GroundFlight.StartRad, GroundFlight.SweepRad)
			|| SolidStronghold::AngleInSweep(Plan.DoorAngleRad, GroundFlight.StartRad, GroundFlight.SweepRad)
			|| SolidStronghold::AngleInSweep(Plan.DoorAngleRad + DoorHalf, GroundFlight.StartRad, GroundFlight.SweepRad));
	TestFalse(
		TEXT("the ground sweep is not the Imperious ground sweep"),
		FMath::IsNearlyEqual(
			SolidStronghold::FlightSweepRad(Plan, 0),
			SolidStronghold::FlightSweepRad(SolidStronghold::Imperious, 0),
			0.001f));
	SolidTowerStrongholdTestPrivate::TestLandings(*this, Plan);
	TestTrue(TEXT("the ground floor has a flight"), SolidStronghold::FlightWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the ground floor has no hole"), SolidStronghold::GapWedge(Plan, 0).bOpen);
	TestFalse(TEXT("the top floor has no flight"), SolidStronghold::FlightWedge(Plan, Plan.FloorCount - 1).bOpen);
	TestTrue(TEXT("the top floor has a hole"), SolidStronghold::GapWedge(Plan, Plan.FloorCount - 1).bOpen);

	// Cells that fit, before the floor is cut down to 12, 6, and 6 tiles. The grid stays centered.
	const int32 KeptCounts[] = { 16, 7, 8 };
	const int32 CenteredCounts[] = { 16, 7, 8 };
	bool bGroundStairLeftOut = false;
	bool bTopGapLeftOut = false;
	bool bGroundHadGap = false;
	bool bTopHadStair = false;
	bool bGroundHadLanding = false;
	bool bLandingLeftOut = false;
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		bool bSawStair = false;
		bool bSawGap = false;
		bool bSawLanding = false;
		SolidTowerStrongholdTestPrivate::TestKeptHexes(
			*this, Plan, Floor, KeptCounts[Floor], CenteredCounts[Floor], bSawStair, bSawGap, bSawLanding);
		bGroundStairLeftOut |= Floor == 0 && bSawStair;
		bGroundHadGap |= Floor == 0 && bSawGap;
		bGroundHadLanding |= Floor == 0 && bSawLanding;
		bTopHadStair |= Floor == Plan.FloorCount - 1 && bSawStair;
		bTopGapLeftOut |= Floor == Plan.FloorCount - 1 && bSawGap;
		bLandingLeftOut |= Floor > 0 && bSawLanding;
	}
	TestTrue(TEXT("the ground stair is left out"), bGroundStairLeftOut);
	TestFalse(TEXT("the ground floor has no hole cell"), bGroundHadGap);
	TestFalse(TEXT("the ground floor has no landing cell"), bGroundHadLanding);
	TestTrue(TEXT("the top hole is left out"), bTopGapLeftOut);
	TestFalse(TEXT("the top floor has no stair cell"), bTopHadStair);
	TestTrue(TEXT("a landing cell is left out"), bLandingLeftOut);

	SolidStronghold::FPlan Single = SolidStronghold::Imperious;
	Single.FloorCount = 1;
	Single.GroundExteriorRadiusCm = 350.f;
	Single.TopExteriorRadiusCm = 350.f;
	TestTrue(TEXT("a single floor is one storey"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Single), 300.f, 0.01f));
	TestTrue(TEXT("the single floor has the door"), AverageTower.HasDoor(0));
	SolidTowerStrongholdTestPrivate::TestLandings(*this, Single);
	TestFalse(TEXT("a single floor has no flight"), SolidStronghold::FlightWedge(Single, 0).bOpen);
	TestFalse(TEXT("a single floor has no hole"), SolidStronghold::GapWedge(Single, 0).bOpen);
	bool bIgnoredStair = false;
	bool bIgnoredGap = false;
	bool bIgnoredLanding = false;
	SolidTowerStrongholdTestPrivate::TestKeptHexes(*this, Single, 0, 13, 13, bIgnoredStair, bIgnoredGap, bIgnoredLanding);
	TestFalse(TEXT("a single floor has no stair cell"), bIgnoredStair);
	TestFalse(TEXT("a single floor has no hole cell"), bIgnoredGap);
	TestFalse(TEXT("a single floor has no landing cell"), bIgnoredLanding);

	SolidStronghold::FPlan Narrow = SolidStronghold::Imperious;
	Narrow.FloorCount = 2;
	Narrow.GroundExteriorRadiusCm = 100.f;
	Narrow.TopExteriorRadiusCm = 100.f;
	TestTrue(
		TEXT("a stair wider than the room has no sweep"),
		FMath::IsNearlyZero(SolidStronghold::FlightSweepRad(Narrow, 0)));
	SolidTowerStrongholdTestPrivate::TestLandings(*this, Narrow);
	TestFalse(TEXT("that flight stays closed"), SolidStronghold::FlightWedge(Narrow, 0).bOpen);
	TArray<SolidStrongholdGrid::FHex> Empty;
	SolidStronghold::CollectKeptHexes(Narrow, 0, Empty);
	TestEqual(TEXT("a room narrower than a hex keeps nothing"), Empty.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerStepMeasureTest,
	"SolidCore1.Stronghold.TowerSteps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerStepMeasureTest::RunTest(const FString& Parameters)
{
	using SolidStronghold::Average;
	using SolidStronghold::FPlan;
	using SolidStronghold::FlightStartRad;
	using SolidStronghold::Grand;
	using SolidStronghold::Imperious;
	using SolidStronghold::Impressive;
	using SolidStronghold::LegacyLowPlan;
	using SolidStronghold::LerpPlan;
	using SolidStronghold::SizeAlong;
	using SolidStronghold::Stately;

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

	SolidStronghold::FSolidTowerBand NoTower;
	TestFalse(
		TEXT("Meagre has no tower floors"),
		SolidStronghold::FindTowerBand(ESolidStrongholdSize::Meagre, NoTower));
	TestFalse(
		TEXT("Humble has no tower floors"),
		SolidStronghold::FindTowerBand(ESolidStrongholdSize::Humble, NoTower));

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
	const int32 KeptImpressive[] = { 16, 12, 12, 7, 8 };
	const int32 CenteredImpressive[] = { 16, 12, 11, 7, 8 };
	const int32 KeptGrand[] = { 26, 19, 13, 12, 12, 7, 8 };
	const int32 CenteredGrand[] = { 26, 19, 13, 12, 11, 7, 8 };
	const int32 KeptStately[] = { 33, 20, 18, 13, 12, 12, 7, 8 };
	const int32 CenteredStately[] = { 33, 20, 18, 13, 12, 12, 7, 8 };
	const int32* KeptByStep[] = { nullptr, KeptImpressive, KeptGrand, KeptStately, nullptr };
	const int32* CenteredByStep[] = { nullptr, CenteredImpressive, CenteredGrand, CenteredStately, nullptr };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Steps); ++Index)
	{
		const FPlan& Plan = Steps[Index];
		SolidStronghold::FSolidTowerBand Band;
		const FString SizeName = SolidStronghold::SizeName(Sizes[Index]);
		TestTrue(
			*FString::Printf(TEXT("%s has a floor band"), *SizeName),
			SolidStronghold::FindTowerBand(Sizes[Index], Band));
		TestEqual(
			*FString::Printf(TEXT("%s plan starts on its band"), *SizeName),
			Plan.SourceFloor,
			SolidStronghold::BandSourceFloor(Band));
		TestEqual(
			*FString::Printf(TEXT("%s plan keeps its band"), *SizeName),
			Plan.FloorCount,
			SolidStronghold::BandFloorCount(Band));
		if (Sizes[Index] == ESolidStrongholdSize::Average)
		{
			TestEqual(TEXT("Average is three floors"), Plan.FloorCount, 3);
			TestEqual(TEXT("Average starts on Imperious floor 7"), Plan.SourceFloor, 6);
			TestTrue(
				TEXT("Average ground is Imperious at floor 7"),
				FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, 380.f, 0.01f));
			TestTrue(
				TEXT("Average cap is Imperious above floor 9"),
				FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, 320.f, 0.01f));
		}
		else if (Sizes[Index] == ESolidStrongholdSize::Impressive)
		{
			TestEqual(TEXT("Impressive is five floors"), Plan.FloorCount, 5);
			TestEqual(TEXT("Impressive starts on Imperious floor 5"), Plan.SourceFloor, 4);
			TestTrue(TEXT("Impressive is 15 m"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Plan), 1500.f, 0.01f));
			TestTrue(
				TEXT("Impressive ground is Imperious at floor 5"),
				FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, 420.f, 0.01f));
			TestTrue(
				TEXT("Impressive cap is Imperious above floor 9"),
				FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, 320.f, 0.01f));
			TestTrue(
				TEXT("the Impressive ground flight starts just past the door"),
				FMath::IsNearlyEqual(FlightStartRad(Plan, 0), Plan.DoorAngleRad + Plan.StairPastDoorRad, 0.001f));
		}
		else if (Sizes[Index] == ESolidStrongholdSize::Grand)
		{
			TestEqual(TEXT("Grand is seven floors"), Plan.FloorCount, 7);
			TestEqual(TEXT("Grand starts on Imperious floor 3"), Plan.SourceFloor, 2);
			TestTrue(TEXT("Grand is 21 m"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Plan), 2100.f, 0.01f));
			TestTrue(
				TEXT("Grand ground is Imperious at floor 3"),
				FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, 460.f, 0.01f));
			TestTrue(
				TEXT("Grand cap is Imperious above floor 9"),
				FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, 320.f, 0.01f));
			TestTrue(
				TEXT("the Grand ground flight starts just past the door"),
				FMath::IsNearlyEqual(FlightStartRad(Plan, 0), Plan.DoorAngleRad + Plan.StairPastDoorRad, 0.001f));
		}
		else if (Sizes[Index] == ESolidStrongholdSize::Stately)
		{
			TestEqual(TEXT("Stately is eight floors"), Plan.FloorCount, 8);
			TestEqual(TEXT("Stately starts on Imperious floor 2"), Plan.SourceFloor, 1);
			TestTrue(TEXT("Stately is 24 m"), FMath::IsNearlyEqual(SolidStronghold::HeightCm(Plan), 2400.f, 0.01f));
			TestTrue(
				TEXT("Stately ground is Imperious at floor 2"),
				FMath::IsNearlyEqual(Plan.GroundExteriorRadiusCm, 480.f, 0.01f));
			TestTrue(
				TEXT("Stately cap is Imperious above floor 9"),
				FMath::IsNearlyEqual(Plan.TopExteriorRadiusCm, 320.f, 0.01f));
			TestTrue(
				TEXT("the Stately ground flight starts just past the door"),
				FMath::IsNearlyEqual(FlightStartRad(Plan, 0), Plan.DoorAngleRad + Plan.StairPastDoorRad, 0.001f));
		}
		else
		{
			const FPlan Blended = LerpPlan(LegacyLowPlan(), Imperious, SizeAlong(Sizes[Index]));
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
		}
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

		ESolidStrongholdType TokenType = ESolidStrongholdType::Dungeon;
		ESolidStrongholdSize TokenSize = ESolidStrongholdSize::Meagre;
		TestTrue(TEXT("the size token resolves"), SolidStronghold::FromToken(Tokens[Index], TokenType, TokenSize));
		SolidStronghold::FSolidTower Built;
		Built.Size = Sizes[Index];
		Built.BindPlan();
		SolidTowerStrongholdTestPrivate::TestLandings(*this, Plan);
		TestEqual(TEXT("the tower plan is this step"), Built.Plan().FloorCount, Plan.FloorCount);
		TestTrue(
			TEXT("the tower plan has this step's base"),
			FMath::IsNearlyEqual(Built.Plan().GroundExteriorRadiusCm, Plan.GroundExteriorRadiusCm, 0.01f));

		if (KeptByStep[Index] == nullptr)
		{
			continue;
		}
		bool bGroundStair = false;
		bool bTopGap = false;
		bool bGroundGap = false;
		bool bTopStair = false;
		bool bGroundLanding = false;
		bool bLandingLeftOut = false;
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			bool bSawStair = false;
			bool bSawGap = false;
			bool bSawLanding = false;
			SolidTowerStrongholdTestPrivate::TestKeptHexes(
				*this,
				Plan,
				Floor,
				KeptByStep[Index][Floor],
				CenteredByStep[Index][Floor],
				bSawStair,
				bSawGap,
				bSawLanding);
			bGroundStair |= Floor == 0 && bSawStair;
			bGroundGap |= Floor == 0 && bSawGap;
			bGroundLanding |= Floor == 0 && bSawLanding;
			bTopStair |= Floor == Plan.FloorCount - 1 && bSawStair;
			bTopGap |= Floor == Plan.FloorCount - 1 && bSawGap;
			bLandingLeftOut |= Floor > 0 && bSawLanding;
		}
		TestTrue(TEXT("the ground stair is left out"), bGroundStair);
		TestFalse(TEXT("the ground floor has no hole cell"), bGroundGap);
		TestFalse(TEXT("the ground floor has no landing cell"), bGroundLanding);
		TestTrue(TEXT("the top hole is left out"), bTopGap);
		TestFalse(TEXT("the top floor has no stair cell"), bTopStair);
		TestTrue(TEXT("a landing cell is left out"), bLandingLeftOut);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerFloorHexTest,
	"SolidCore1.Stronghold.TowerFloorHexes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerFloorHexTest::RunTest(const FString& Parameters)
{
	const SolidStronghold::FPlan& Plan = SolidStronghold::Imperious;
	SolidStronghold::FSolidTower ImperiousTower;
	ImperiousTower.Size = ESolidStrongholdSize::Imperious;
	ImperiousTower.BindPlan();
	TestEqual(TEXT("Imperious has ten floors"), Plan.FloorCount, 10);

	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		TArray<SolidStrongholdGrid::FHex> Tiles;
		TArray<SolidStrongholdGrid::FHex> Kept;
		ImperiousTower.CollectFloorHexes(Floor, Tiles);
		SolidStronghold::CollectKeptHexes(Plan, Floor, Kept);
		TestTrue(
			*FString::Printf(TEXT("floor %d tiles fit in the circle"), Floor + 1),
			Tiles.Num() > 0 && Tiles.Num() <= Kept.Num());
		const float Radius = SolidStronghold::InteriorRadiusCm(Plan, SolidStronghold::FloorZCm(Plan, Floor));
		TSet<uint64> KeptKeys;
		for (const SolidStrongholdGrid::FHex& Hex : Kept)
		{
			KeptKeys.Add((static_cast<uint64>(static_cast<uint32>(Hex.Col)) << 32) | static_cast<uint32>(Hex.Row));
		}
		TArray<bool> Keep;
		Keep.Init(true, Tiles.Num());
		for (const SolidStrongholdGrid::FHex& Hex : Tiles)
		{
			const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Hex.Col)) << 32) | static_cast<uint32>(Hex.Row);
			TestTrue(
				*FString::Printf(TEXT("floor %d tile sits in the circle"), Floor + 1),
				KeptKeys.Contains(Key));
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
			for (const FVector2D& Vertex : Vertices)
			{
				TestTrue(
					*FString::Printf(TEXT("floor %d tile stays inside the floor"), Floor + 1),
					Vertex.Size() <= Radius + 0.01f);
			}
			TestFalse(
				*FString::Printf(TEXT("floor %d tile is not on the stair"), Floor + 1),
				SolidStronghold::HexOnStair(Plan, Floor, Hex.Col, Hex.Row));
			TestFalse(
				*FString::Printf(TEXT("floor %d tile is not in the hole"), Floor + 1),
				SolidStronghold::HexOnGap(Plan, Floor, Hex.Col, Hex.Row));
			TestFalse(
				*FString::Printf(TEXT("floor %d tile is not on the landing"), Floor + 1),
				SolidStronghold::HexOnLanding(Plan, Floor, Hex.Col, Hex.Row));
		}
		TestTrue(
			*FString::Printf(TEXT("floor %d tiles stay connected"), Floor + 1),
			ImperiousTower.FloorHexesConnect(Tiles, Keep, Tiles.Num()));
	}

	SolidTowerStrongholdTestPrivate::CheckTowerRooms(*this, ESolidStrongholdSize::Imperious, Plan);

	const FVector2D Door = ImperiousTower.DoorHexCenter();
	TArray<SolidStrongholdGrid::FHex> Ground;
	ImperiousTower.CollectFloorHexes(0, Ground);
	bool bDoorTile = false;
	for (const SolidStrongholdGrid::FHex& Hex : Ground)
	{
		bDoorTile |= Hex.Center.Equals(Door, 0.1f);
	}
	TestFalse(TEXT("the ground floor leaves the door cell empty"), bDoorTile);

	const SolidStronghold::FPlan& Average = SolidStronghold::Average;
	SolidTowerStrongholdTestPrivate::CheckTowerRooms(*this, ESolidStrongholdSize::Average, Average);

	const SolidStronghold::FPlan& Impressive = SolidStronghold::Impressive;
	SolidTowerStrongholdTestPrivate::CheckTowerRooms(*this, ESolidStrongholdSize::Impressive, Impressive);
	SolidStronghold::FSolidTower ImpressiveTower;
	ImpressiveTower.Size = ESolidStrongholdSize::Impressive;
	ImpressiveTower.BindPlan();
	const FVector2D ImpressiveDoor = ImpressiveTower.DoorHexCenter();
	TArray<SolidStrongholdGrid::FHex> ImpressiveGround;
	ImpressiveTower.CollectFloorHexes(0, ImpressiveGround);
	bool bImpressiveDoorTile = false;
	for (const SolidStrongholdGrid::FHex& Hex : ImpressiveGround)
	{
		bImpressiveDoorTile |= Hex.Center.Equals(ImpressiveDoor, 0.1f);
	}
	TestFalse(TEXT("the Impressive ground floor leaves the door cell empty"), bImpressiveDoorTile);

	const SolidStronghold::FPlan& Grand = SolidStronghold::Grand;
	SolidTowerStrongholdTestPrivate::CheckTowerRooms(*this, ESolidStrongholdSize::Grand, Grand);
	SolidStronghold::FSolidTower GrandTower;
	GrandTower.Size = ESolidStrongholdSize::Grand;
	GrandTower.BindPlan();
	const FVector2D GrandDoor = GrandTower.DoorHexCenter();
	TArray<SolidStrongholdGrid::FHex> GrandGround;
	GrandTower.CollectFloorHexes(0, GrandGround);
	bool bGrandDoorTile = false;
	for (const SolidStrongholdGrid::FHex& Hex : GrandGround)
	{
		bGrandDoorTile |= Hex.Center.Equals(GrandDoor, 0.1f);
	}
	TestFalse(TEXT("the Grand ground floor leaves the door cell empty"), bGrandDoorTile);

	const SolidStronghold::FPlan& Stately = SolidStronghold::Stately;
	SolidTowerStrongholdTestPrivate::CheckTowerRooms(*this, ESolidStrongholdSize::Stately, Stately);
	SolidStronghold::FSolidTower StatelyTower;
	StatelyTower.Size = ESolidStrongholdSize::Stately;
	StatelyTower.BindPlan();
	const FVector2D StatelyDoor = StatelyTower.DoorHexCenter();
	TArray<SolidStrongholdGrid::FHex> StatelyGround;
	StatelyTower.CollectFloorHexes(0, StatelyGround);
	bool bStatelyDoorTile = false;
	for (const SolidStrongholdGrid::FHex& Hex : StatelyGround)
	{
		bStatelyDoorTile |= Hex.Center.Equals(StatelyDoor, 0.1f);
	}
	TestFalse(TEXT("the Stately ground floor leaves the door cell empty"), bStatelyDoorTile);
	return true;
}

#endif
