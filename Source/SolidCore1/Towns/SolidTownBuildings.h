#pragma once

#include "CoreMinimal.h"
#include "SolidTownSigns.h"

class USolidTerrainMap;
class USolidWorldChart;

/**
 * Pack building footprints onto town points.
 * The number of buildings is a share of the town points left after roads and signs.
 * A footprint that would cover a routed road is slid off that road, still on a town point.
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
		/** Share of the open town points that receive a building [0, 1]. */
		float Density = 0.35f;
		/** Extra gap between packing circles (cm). */
		float MinSeparationCm = 100.f;
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
	 * Fill OutPlacements with buildings on town points.
	 * An open point has a town biome, is not on a routed road, and is not under a welcome sign.
	 * The count is Density times the open points.
	 * A point that would overlap a building already placed is skipped.
	 * Deterministic for a given map, seed, and params.
	 * @return Number of placements written.
	 */
	int32 CollectPlacements(
		const USolidTerrainMap* TerrainMap,
		int32 Seed,
		const FScatterParams& Params,
		TArray<FPlacement>& OutPlacements,
		const TArray<SolidTownSigns::FPlacement>& Signs = TArray<SolidTownSigns::FPlacement>());
}
