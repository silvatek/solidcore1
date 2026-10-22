#pragma once

#include "CoreMinimal.h"
#include "BattlePlans/SolidBattlePlan.h"
#include "SolidMonster.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Several monsters that act together.
 * An encounter owns one or more groups. Detection comes in a later story.
 */
class FSolidMonsterGroup
{
public:
	FSolidMonsterGroup() = default;

	/** Owned members. Empty until the encounter places them. */
	TArray<TUniquePtr<FSolidMonster>> Members;

	/** Member indexes that already have an actor this visit. Place starts this empty. */
	TArray<int32> Seeded;

	/** Plans this group can use. */
	TArray<FSolidBattlePlan> Available;

	/** Index into Available. INDEX_NONE while none is active. */
	int32 ActivePlan = INDEX_NONE;

	/** The active plan, or null when none is active. */
	const FSolidBattlePlan* GetActivePlan() const;

	int32 Num() const { return Members.Num(); }

	FSolidMonster* Get(int32 Index);
	const FSolidMonster* Get(int32 Index) const;

	/**
	 * Member indexes whose fog is now below full, and that have no seeded actor yet.
	 * A point that stays at full fog is never listed.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& AlreadySeeded,
		TArray<int32>& OutReady) const;
};
