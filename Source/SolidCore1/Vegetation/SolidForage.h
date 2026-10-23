#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidItemType.h"
#include "SolidTerrainTypes.h"

class USolidGameState;
class USolidWorldChart;

/**
 * Sticks and pitch lying on forest ground.
 * The scatter chooses points. A pile is seeded only after that point's fog falls below full.
 */
namespace SolidForage
{
	/** Pile shuffle. Separate from the tree seed, so a tree-seed change does not reshuffle these. */
	inline constexpr int32 Seed = 43;
	/** Fraction of treeless forest points that receive a pile. */
	inline constexpr float Density = 0.05f;
	/** Hard cap on the scattered piles. The Iglin forest guarantee is not part of this cap. */
	inline constexpr int32 MaxPiles = 64;
	/** Forest cells this close to Iglin each keep a stick and a pitch (cm). */
	inline constexpr float IglinRadiusCm = 10000.f;
	/** Captain ground distance that names a pile and allows a click to take it (cm). */
	inline constexpr float ReachCm = 400.f;
	/** Placeholder cube, on a side (cm). */
	inline constexpr float CubeCm = 40.f;
	inline constexpr const TCHAR* CubeMesh = TEXT("/Engine/BasicShapes/Cube.Cube");

	struct FScatterParams
	{
		float Density = SolidForage::Density;
		int32 MaxPiles = SolidForage::MaxPiles;
		float JitterCm = 90.f;
	};

	/** One chosen pile. The index in the scatter list is the slot a pickup remembers. */
	struct FChoice
	{
		int32 PointIndex = INDEX_NONE;
		FVector2D XY = FVector2D::ZeroVector;
		const TCHAR* ItemId = nullptr;
	};

	/** This story places a stick or a pitch, and only on forest. Sea, road, and coins are not finds. */
	inline bool OccursInBiome(const TCHAR* ItemId, const ESolidBiome Biome)
	{
		if (Biome != ESolidBiome::Forest || !ItemId || !ItemId[0])
		{
			return false;
		}
		return FCString::Strcmp(ItemId, SolidItemType::IdStick) == 0
			|| FCString::Strcmp(ItemId, SolidItemType::IdPitch) == 0;
	}

	/** Every id uses the cube until a later story gives each kind its own mesh. */
	inline const TCHAR* MeshPath(const TCHAR* /*ItemId*/)
	{
		return CubeMesh;
	}

	/** A closed visit has no piles. */
	inline bool WorldHasPiles(const bool bClosedVisit)
	{
		return !bClosedVisit;
	}

	inline bool IsWithinReach(const float GroundDistanceCm)
	{
		return GroundDistanceCm <= ReachCm;
	}

	/**
	 * Choose piles on forest points that did not receive a tree.
	 * An even index is a stick and an odd index is a pitch. The first is a stick.
	 * @return Number of choices written.
	 */
	int32 CollectChoices(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		int32 ChoiceSeed,
		const FScatterParams& Params,
		TArray<FChoice>& OutChoices);

	/**
	 * Append a stick and a pitch for each world-map forest cell within RadiusCm of TownXY,
	 * when that cell does not already have one. Treeless forest points only. No jitter.
	 * @return Number of piles appended.
	 */
	int32 GuaranteeForestCells(
		const TArray<FSolidTerrainPoint>& Points,
		const USolidWorldChart* WorldMap,
		const FVector2D& WorldMinXY,
		const FVector2D& WorldMaxXY,
		const FVector2D& TownXY,
		float RadiusCm,
		const TArray<int32>& TreePointIndices,
		TArray<FChoice>& InOutChoices);

	/**
	 * Choice indexes whose fog is now below full, that are not taken, and that have no pile yet.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<FChoice>& Choices,
		const TArray<int32>& Taken,
		const TArray<int32>& Seeded,
		TArray<int32>& OutReady);

	/**
	 * Put one pile into the pack and remember its scatter index.
	 * False when that index was already taken or the pack cannot hold the item.
	 * Writes no journal line.
	 */
	bool TakePile(USolidGameState& State, int32 ChoiceIndex, const TCHAR* ItemId);

	/** Entry distance along a ray, or false when the ray misses the box. Inside the box is 0. */
	inline bool RayBoxDistance(
		const FVector& Origin,
		const FVector& Direction,
		const FBox& Box,
		const float MaxDistanceCm,
		float& OutDistanceCm)
	{
		OutDistanceCm = 0.f;
		const bool bHasExtent = Box.Min.X <= Box.Max.X && Box.Min.Y <= Box.Max.Y && Box.Min.Z <= Box.Max.Z;
		if (!bHasExtent || MaxDistanceCm <= 0.f)
		{
			return false;
		}
		if (Box.IsInsideOrOn(Origin))
		{
			return true;
		}

		const FVector Dir = Direction.GetSafeNormal();
		if (Dir.IsNearlyZero())
		{
			return false;
		}

		const float OriginAxis[3] = { Origin.X, Origin.Y, Origin.Z };
		const float DirAxis[3] = { Dir.X, Dir.Y, Dir.Z };
		const float MinAxis[3] = { Box.Min.X, Box.Min.Y, Box.Min.Z };
		const float MaxAxis[3] = { Box.Max.X, Box.Max.Y, Box.Max.Z };

		float TMin = 0.f;
		float TMax = MaxDistanceCm;
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			if (FMath::Abs(DirAxis[Axis]) < KINDA_SMALL_NUMBER)
			{
				if (OriginAxis[Axis] < MinAxis[Axis] || OriginAxis[Axis] > MaxAxis[Axis])
				{
					return false;
				}
				continue;
			}

			float Near = (MinAxis[Axis] - OriginAxis[Axis]) / DirAxis[Axis];
			float Far = (MaxAxis[Axis] - OriginAxis[Axis]) / DirAxis[Axis];
			if (Near > Far)
			{
				const float SwapNear = Near;
				Near = Far;
				Far = SwapNear;
			}
			TMin = FMath::Max(TMin, Near);
			TMax = FMath::Min(TMax, Far);
			if (TMin > TMax)
			{
				return false;
			}
		}
		if (TMax < 0.f)
		{
			return false;
		}
		OutDistanceCm = FMath::Max(TMin, 0.f);
		return true;
	}
}
