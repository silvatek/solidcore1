#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"
#include "SolidStrongholdGrid.h"

/**
 * A round tower, measured in the tower's own plan.
 * FPlan is one size. Average and Imperious are the ends.
 * Impressive, Grand, and Stately are even steps between those ends.
 * Local +X runs in from the door. Yaw 0 looks into the tower, the same way a dungeon visit does.
 * Placement turns the whole tower so this door faces the grass. On cell 17,31 that is north.
 * The Meagre den stays the fourteen-cell outline. This floor uses the same 1.2 m pointy-top grid,
 * measured from the tower center.
 */
namespace SolidTowerStronghold
{
	/**
	 * One tower size. Height, clear height, interior radii, and the stair run are derived,
	 * so the wall stays one thickness and the top floor's ceiling is the cap.
	 */
	struct FPlan
	{
		int32 FloorCount = 10;
		/** Walking surface to the walking surface above. */
		float FloorPitchCm = 300.f;
		float SlabCm = 20.f;
		/** Exterior radius minus interior radius, at every height. */
		float WallCm = 50.f;

		float GroundExteriorRadiusCm = 500.f;
		float TopExteriorRadiusCm = 300.f;

		/** Same clear opening as the cave door. */
		float DoorWidthCm = 140.f;
		float DoorHeightCm = 240.f;

		/** The flight occupies this band inside the wall. */
		float StairWidthCm = 140.f;
		int32 StairRiserCount = 17;
		int32 StairTreadCount = 16;
		float StairTreadCm = 26.f;

		/** Out through the door, local -X. */
		float DoorAngleRad = PI;
		/** The first flight starts this far past the door center, then the spiral continues. */
		float StairPastDoorRad = 20.f * PI / 180.f;

		/** Outside of the stone, ground to the flat cap. */
		float HeightCm() const
		{
			return static_cast<float>(FloorCount) * FloorPitchCm;
		}

		/** Clear room under the slab. */
		float ClearHeightCm() const
		{
			return FloorPitchCm - SlabCm;
		}

		float GroundInteriorRadiusCm() const
		{
			return GroundExteriorRadiusCm - WallCm;
		}

		float TopInteriorRadiusCm() const
		{
			return TopExteriorRadiusCm - WallCm;
		}

		float StairRunCm() const
		{
			return static_cast<float>(StairTreadCount) * StairTreadCm;
		}

		float RiserCm() const
		{
			if (StairRiserCount <= 0)
			{
				return 0.f;
			}
			return FloorPitchCm / static_cast<float>(StairRiserCount);
		}
	};

	/** Average through Imperious, in size-list order. Meagre and Humble are outside this span. */
	inline bool IsDefinedSize(const ESolidStrongholdSize Size)
	{
		return Size >= ESolidStrongholdSize::Average && Size <= ESolidStrongholdSize::Imperious;
	}

	/** 0 at Average, 1 at Imperious. The three sizes between them land on 1/4, 1/2, and 3/4. */
	inline float SizeAlong(const ESolidStrongholdSize Size)
	{
		const float From = static_cast<float>(ESolidStrongholdSize::Average);
		const float To = static_cast<float>(ESolidStrongholdSize::Imperious);
		return (static_cast<float>(Size) - From) / (To - From);
	}

	/** A halfway count rounds up. The steps from 4 floors to 10 are 6, 7, and 9. */
	inline int32 LerpCount(const int32 From, const int32 To, const float Along)
	{
		const float Blended = FMath::Lerp(static_cast<float>(From), static_cast<float>(To), Along);
		return FMath::FloorToInt(Blended + 0.5f);
	}

