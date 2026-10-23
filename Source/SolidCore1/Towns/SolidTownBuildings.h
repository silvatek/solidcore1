#pragma once

#include "CoreMinimal.h"

class USolidTerrainMap;
class USolidWorldChart;

/**
 * Pack random non-overlapping building footprints onto marked town cells.
 * A footprint that would cover a road is slid off that road, still in the town.
 * Pure logic — no actor spawning (safe for automation).
 */
namespace SolidTownBuildings
{
	/** How far a building may move to leave the road, in centimetres. */
	inline constexpr float RoadSlideCm = 800.f;
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
		/** Keep the town centroid clear (cm). */
		float ClearRadiusAroundTownCenterCm = 450.f;
		float MinFootprintCm = 180.f;
		float MaxFootprintCm = 420.f;
		float MinBodyHeightCm = 220.f;
		float MaxBodyHeightCm = 520.f;
		float MinRoofHeightCm = 90.f;
		float MaxRoofHeightCm = 220.f;
	};

	/**
	 * True when the footprint meets the painted road, including a small verge.
	 * The strip sits half a terrain step southwest of the road line.
	 * OutOnRoad, when set, is the middle of the part that meets the road.
	 */
	bool CoversRoad(
		const USolidWorldChart& Chart,
		const FPlacement& Placement,
		FVector2D WorldMin,
		FVector2D WorldMax,
		FVector2D* OutOnRoad = nullptr);

	/**
	 * Fill OutPlacements with non-overlapping buildings on WorldMap town markers.
	 * The town marker and a ! named as a town count. A road strip does not, even where it is brown.
	 * Deterministic for a given map + seed + params.
	 * ExtraClearCenters use the same radius as the start-town centroid (welcome signs).
	 * @return Number of placements written.
	 */
	int32 CollectPlacements(
		const USolidTerrainMap* TerrainMap,
		int32 Seed,
		const FScatterParams& Params,
		TArray<FPlacement>& OutPlacements,
		const TArray<FVector2D>& ExtraClearCenters = TArray<FVector2D>());
}
