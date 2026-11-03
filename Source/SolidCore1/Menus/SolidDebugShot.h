#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"
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

	/** True when the editor was started with -DebugShot. The shot runs, then the editor quits. */
	inline bool RequestsLaunch(const TCHAR* CommandLine)
	{
		return CommandLine && FParse::Param(CommandLine, TEXT("DebugShot"));
	}

	inline constexpr int32 TownId = SolidTowns::Dragongate;
	/** 10 m south of that town's mark. */
	inline constexpr float SouthOfMarkCm = 1000.f;
	inline const TCHAR* FormationName = TEXT("Parade");

	/** Shirt or trousers. Specular 0 and metallic 0 is the parent that stayed matte. */
	struct FCloth
	{
		float Roughness = 0.f;
		float Specular = 0.f;
		float Metallic = 0.f;
		bool bUnlit = false;
		bool bFlat = false;
	};

	inline constexpr FCloth MatteCloth{ 0.f, 0.f, 0.f, false, false };

	/** One companion in the parade line. Shirt and trousers can wear different cloth. */
	struct FWear
	{
		const TCHAR* Name = nullptr;
		FCloth Shirt;
		FCloth Trousers;
	};

	/** Parade stands every one of these in front of the captain, in their own colours, on the matte parent. */
	inline const FWear Wear[] = {
		{ TEXT("Sam"), MatteCloth, MatteCloth },
		{ TEXT("Max"), MatteCloth, MatteCloth },
		{ TEXT("Morgan"), MatteCloth, MatteCloth },
		{ TEXT("Quinn"), MatteCloth, MatteCloth },
	};
	inline constexpr ESolidSight Sight = ESolidSight::True;
	inline constexpr float FacingYawDeg = SolidTowns::SignFacesSouthYawDeg;
	inline constexpr float CaptureDelaySeconds = 4.f;
}