	/** Whole-number fields round. The other fields blend in a straight line. */
	inline FPlan LerpPlan(const FPlan& From, const FPlan& To, const float Along)
	{
		FPlan Plan;
		Plan.FloorCount = LerpCount(From.FloorCount, To.FloorCount, Along);
		Plan.FloorPitchCm = FMath::Lerp(From.FloorPitchCm, To.FloorPitchCm, Along);
		Plan.SlabCm = FMath::Lerp(From.SlabCm, To.SlabCm, Along);
		Plan.WallCm = FMath::Lerp(From.WallCm, To.WallCm, Along);
		Plan.GroundExteriorRadiusCm = FMath::Lerp(From.GroundExteriorRadiusCm, To.GroundExteriorRadiusCm, Along);
		Plan.TopExteriorRadiusCm = FMath::Lerp(From.TopExteriorRadiusCm, To.TopExteriorRadiusCm, Along);
		Plan.DoorWidthCm = FMath::Lerp(From.DoorWidthCm, To.DoorWidthCm, Along);
		Plan.DoorHeightCm = FMath::Lerp(From.DoorHeightCm, To.DoorHeightCm, Along);
		Plan.StairWidthCm = FMath::Lerp(From.StairWidthCm, To.StairWidthCm, Along);
		Plan.StairRiserCount = LerpCount(From.StairRiserCount, To.StairRiserCount, Along);
		Plan.StairTreadCount = LerpCount(From.StairTreadCount, To.StairTreadCount, Along);
		Plan.StairTreadCm = FMath::Lerp(From.StairTreadCm, To.StairTreadCm, Along);
		Plan.DoorAngleRad = FMath::Lerp(From.DoorAngleRad, To.DoorAngleRad, Along);
		Plan.StairPastDoorRad = FMath::Lerp(From.StairPastDoorRad, To.StairPastDoorRad, Along);
		return Plan;
	}

	inline FPlan MakePlan(const ESolidStrongholdSize Size)
	{
		FPlan Small;
		Small.FloorCount = 4;
		Small.GroundExteriorRadiusCm = 400.f;
		Small.TopExteriorRadiusCm = 280.f;
		const FPlan Large;
		return LerpPlan(Small, Large, SizeAlong(Size));
	}

	/** Four floors, 8 m across at the grass, 5.6 m across at the cap. */
	inline const FPlan Average = MakePlan(ESolidStrongholdSize::Average);
	/** One quarter of the way from Average to Imperious. Six floors, 8.5 m across at the grass. */
	inline const FPlan Impressive = MakePlan(ESolidStrongholdSize::Impressive);
	/** Halfway from Average to Imperious. Seven floors, 9 m across at the grass. */
	inline const FPlan Grand = MakePlan(ESolidStrongholdSize::Grand);
	/** Three quarters of the way from Average to Imperious. Nine floors, 9.5 m across at the grass. */
	inline const FPlan Stately = MakePlan(ESolidStrongholdSize::Stately);
	/** The shipped tower. Ten floors, 10 m across at the grass, 6 m across at the cap. */
	inline const FPlan Imperious = MakePlan(ESolidStrongholdSize::Imperious);

	/** The plan for a tower size that has one. A dungeon, or a tower below Average, leaves OutPlan alone. */
	inline bool PlanFor(const FSolidStronghold& Stronghold, FPlan& OutPlan)
	{
		if (Stronghold.Type != ESolidStrongholdType::Tower || !IsDefinedSize(Stronghold.Size))
		{
			return false;
		}
		OutPlan = MakePlan(Stronghold.Size);
		return true;
	}

	inline bool IsFloor(const FPlan& Plan, const int32 Floor)
	{
		return Floor >= 0 && Floor < Plan.FloorCount;
	}

	/** Walking surface of a floor. Floor 0 is the ground. The last floor is the top room. */
	inline float FloorZCm(const FPlan& Plan, const int32 Floor)
	{
		return static_cast<float>(Floor) * Plan.FloorPitchCm;
	}

	inline float ExteriorRadiusCm(const FPlan& Plan, const float ZCm)
	{
		const float Height = FMath::Max(Plan.HeightCm(), 1.f);
		const float Along = FMath::Clamp(ZCm / Height, 0.f, 1.f);
		return FMath::Lerp(Plan.GroundExteriorRadiusCm, Plan.TopExteriorRadiusCm, Along);
	}

	inline float InteriorRadiusCm(const FPlan& Plan, const float ZCm)
	{
		const float Height = FMath::Max(Plan.HeightCm(), 1.f);
		const float Along = FMath::Clamp(ZCm / Height, 0.f, 1.f);
		return FMath::Lerp(Plan.GroundInteriorRadiusCm(), Plan.TopInteriorRadiusCm(), Along);
	}

	/** The ground floor only. */
	inline bool HasDoor(const int32 Floor)
	{
		return Floor == 0;
	}

	/** Direction from the center out through the door. */
	inline FVector2D DoorOutward(const FPlan& Plan)
	{
		return FVector2D(FMath::Cos(Plan.DoorAngleRad), FMath::Sin(Plan.DoorAngleRad));
	}

	/** Depth of the dark door slab, along local X. */
	inline constexpr float DoorSlabDepthCm = 40.f;
	/** How far the outer face stands past the ground course. */
	inline constexpr float DoorProudCm = 8.f;

