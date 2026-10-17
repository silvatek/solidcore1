#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdGrid.h"

/**
 * The Imperious Tower, measured in the tower's own plan.
 * Local +X runs in from the door. Yaw 0 looks into the tower, the same way a dungeon visit does.
 * Placement turns the whole tower so this door faces the grass. On cell 17,31 that is north.
 * The Meagre den stays the fourteen-cell outline. This floor uses the same 1.2 m pointy-top grid,
 * measured from the tower center.
 */
namespace SolidImperiousTower
{
	/** Outside of the stone, ground to the flat cap. */
	inline constexpr float HeightCm = 3000.f;
	inline constexpr int32 FloorCount = 10;
	/** Walking surface to the walking surface above. */
	inline constexpr float FloorPitchCm = 300.f;
	inline constexpr float SlabCm = 20.f;
	/** Clear room under the slab. */
	inline constexpr float ClearHeightCm = FloorPitchCm - SlabCm;
	/** Exterior radius minus interior radius, at every height. */
	inline constexpr float WallCm = 50.f;

	inline constexpr float GroundExteriorRadiusCm = 500.f;
	inline constexpr float GroundInteriorRadiusCm = 450.f;
	inline constexpr float TopExteriorRadiusCm = 300.f;
	inline constexpr float TopInteriorRadiusCm = 250.f;

	/** Same clear opening as the cave door. */
	inline constexpr float DoorWidthCm = 140.f;
	inline constexpr float DoorHeightCm = 240.f;

	/** The flight occupies this band inside the wall. */
	inline constexpr float StairWidthCm = 140.f;
	inline constexpr int32 StairRiserCount = 17;
	inline constexpr int32 StairTreadCount = 16;
	inline constexpr float StairTreadCm = 26.f;
	inline constexpr float StairRunCm = static_cast<float>(StairTreadCount) * StairTreadCm;

	/** Out through the door, local -X. */
	inline constexpr float DoorAngleRad = PI;
	/** The first flight starts this far past the door center, then the spiral continues. */
	inline constexpr float StairPastDoorRad = 20.f * PI / 180.f;

	inline bool IsFloor(const int32 Floor)
	{
		return Floor >= 0 && Floor < FloorCount;
	}

	/** Walking surface of a floor. Floor 0 is the ground. Floor 9 is the top room. */
	inline float FloorZCm(const int32 Floor)
	{
		return static_cast<float>(Floor) * FloorPitchCm;
	}

	inline float ExteriorRadiusCm(const float ZCm)
	{
		const float Along = FMath::Clamp(ZCm / HeightCm, 0.f, 1.f);
		return FMath::Lerp(GroundExteriorRadiusCm, TopExteriorRadiusCm, Along);
	}

	inline float InteriorRadiusCm(const float ZCm)
	{
		const float Along = FMath::Clamp(ZCm / HeightCm, 0.f, 1.f);
		return FMath::Lerp(GroundInteriorRadiusCm, TopInteriorRadiusCm, Along);
	}

	inline float RiserCm()
	{
		return FloorPitchCm / static_cast<float>(StairRiserCount);
	}

	/** The ground floor only. */
	inline bool HasDoor(const int32 Floor)
	{
		return Floor == 0;
	}

	/** Direction from the center out through the door. */
	inline FVector2D DoorOutward()
	{
		return FVector2D(FMath::Cos(DoorAngleRad), FMath::Sin(DoorAngleRad));
	}

	struct FWedge
	{
		float StartRad = 0.f;
		float SweepRad = 0.f;
		/** False when this floor has no such opening. */
		bool bOpen = false;
	};

	/** Arc of the flight that leaves this floor. The top floor has none. */
	inline float FlightSweepRad(const int32 Floor)
	{
		if (Floor < 0 || Floor >= FloorCount - 1)
		{
			return 0.f;
		}
		const float Centerline = InteriorRadiusCm(FloorZCm(Floor)) - StairWidthCm * 0.5f;
		return StairRunCm / Centerline;
	}

	/** Where this floor's flight starts. Each flight begins where the one below ended. */
	inline float FlightStartRad(const int32 Floor)
	{
		float Start = DoorAngleRad + StairPastDoorRad;
		const int32 Last = FMath::Clamp(Floor, 0, FloorCount - 1);
		for (int32 Index = 0; Index < Last; ++Index)
		{
			Start += FlightSweepRad(Index);
		}
		return Start;
	}

	inline FWedge FlightWedge(const int32 Floor)
	{
		FWedge Wedge;
		if (Floor < 0 || Floor >= FloorCount - 1)
		{
			return Wedge;
		}
		Wedge.bOpen = true;
		Wedge.StartRad = FlightStartRad(Floor);
		Wedge.SweepRad = FlightSweepRad(Floor);
		return Wedge;
	}

	/** The hole for the flight that arrives from the floor below. The ground floor has none. */
	inline FWedge GapWedge(const int32 Floor)
	{
		if (Floor <= 0)
		{
			return FWedge{};
		}
		return FlightWedge(Floor - 1);
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
	inline bool CenterMeetsWedge(const FVector2D& Center, const float InteriorRadius, const FWedge& Wedge)
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
		return Distance + Reach > InteriorRadius - StairWidthCm
			&& Distance - Reach < InteriorRadius;
	}

	inline FVector2D CellCenter(const int32 Col, const int32 Row)
	{
		return SolidStrongholdGrid::HexCenter(Col, Row, FVector2D::ZeroVector);
	}

	/** Every vertex lies inside the interior circle at this floor's walking surface. */
	inline bool HexFitsFloor(const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!IsFloor(Floor))
		{
			return false;
		}
		const FVector2D Center = CellCenter(Col, Row);
		FVector2D Vertices[6];
		SolidStrongholdGrid::HexVertices(Center, Vertices);
		const float Radius = InteriorRadiusCm(FloorZCm(Floor));
		for (const FVector2D& Vertex : Vertices)
		{
			if (Vertex.Size() > Radius + 0.01f)
			{
				return false;
			}
		}
		return true;
	}

	inline bool HexOnStair(const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!HexFitsFloor(Floor, Col, Row))
		{
			return false;
		}
		return CenterMeetsWedge(CellCenter(Col, Row), InteriorRadiusCm(FloorZCm(Floor)), FlightWedge(Floor));
	}

	inline bool HexOnGap(const int32 Floor, const int32 Col, const int32 Row)
	{
		if (!HexFitsFloor(Floor, Col, Row))
		{
			return false;
		}
		return CenterMeetsWedge(CellCenter(Col, Row), InteriorRadiusCm(FloorZCm(Floor)), GapWedge(Floor));
	}

	/** A floor cell: inside the circle, and neither the stair nor the hole. */
	inline bool HexKept(const int32 Floor, const int32 Col, const int32 Row)
	{
		return HexFitsFloor(Floor, Col, Row) && !HexOnStair(Floor, Col, Row) && !HexOnGap(Floor, Col, Row);
	}

	/** Kept cells of one floor, row then column. The stair and the hole are absent. */
	inline void CollectKeptHexes(const int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes)
	{
		OutHexes.Reset();
		if (!IsFloor(Floor))
		{
			return;
		}
		for (int32 Row = -12; Row <= 12; ++Row)
		{
			for (int32 Col = -12; Col <= 12; ++Col)
			{
				if (!HexKept(Floor, Col, Row))
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
