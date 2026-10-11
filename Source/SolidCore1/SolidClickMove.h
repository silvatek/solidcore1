#pragma once

#include "CoreMinimal.h"

/**
 * Click-to-move, raven sight only. A left click picks a point on the ground.
 * The captain walks the straight line. There is no navigation mesh.
 */
namespace SolidClickMove
{
	/** Stop steering once the captain is this close on the ground plane (cm). */
	inline constexpr float ArrivalRadiusCm = 90.f;

	/** Longest ray from the camera (cm). The world is 1024 m on a side. */
	inline constexpr float MaxRayCm = 80000.f;

	/** March step when intersecting the height field (cm). */
	inline constexpr float RayStepCm = 100.f;

	/** True sight, the menu, and the formation drill do not take a click. */
	inline bool AllowsClick(const bool bRavenSight, const bool bBlocked)
	{
		return bRavenSight && !bBlocked;
	}

	inline bool HasArrived(const FVector2D& From, const FVector2D& To, const float RadiusCm = ArrivalRadiusCm)
	{
		return FVector2D::DistSquared(From, To) <= RadiusCm * RadiusCm;
	}

	/** Unit ground direction, or zero once the captain has arrived. */
	inline FVector2D PlanarDirection(const FVector2D& From, const FVector2D& To, const float RadiusCm = ArrivalRadiusCm)
	{
		if (HasArrived(From, To, RadiusCm))
		{
			return FVector2D::ZeroVector;
		}
		return (To - From).GetSafeNormal();
	}

	/**
	 * First place a ray drops from above a height field onto it.
	 * HeightAt(X, Y) is the ground Z in cm. OutPoint sits on that surface.
	 */
	template<typename FHeightAt>
	bool IntersectHeight(
		const FVector& Origin,
		const FVector& Direction,
		FHeightAt HeightAt,
		FVector& OutPoint,
		const float MaxDistanceCm = MaxRayCm,
		const float StepCm = RayStepCm)
	{
		const FVector Dir = Direction.GetSafeNormal();
		if (Dir.IsNearlyZero() || StepCm <= 0.f || MaxDistanceCm <= 0.f)
		{
			return false;
		}

		float PrevT = 0.f;
		float PrevDelta = Origin.Z - HeightAt(Origin.X, Origin.Y);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(MaxDistanceCm / StepCm));
		for (int32 Index = 1; Index <= Steps; ++Index)
		{
			const float T = FMath::Min(MaxDistanceCm, StepCm * static_cast<float>(Index));
			const FVector Point = Origin + Dir * T;
			const float Delta = Point.Z - HeightAt(Point.X, Point.Y);
			if (PrevDelta >= 0.f && Delta <= 0.f)
			{
				const float Span = PrevDelta - Delta;
				const float Alpha = FMath::IsNearlyZero(Span) ? 1.f : FMath::Clamp(PrevDelta / Span, 0.f, 1.f);
				const float HitT = FMath::Lerp(PrevT, T, Alpha);
				const FVector Hit = Origin + Dir * HitT;
				OutPoint = FVector(Hit.X, Hit.Y, HeightAt(Hit.X, Hit.Y));
				return true;
			}
			PrevT = T;
			PrevDelta = Delta;
		}
		return false;
	}
}