	/**
	 * Grass-facing yaw points local +X at the grass, the cave-mouth convention.
	 * The tower door is local -X, so the shell turns half a turn from that yaw.
	 * North grass (facing yaw 90) puts the door at shell yaw -90.
	 */
	inline float ShellYawDeg(const float GrassFacingYawDeg)
	{
		return FMath::UnwindDegrees(GrassFacingYawDeg + 180.f);
	}

	/** Center of the door slab. Local -X, bottom on the ground, proud of the ground course. */
	inline FVector DoorSlabCenterCm(const FPlan& Plan)
	{
		const float Radius = Plan.GroundExteriorRadiusCm;
		const float X = -(Radius - DoorSlabDepthCm * 0.5f + DoorProudCm);
		return FVector(X, 0.f, Plan.DoorHeightCm * 0.5f);
	}

	struct FWedge
	{
		float StartRad = 0.f;
		float SweepRad = 0.f;
		/** False when this floor has no such opening. */
		bool bOpen = false;
	};

	/** Arc of the flight that leaves this floor. The top floor has none. */
	inline float FlightSweepRad(const FPlan& Plan, const int32 Floor)
	{
		if (Floor < 0 || Floor >= Plan.FloorCount - 1)
		{
			return 0.f;
		}
		const float Centerline = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)) - Plan.StairWidthCm * 0.5f;
		if (Centerline <= 0.f)
		{
			return 0.f;
		}
		return Plan.StairRunCm() / Centerline;
	}

	/**
	 * Flat run along the stair centerline where a flight arrives, before the next flight.
	 * About two metres. The same length on every tower size.
	 */
	inline constexpr float LandingRunCm = 200.f;

	/** Stairs arrive on this floor, so it has a landing. The ground flight has none of its own. */
	inline bool HasLanding(const FPlan& Plan, const int32 Floor)
	{
		return IsFloor(Plan, Floor) && Floor > 0 && FlightSweepRad(Plan, Floor - 1) > 0.f;
	}

	/** Arc of the landing at this floor's walking surface. Zero when this floor has no landing. */
	inline float LandingSweepRad(const FPlan& Plan, const int32 Floor)
	{
		if (!HasLanding(Plan, Floor))
		{
			return 0.f;
		}
		const float Centerline = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)) - Plan.StairWidthCm * 0.5f;
		if (Centerline <= 0.f)
		{
			return 0.f;
		}
		return LandingRunCm / Centerline;
	}

	/**
	 * Where this floor's flight starts.
	 * Each landing sits between the flight below and the next flight, so a flight above the ground
	 * starts at the far end of that landing.
	 */
	inline float FlightStartRad(const FPlan& Plan, const int32 Floor)
	{
		float Start = Plan.DoorAngleRad + Plan.StairPastDoorRad;
		const int32 Last = FMath::Clamp(Floor, 0, FMath::Max(Plan.FloorCount - 1, 0));
		for (int32 Index = 0; Index < Last; ++Index)
		{
			Start += FlightSweepRad(Plan, Index);
			Start += LandingSweepRad(Plan, Index + 1);
		}
		return Start;
	}

	inline FWedge FlightWedge(const FPlan& Plan, const int32 Floor)
	{
		FWedge Wedge;
		if (Floor < 0 || Floor >= Plan.FloorCount - 1)
		{
			return Wedge;
		}
		Wedge.bOpen = true;
		Wedge.StartRad = FlightStartRad(Plan, Floor);
		Wedge.SweepRad = FlightSweepRad(Plan, Floor);
		if (Wedge.SweepRad <= 0.f)
		{
			Wedge.bOpen = false;
		}
		return Wedge;
	}

	/** The hole for the flight that arrives from the floor below. The ground floor has none. The landing is not part of the hole. */
	inline FWedge GapWedge(const FPlan& Plan, const int32 Floor)
	{
		if (Floor <= 0)
		{
			return FWedge{};
		}
		return FlightWedge(Plan, Floor - 1);
	}

	/** Ground is floor 1. The plan's floor index starts at 0. */
	inline int32 FloorNumber(const int32 Floor)
	{
		return Floor + 1;
	}

	/** How far above the walking surface the floor number sits. */
	inline constexpr float FloorSignLiftCm = 150.f;
	/** How far inside the curved wall the number's plane stands, so the plate stays in the room. */
	inline constexpr float FloorSignInsetCm = 32.f;
	/** One digit. A two-digit number, such as 10, is two of these with a gap between. */
	inline constexpr float FloorSignDigitWidthCm = 44.f;
	inline constexpr float FloorSignDigitHeightCm = 72.f;
	inline constexpr float FloorSignDigitStrokeCm = 11.f;
	inline constexpr float FloorSignDigitGapCm = 12.f;
	/** Thickness of a stroke, toward the room from the plate. */
	inline constexpr float FloorSignDigitDepthCm = 5.f;
	/** Light plate behind the strokes. The number reads on this, not on the bare stone. */
	inline constexpr float FloorSignPlateWidthCm = 140.f;
	inline constexpr float FloorSignPlateHeightCm = 110.f;
	inline constexpr float FloorSignPlateDepthCm = 8.f;
	/** Clear space between the plate's room face and the back of the strokes. */
	inline constexpr float FloorSignPlateGapCm = 2.f;

	inline FLinearColor FloorSignPlateColor()
	{
		return FLinearColor(0.90f, 0.86f, 0.74f);
	}

	inline FColor FloorSignTextColor()
	{
		return FColor(24, 18, 12);
	}

	/**
	 * One dark bar of a floor number, in the sign's own frame.
	 * +X points into the room, +Y is to the viewer's left, +Z is up.
	 * The number is centered on the origin. The viewer's right is -Y.
	 */
	struct FFloorSignStroke
	{
		FVector CenterCm = FVector::ZeroVector;
		FVector SizeCm = FVector::OneVector;
	};

	/** Which bars a digit uses. Bit 0 is the top bar, then upper right, lower right, bottom, lower left, upper left, middle. */
	inline int32 FloorSignDigitMask(const int32 Digit)
	{
		switch (Digit)
		{
		case 0: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
		case 1: return (1 << 1) | (1 << 2);
		case 2: return (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6);
		case 3: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6);
		case 4: return (1 << 1) | (1 << 2) | (1 << 5) | (1 << 6);
		case 5: return (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6);
		case 6: return (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
		case 7: return (1 << 0) | (1 << 1) | (1 << 2);
		case 8: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
		case 9: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6);
		default: return 0;
		}
	}

	/**
	 * Dark strokes for this floor number, centered on the sign.
	 * A runtime text component on this wall did not draw, so the number is these bars.
	 */
	inline void CollectFloorSignStrokes(const int32 Number, TArray<FFloorSignStroke>& Out)
	{
		Out.Reset();
		TArray<int32, TInlineAllocator<4>> Digits;
		int32 Remaining = FMath::Max(Number, 0);
		if (Remaining == 0)
		{
			Digits.Add(0);
		}
		while (Remaining > 0)
		{
			Digits.Add(Remaining % 10);
			Remaining /= 10;
		}

		const float Pitch = FloorSignDigitWidthCm + FloorSignDigitGapCm;
		const int32 Count = Digits.Num();
		const float LeftU = -0.5f * static_cast<float>(Count - 1) * Pitch;
		const float HalfBar = FloorSignDigitWidthCm * 0.5f - FloorSignDigitStrokeCm * 0.5f;
		const float Outer = FloorSignDigitHeightCm * 0.5f - FloorSignDigitStrokeCm * 0.5f;
		const float Rise = FloorSignDigitHeightCm * 0.25f;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Digit = Digits[Count - 1 - Index];
			const int32 Mask = FloorSignDigitMask(Digit);
			const float DigitY = -(LeftU + static_cast<float>(Index) * Pitch);
			auto Add = [&](const int32 Bit, const float VisualRightCm, const float UpCm, const float WidthCm, const float HeightCm)
			{
				if ((Mask & Bit) == 0)
				{
					return;
				}
				FFloorSignStroke Stroke;
				Stroke.CenterCm = FVector(0.f, DigitY - VisualRightCm, UpCm);
				Stroke.SizeCm = FVector(FloorSignDigitDepthCm, WidthCm, HeightCm);
				Out.Add(Stroke);
			};
			Add(1 << 0, 0.f, Outer, FloorSignDigitWidthCm, FloorSignDigitStrokeCm);
			Add(1 << 3, 0.f, -Outer, FloorSignDigitWidthCm, FloorSignDigitStrokeCm);
			Add(1 << 6, 0.f, 0.f, FloorSignDigitWidthCm, FloorSignDigitStrokeCm);
			Add(1 << 1, HalfBar, Rise, FloorSignDigitStrokeCm, FloorSignDigitHeightCm * 0.5f);
			Add(1 << 2, HalfBar, -Rise, FloorSignDigitStrokeCm, FloorSignDigitHeightCm * 0.5f);
			Add(1 << 5, -HalfBar, Rise, FloorSignDigitStrokeCm, FloorSignDigitHeightCm * 0.5f);
			Add(1 << 4, -HalfBar, -Rise, FloorSignDigitStrokeCm, FloorSignDigitHeightCm * 0.5f);
		}
	}

	/** Where the light plate sits, toward the wall from the strokes. */
	inline FVector FloorSignPlateLocation(const FVector& NumberLocation, const float YawDeg)
	{
		const FVector Inward = FRotator(0.f, YawDeg, 0.f).Vector();
		const float TowardWall = FloorSignDigitDepthCm * 0.5f + FloorSignPlateGapCm + FloorSignPlateDepthCm * 0.5f;
		return NumberLocation - Inward * TowardWall;
	}

	/**
	 * The number on the wall beside this floor's landing.
	 * Absent when this floor has no landing. The text faces the room.
	 */
	struct FFloorSign
	{
		int32 Number = 0;
		FVector Location = FVector::ZeroVector;
		/** Yaw that points the text's forward axis into the room. */
		float YawDeg = 0.f;
		bool bPresent = false;
	};

	/** Flat arc in the stair band after the arriving flight, at this floor's walking surface. */
	inline FWedge LandingWedge(const FPlan& Plan, const int32 Floor)
	{
		FWedge Wedge;
		const float Sweep = LandingSweepRad(Plan, Floor);
		if (Sweep <= 0.f)
		{
			return Wedge;
		}
		Wedge.bOpen = true;
		Wedge.StartRad = FlightStartRad(Plan, Floor - 1) + FlightSweepRad(Plan, Floor - 1);
		Wedge.SweepRad = Sweep;
		return Wedge;
	}

	inline FFloorSign FloorSignFor(const FPlan& Plan, const int32 Floor)
	{
		FFloorSign Sign;
		const FWedge Landing = LandingWedge(Plan, Floor);
		if (!Landing.bOpen)
		{
			return Sign;
		}
		const float Angle = Landing.StartRad + Landing.SweepRad * 0.5f;
		const float SignZ = FloorZCm(Plan, Floor) + FloorSignLiftCm;
		const float Radius = FMath::Max(InteriorRadiusCm(Plan, SignZ) - FloorSignInsetCm, 0.f);
		const float InwardYaw = Angle + PI;
		Sign.bPresent = true;
		Sign.Number = FloorNumber(Floor);
		Sign.Location = FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, SignZ);
		Sign.YawDeg = FMath::RadiansToDegrees(InwardYaw);
		return Sign;
	}

	inline float WrapTau(const float Angle)
	{
		const float Tau = 2.f * PI;
		float Wrapped = FMath::Fmod(Angle, Tau);
		if (Wrapped < 0.f)
		{
			Wrapped += Tau;
		}
		return Wrapped;
	}

	inline bool AngleInSweep(const float Angle, const float StartRad, const float SweepRad)
	{
		if (SweepRad <= 0.f)
		{
			return false;
		}
		return WrapTau(Angle - StartRad) < SweepRad;
	}

	/** True when the hex center meets the annular sector of a flight or a hole. */
	inline bool CenterMeetsWedge(
		const FPlan& Plan,
		const FVector2D& Center,
		const float InteriorRadius,
		const FWedge& Wedge)
	{
		if (!Wedge.bOpen)
		{
			return false;
		}
		if (!AngleInSweep(FMath::Atan2(Center.Y, Center.X), Wedge.StartRad, Wedge.SweepRad))
		{
			return false;
		}
		const float Distance = Center.Size();
		const float Reach = SolidStrongholdGrid::SideCm();
		return Distance + Reach > InteriorRadius - Plan.StairWidthCm
			&& Distance - Reach < InteriorRadius;
	}

	inline FVector2D CellCenter(const int32 Col, const int32 Row)
	{
		return SolidStrongholdGrid::HexCenter(Col, Row, FVector2D::ZeroVector);
	}

	/** Inclusive column and row limit that covers the widest interior circle of this plan. */
	inline int32 HexSpan(const FPlan& Plan)
	{
		const float Radius = FMath::Max(Plan.GroundInteriorRadiusCm(), Plan.TopInteriorRadiusCm());
		const float Reach = Radius + SolidStrongholdGrid::SideCm();
		const int32 Cols = FMath::CeilToInt(Reach / SolidStrongholdGrid::FlatToFlatCm) + 1;
		const float RowStep = FMath::Max(SolidStrongholdGrid::RowStepCm(), 1.f);
		const int32 Rows = FMath::CeilToInt(Reach / RowStep) + 1;
		return FMath::Max(Cols, Rows);
	}

	/** Every vertex lies inside the interior circle at this floor's walking surface. */
	inline bool HexFitsFloor(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!IsFloor(Plan, Floor))
		{
			return false;
		}
		const FVector2D Center = CellCenter(Col, Row);
		FVector2D Vertices[6];
		SolidStrongholdGrid::HexVertices(Center, Vertices);
		const float Radius = InteriorRadiusCm(Plan, FloorZCm(Plan, Floor));
		for (const FVector2D& Vertex : Vertices)
		{
			if (Vertex.Size() > Radius + 0.01f)
			{
				return false;
			}
		}
		return true;
	}

	inline bool HexOnStair(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			FlightWedge(Plan, Floor));
	}

	inline bool HexOnGap(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			GapWedge(Plan, Floor));
	}

	/** The landing is floored, and it is not a hex. The ground floor has none. */
	inline bool HexOnLanding(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!HexFitsFloor(Plan, Floor, Col, Row))
		{
			return false;
		}
		return CenterMeetsWedge(
			Plan,
			CellCenter(Col, Row),
			InteriorRadiusCm(Plan, FloorZCm(Plan, Floor)),
			LandingWedge(Plan, Floor));
	}

	/** A floor cell: inside the circle, and neither the stair, the hole, nor the landing. */
	inline bool HexKept(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexFitsFloor(Plan, Floor, Col, Row)
			&& !HexOnStair(Plan, Floor, Col, Row)
			&& !HexOnGap(Plan, Floor, Col, Row)
			&& !HexOnLanding(Plan, Floor, Col, Row);
	}

	/** Kept cells of one floor, row then column. The stair, the hole, and the landing are absent. */
	inline void CollectKeptHexes(const FPlan& Plan, const int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes)
	{
		OutHexes.Reset();
		if (!IsFloor(Plan, Floor))
		{
			return;
		}
		const int32 Span = HexSpan(Plan);
		for (int32 Row = -Span; Row <= Span; ++Row)
		{
			for (int32 Col = -Span; Col <= Span; ++Col)
			{
				if (!HexKept(Plan, Floor, Col, Row))
				{
					continue;
				}
				SolidStrongholdGrid::FHex Hex;
				Hex.Center = CellCenter(Col, Row);
				Hex.Col = Col;
				Hex.Row = Row;
				OutHexes.Add(Hex);
			}
		}
	}

	/** How far past the proud door the captain stands on the grass. Clears the capsule. */
	inline constexpr float DoorStandClearCm = 120.f;

	inline const TCHAR* EnterLabel = TEXT("Enter Stronghold");
	inline const TCHAR* LeaveLabel = TEXT("Exit Stronghold");

	/**
	 * Grass side of the door, just past the stone.
	 * Grass-facing yaw 90 is north, so the stand point adds +Y.
	 */
	inline FVector2D OutsideDoorXY(const FVector2D& CellCenterXY, const float GrassFacingYawDeg, const FPlan& Plan)
	{
		const float YawRad = FMath::DegreesToRadians(GrassFacingYawDeg);
		const FVector2D Grass(FMath::Cos(YawRad), FMath::Sin(YawRad));
		const float Distance = Plan.GroundExteriorRadiusCm + DoorProudCm + DoorStandClearCm;
		return CellCenterXY + Grass * Distance;
	}

	/** On the grass, looking out from the door. The same yaw the mouth uses. */
	inline float OutsideDoorYawDeg(const float GrassFacingYawDeg)
	{
		return GrassFacingYawDeg;
	}

	/**
	 * Floor-0 kept hex nearest the door.
	 * The captain arrives there, and yaw 0 looks into the tower.
	 */
	inline FVector2D DoorHexCenter(const FPlan& Plan)
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		CollectKeptHexes(Plan, 0, Hexes);
		const FVector2D DoorPoint = DoorOutward(Plan) * FMath::Max(Plan.GroundInteriorRadiusCm() - 30.f, 0.f);
		FVector2D Best = FVector2D::ZeroVector;
		float BestDist = TNumericLimits<float>::Max();
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			const float Dist = FVector2D::DistSquared(Hex.Center, DoorPoint);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Best = Hex.Center;
			}
		}
		return Best;
	}
}
