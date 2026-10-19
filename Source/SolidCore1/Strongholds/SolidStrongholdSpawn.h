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
	 * Places each company stronghold's exterior, and a standing stone on a site that has none.
	 * The stronghold names the actor, the world position, and the facing.
	 */
	void SpawnOutposts(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors,
		TArray<TObjectPtr<ASolidStrongholdStone>>& Stones,
		bool& bStonesSpawned);
}
