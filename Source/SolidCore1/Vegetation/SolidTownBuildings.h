#pragma once

#include "CoreMinimal.h"

class USolidTerrainMap;

/**
 * Pack random non-overlapping building footprints onto Town TerrainPoints.
 * Pure logic — no actor spawning (safe for automation).
 */
namespace SolidTownBuildings
{
	struct FPlacement
	{
		FVector2D CenterXY = FVector2D::ZeroVector;
		float FootprintXCm = 280.f;
		float FootprintYCm = 240.f;
		float BodyHeightCm = 320.f;
		float RoofHeightCm = 140.f;
		float YawDeg = 0.f;

		float PackingRadiusCm() const
		{
			return 0.5f * FMath::Max(FootprintXCm, FootprintYCm);
		}
	};

	struct FScatterParams
	{
		/** Fraction of town grid points considered as candidate sites [0, 1]. */
		float Density = 0.35f;
		/** Hard cap on returned placements. */
		int32 MaxBuildings = 48;
		/** Extra gap between packing circles (cm). */
		float MinSeparationCm = 100.f;
		/** Keep the monolith / town centroid clear (cm). */
		float ClearRadiusAroundTownCenterCm = 450.f;
		float MinFootprintCm = 180.f;
		float MaxFootprintCm = 420.f;
		float MinBodyHeightCm = 220.f;
		float MaxBodyHeightCm = 520.f;
		float MinRoofHeightCm = 90.f;
		float MaxRoofHeightCm = 220.f;
	};

	/**
	 * Fill OutPlacements with non-overlapping buildings inside Town biomes.
	 * Deterministic for a given map + seed + params.
	 * @return Number of placements written.
	 */
	int32 CollectPlacements(
		const USolidTerrainMap* TerrainMap,
		int32 Seed,
		const FScatterParams& Params,
		TArray<FPlacement>& OutPlacements);
}
