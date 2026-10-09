#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidGameMode.generated.h"

class ASolidCompanionCharacter;

UCLASS()
class SOLIDCORE1_API ASolidGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASolidGameMode();

	/** When true, spawns a terrain streamer at BeginPlay if the level does not already have one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAutoSpawnTerrainStreamer = true;

	/** Spawn Quinn as a follower once the player pawn exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCompanionCharacter> CompanionClass;

	ASolidCompanionCharacter* GetCompanion() const { return SpawnedCompanion.Get(); }

protected:
	virtual void BeginPlay() override;

	void EnsureTerrainStreamer();
	void EnsureCompanion();

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidCompanionCharacter> SpawnedCompanion;

	FTimerHandle CompanionSpawnTimer;
};
