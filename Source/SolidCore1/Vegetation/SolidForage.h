#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidItemType.h"
#include "SolidTerrainTypes.h"

struct FGameState;

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
	/** Hard cap on chosen piles. */
	inline constexpr int32 MaxPiles = 64;
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

	/** A pile the pointer ray meets. Ray distance picks the nearest. Ground distance is the 4 m test. */
	struct FPileHit
	{
		int32 Index = INDEX_NONE;
		float RayDistanceCm = 0.f;
		float GroundDistanceCm = 0.f;
	};

	enum class EPointerAct : uint8
	{
		/** Not a pile in reach. A raven click may still walk. */
		None,
		/** Take the pile and stop a walk already under way. */
		Take,
		/** The pile is in reach and the pack cannot hold it. The click changes nothing. */
		Leave,
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

	/** The dungeon, the tower, and the Iglin tunnels have no piles. */
	inline bool WorldHasPiles(const bool bDungeon, const bool bTower, const bool bLair)
	{
		return !bDungeon && !bTower && !bLair;
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
		int32 Seed,
		const FScatterParams& Params,
		TArray<FChoice>& OutChoices);

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
	bool TakePile(FGameState& State, int32 ChoiceIndex, const TCHAR* ItemId);

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

	/** Smallest ray distance. The 4 m test is applied to that hit, not to a later one. */
	inline int32 NearestHitIndex(const TArray<FPileHit>& Hits)
	{
		int32 Best = INDEX_NONE;
		float BestRay = 0.f;
		for (int32 Index = 0; Index < Hits.Num(); ++Index)
		{
			if (Hits[Index].Index == INDEX_NONE)
			{
				continue;
			}
			if (Best == INDEX_NONE || Hits[Index].RayDistanceCm < BestRay)
			{
				Best = Index;
				BestRay = Hits[Index].RayDistanceCm;
			}
		}
		return Best;
	}

	/**
	 * What a click does to a pile. A passage, a hidden pointer, or a menu is not a pickup.
	 * OutChoiceIndex is the scatter index when the act is Take or Leave.
	 */
	inline EPointerAct ResolvePileClick(
		const bool bPointerAccepts,
		const bool bPassage,
		const TArray<FPileHit>& Hits,
		const bool bPackCanHold,
		int32& OutChoiceIndex)
	{
		OutChoiceIndex = INDEX_NONE;
		if (!bPointerAccepts || bPassage)
		{
			return EPointerAct::None;
		}

		const int32 HitIndex = NearestHitIndex(Hits);
		if (HitIndex == INDEX_NONE || !IsWithinReach(Hits[HitIndex].GroundDistanceCm))
		{
			return EPointerAct::None;
		}

		OutChoiceIndex = Hits[HitIndex].Index;
		return bPackCanHold ? EPointerAct::Take : EPointerAct::Leave;
	}

	/**
	 * A raven click walks when the pile did not take the click.
	 * A passage still wins, and true sight does not walk.
	 */
	inline bool ClickStartsWalk(const bool bRavenSight, const bool bPassage, const EPointerAct Act)
	{
		return bRavenSight && !bPassage && Act == EPointerAct::None;
	}
}
