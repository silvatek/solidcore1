#pragma once

#include "CoreMinimal.h"

/** Raven sight is the third-person party camera. True sight is first person. */
enum class ESolidSight : uint8
{
	Raven,
	True,
};

/** F9 view toggle. Pure helpers — no actor spawning (safe for automation). */
namespace SolidSight
{
	/** Eye height above the capsule center (cm). */
	inline constexpr float TrueSightEyeHeightCm = 64.f;

	/** Camera sits this far along the look direction so it clears the near clip (cm). */
	inline constexpr float TrueSightForwardCm = 12.f;

	inline bool IsTrueSight(ESolidSight Sight)
	{
		return Sight == ESolidSight::True;
	}

	inline ESolidSight Toggle(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? ESolidSight::Raven : ESolidSight::True;
	}

	/**
	 * Look used after a sight change. True sight is horizontal: pitch and roll are 0.
	 * Yaw stays, so the captain still faces the same way. Raven sight keeps the tilt.
	 */
	inline FRotator ViewForSight(const ESolidSight Sight, const FRotator& View)
	{
		if (!IsTrueSight(Sight))
		{
			return View;
		}
		return FRotator(0.f, View.Yaw, 0.f);
	}

	inline const TCHAR* Label(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? TEXT("True sight") : TEXT("Raven sight");
	}

	/** Top-right marker. Eye for true sight, eagle for raven sight. */
	inline const TCHAR* Marker(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? TEXT("\U0001F441") : TEXT("\U0001F985");
	}
}
