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
	 * Also spawns a marker stone on a stronghold site that has no exterior.
	 * The stronghold names the actor, the world position, and the facing.
	 */
	void SpawnStrongholds(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors,
		TArray<TObjectPtr<ASolidStrongholdStone>>& Stones,
		bool& bStonesSpawned);
}
