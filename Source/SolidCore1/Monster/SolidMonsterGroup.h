#pragma once

#include "CoreMinimal.h"
#include "SolidMonster.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Several monsters that act together.
 * An encounter owns one or more groups. Later stories hang group behaviour
 * (battle plans, detection) on this type.
 */
class FSolidMonsterGroup
{
public:
	FSolidMonsterGroup() = default;

	/** Owned members. Empty until the encounter places them. */
	TArray<TUniquePtr<FSolidMonster>> Members;

	int32 Num() const { return Members.Num(); }

	FSolidMonster* Get(int32 Index);
	const FSolidMonster* Get(int32 Index) const;

	/**
	 * Member indexes whose fog is now below full, and that have no seeded actor yet.
	 * A point that stays at full fog is never listed.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& Seeded,
		TArray<int32>& OutReady) const;
};
