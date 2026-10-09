#pragma once

#include "CoreMinimal.h"

class USolidTerrainMap;

/**
 * Pick random spawn XY positions on Forest TerrainPoints.
 * Pure logic — no actor spawning (safe for automation).
 */
namespace SolidForestTrees
{
	struct FScatterParams
	{
		/** Fraction of forest grid points that receive a tree [0, 1]. */
		float Density = 0.15f;
		/** Hard cap on returned positions. */
		int32 MaxTrees = 256;
		/** Planar jitter around each chosen grid point (cm). */
		float JitterCm = 90.f;
	};

	/**
	 * Fill OutXY with world-space positions inside Forest biomes.
	 * Deterministic for a given map + seed + params.
	 * @return Number of positions written.
	 */
	int32 CollectSpawnPositions(
		const USolidTerrainMap* TerrainMap,
		int32 Seed,
		const FScatterParams& Params,
		TArray<FVector2D>& OutXY);
}
