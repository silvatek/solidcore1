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
	inline const TCHAR* FormationName = TEXT("Parade");

	/** One slot of the clothing comparison. Flat uses the flat colour. Unlit drops the light. */
	struct FCloth
	{
		float Roughness = 0.f;
		float Specular = 0.f;
		float Metallic = 0.f;
		bool bUnlit = false;
		bool bFlat = false;
	};

	/** One companion in the parade line. Shirt and trousers can wear different cloth. */
	struct FWear
	{
		const TCHAR* Name = nullptr;
		FCloth Shirt;
		FCloth Trousers;
	};

	/**
	 * Parade stands every one of these in front of the captain.
	 * Sam compares roughness 0 with roughness 1. Max compares unlit with the flat colour.
	 * Morgan compares specular 1 with metallic 1. Quinn compares specular 1 at roughness 1 with unlit.
	 */
	inline const FWear Wear[] = {
		{ TEXT("Sam"), { 0.f, 0.f, 0.f, false, false }, { 1.f, 0.f, 0.f, false, false } },
		{ TEXT("Max"), { 0.f, 0.f, 0.f, true, false }, { 0.f, 0.f, 0.f, false, true } },
		{ TEXT("Morgan"), { 0.f, 1.f, 0.f, false, false }, { 0.f, 0.f, 1.f, false, false } },
		{ TEXT("Quinn"), { 1.f, 1.f, 0.f, false, false }, { 0.f, 0.f, 0.f, true, false } },
	};
	inline constexpr ESolidSight Sight = ESolidSight::True;
	inline constexpr float FacingYawDeg = SolidTowns::SignFacesSouthYawDeg;
	inline constexpr float CaptureDelaySeconds = 3.f;
}
