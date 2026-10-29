#pragma once

#include "CoreMinimal.h"

namespace SolidSight
{
	/** Falcon camera height above the ground under the camera (cm). */
	inline constexpr float FalconMinHeightCm = 2000.f;
	inline constexpr float FalconMaxHeightCm = 20000.f;

	/** Wheel position in the raven zoom range, as 0 at the near end and 1 at the far end. */
	inline float FalconZoomAlpha(const float ArmCm, const float ZoomMin, const float ZoomMax)
	{
		const float Span = ZoomMax - ZoomMin;
		if (Span <= KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		return FMath::Clamp((ArmCm - ZoomMin) / Span, 0.f, 1.f);
	}

	/** 20 m at alpha 0, 200 m at alpha 1, above the ground under the camera. */
	inline float FalconHeightCm(const float ZoomAlpha)
	{
		return FMath::Lerp(FalconMinHeightCm, FalconMaxHeightCm, FMath::Clamp(ZoomAlpha, 0.f, 1.f));
	}

	/**
	 * Walk from the captain opposite the look's yaw. Pitch is ignored.
	 * Stop at the last point whose fog is 0, and do not pass MaxCm.
	 */
	template<typename FogAtFn>
	inline FVector2D FalconClearXY(
		const FVector2D& Captain,
		const FRotator& Look,
		const float StepCm,
		const float MaxCm,
		FogAtFn FogAt)
	{
		const FVector Back = -FRotator(0.f, Look.Yaw, 0.f).Vector();
		const FVector2D Dir(Back.X, Back.Y);
		if (Dir.SizeSquared() <= KINDA_SMALL_NUMBER || MaxCm <= 0.f || FogAt(Captain) > 0.f)
		{
			return Captain;
		}

		const float Step = FMath::Max(StepCm, 1.f);
		const int32 Steps = FMath::Max(FMath::CeilToInt(MaxCm / Step), 1);
		FVector2D Clear = Captain;
		for (int32 Index = 1; Index <= Steps; ++Index)
		{
			const float Dist = FMath::Min(static_cast<float>(Index) * Step, MaxCm);
			const FVector2D At = Captain + Dir * Dist;
			if (FogAt(At) > 0.f)
			{
				return Clear;
			}
			Clear = At;
		}
		return Clear;
	}

	/** Look from the falcon camera toward the captain. Level roll. */
	inline FRotator FalconLookAtCaptain(const FVector& Camera, const FVector& Captain)
	{
		const FVector ToCaptain = Captain - Camera;
		if (ToCaptain.IsNearlyZero())
		{
			return FRotator::ZeroRotator;
		}
		const FRotator Look = ToCaptain.Rotation();
		return FRotator(Look.Pitch, Look.Yaw, 0.f);
	}
}
