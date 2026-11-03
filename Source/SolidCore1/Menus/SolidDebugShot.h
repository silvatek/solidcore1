#pragma once

#include "CoreMinimal.h"
#include "SolidSight.h"
#include "Towns/SolidTowns.h"

/**
 * One F10 debug shot. Change these when the next picture needs a different place.
 * South is negative Y. The yaw is the same south the town signs use.
 */
namespace SolidDebugShot
{
	inline const TCHAR* Label()
	{
		return TEXT("Debug Shot");
	}

	inline constexpr int32 TownId = SolidTowns::Dragongate;
	/** 10 m south of that town's mark. */
	inline constexpr float SouthOfMarkCm = 1000.f;
	inline const TCHAR* CompanionName = TEXT("Sam");
	inline const TCHAR* FormationName = TEXT("Parade");
	inline constexpr ESolidSight Sight = ESolidSight::True;
	inline constexpr float FacingYawDeg = SolidTowns::SignFacesSouthYawDeg;
	inline constexpr float CaptureDelaySeconds = 3.f;
}
