#pragma once

#include "CoreMinimal.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Where the three reptiles stand, and how tall one is.
 * The choice is pure data. The actor is spawned later, and only on the open world.
 */
namespace SolidMonster
{
	/** Standing height of the downloaded mesh after a uniform scale (cm). */
	inline constexpr float StandingHeightCm = 220.f;
	/** Capsule radius. The actor origin is the feet, so the capsule sits above it (cm). */
	inline constexpr float CapsuleRadiusCm = 60.f;
	/** Half of the standing height. The capsule bottom is the actor origin (cm). */
	inline constexpr float CapsuleHalfHeightCm = StandingHeightCm * 0.5f;
	/** Nearest a point may be to the Iglin ramp's outer end (cm). */
	inline constexpr float NearLimitCm = 800.f;
	/** Farthest a point may be from that outer end (cm). */
	inline constexpr float FarLimitCm = 4000.f;
	/** Later points stay at least this far from one already chosen (cm). */
	inline constexpr float SeparationCm = 400.f;
	/** How many reptiles stand beside the Iglin ramp. */
	inline constexpr int32 Count = 3;

	/**
	 * Marketplace skeletons face the mesh +Y.
	 * This yaw lines that forward up with the actor, which looks at the ramp.
	 */
	inline constexpr float MeshFacingYawDeg = -90.f;

	/** One chosen standing place. Yaw looks at the Iglin ramp's outer end. */
	struct FChoice
	{
		int32 PointIndex = INDEX_NONE;
		FVector2D XY = FVector2D::ZeroVector;
		/** Degrees. Yaw only. */
		float YawDeg = 0.f;
	};

	/** A closed visit has no monsters. The tunnels stay empty. */
	inline bool WorldHasMonsters(const bool bClosedVisit)
	{
		return !bClosedVisit;
	}

	/** Uniform scale that makes a mesh bounds height the standing height. A zero height stays at 1. */
	inline float UniformScale(const float BoundsHeightCm)
	{
		if (BoundsHeightCm <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}
		return StandingHeightCm / BoundsHeightCm;
	}

	/**
	 * Relative Z that puts the scaled bounds bottom on the actor origin.
	 * BoundsOriginZ and BoundsExtentZ are the mesh bounds before the scale.
	 */
	inline float MeshFootOffsetCm(const float BoundsOriginZ, const float BoundsExtentZ, const float Scale)
	{
		const float Bottom = BoundsOriginZ - BoundsExtentZ;
		return -Bottom * Scale;
	}

	/**
	 * The nearest grassland points to the Iglin ramp's outer end.
	 * Walkable grassland only, inside the band, clear of trees and foraging piles, and 4 m apart.
	 * A bridge list with no Iglin crossing writes nothing.
	 * @return Number of choices written.
	 */
	int32 CollectChoices(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges,
		TArray<FChoice>& OutChoices);

	/**
	 * Choice indexes whose fog is now below full, and that have no monster yet.
	 * A point that stays at full fog is never listed.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<FChoice>& Choices,
		const TArray<int32>& Seeded,
		TArray<int32>& OutReady);
}
