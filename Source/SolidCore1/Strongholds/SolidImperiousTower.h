#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdGrid.h"

/**
 * A round tower, measured in the tower's own plan.
 * FPlan is one size. Imperious is the shipped tower. A smaller tower is another FPlan.
 * Local +X runs in from the door. Yaw 0 looks into the tower, the same way a dungeon visit does.
 * Placement turns the whole tower so this door faces the grass. On cell 17,31 that is north.
 * The Meagre den stays the fourteen-cell outline. This floor uses the same 1.2 m pointy-top grid,
 * measured from the tower center.
 */
namespace SolidImperiousTower
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

	/** The shipped tower. Ten floors, 10 m across at the grass, 6 m across at the cap. */
	inline const FPlan Imperious{};

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

	/** Where this floor's flight starts. Each flight begins where the one below ended. */
	inline float FlightStartRad(const FPlan& Plan, const int32 Floor)
	{
		float Start = Plan.DoorAngleRad + Plan.StairPastDoorRad;
		const int32 Last = FMath::Clamp(Floor, 0, FMath::Max(Plan.FloorCount - 1, 0));
		for (int32 Index = 0; Index < Last; ++Index)
		{
			Start += FlightSweepRad(Plan, Index);
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

	/** The hole for the flight that arrives from the floor below. The ground floor has none. */
	inline FWedge GapWedge(const FPlan& Plan, const int32 Floor)
	{
		if (Floor <= 0)
		{
			return FWedge{};
		}
		return FlightWedge(Plan, Floor - 1);
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

	/** A floor cell: inside the circle, and neither the stair nor the hole. */
	inline bool HexKept(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexFitsFloor(Plan, Floor, Col, Row)
			&& !HexOnStair(Plan, Floor, Col, Row)
			&& !HexOnGap(Plan, Floor, Col, Row);
	}

	/** Kept cells of one floor, row then column. The stair and the hole are absent. */
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
}
