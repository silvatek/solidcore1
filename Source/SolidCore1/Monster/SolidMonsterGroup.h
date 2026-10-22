#pragma once

#include "CoreMinimal.h"
#include "SolidMonster.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Several monsters that act together.
 * The three raptors beside the Iglin–Relion bridge are one group.
 * Later stories hang group behaviour (battle plans, detection) on this type.
 */
class FSolidMonsterGroup
{
public:
	FSolidMonsterGroup() = default;

	/** Nearest a member may be to the Iglin ramp's outer end (cm). */
	static constexpr float NearLimitCm = 800.f;
	/** Farthest a member may be from that outer end (cm). */
	static constexpr float FarLimitCm = 4000.f;
	/** Later members stay at least this far from one already chosen (cm). */
	static constexpr float SeparationCm = 400.f;
	/** How many raptors stand beside the Iglin ramp. */
	static constexpr int32 BesideIglinCount = 3;

	/** Owned members. Empty until PlaceBesideIglinRamp fills them. */
	TArray<TUniquePtr<FSolidMonster>> Members;

	/** A closed visit has no monsters. The tunnels stay empty. */
	static bool WorldHasMonsters(bool bClosedVisit);

	int32 Num() const { return Members.Num(); }

	FSolidMonster* Get(int32 Index);
	const FSolidMonster* Get(int32 Index) const;

	/**
	 * The nearest grassland points to the Iglin ramp's outer end, each a raptor.
	 * Walkable grassland only, inside the band, clear of trees and foraging piles, and 4 m apart.
	 * Replaces Members. A bridge list with no Iglin crossing leaves Members empty.
	 * @return Number of members placed.
	 */
	int32 PlaceBesideIglinRamp(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges);

	/**
	 * Member indexes whose fog is now below full, and that have no seeded actor yet.
	 * A point that stays at full fog is never listed.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& Seeded,
		TArray<int32>& OutReady) const;
};
