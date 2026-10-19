#pragma once

#include "CoreMinimal.h"

class ASolidStrongholdExterior;
class ASolidStrongholdSite;
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
	 * Spawns each stronghold site that has no exterior.
	 * The site is drawn as a stone block.
	 */
	void SpawnStrongholdSites(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdSite>>& Sites,
		bool& bSitesSpawned);
}
