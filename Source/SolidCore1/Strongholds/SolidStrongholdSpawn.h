#pragma once

#include "CoreMinimal.h"

class ASolidStrongholdExterior;
class ASolidStrongholdStone;
class ASolidTerrainStreamer;
class FSolidStronghold;
class UWorld;
struct FActorSpawnParameters;

namespace SolidStronghold
{
	/**
	 * Spawns the exterior of each company stronghold.
	 * The stronghold names the actor, the world position, and the facing.
	 */
	void SpawnStrongholds(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors);

	/**
	 * Spawns a marker stone on each stronghold site that has no exterior.
	 */
	void SpawnStrongholdStones(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdStone>>& Stones,
		bool& bStonesSpawned);
}
