#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidCore1GameMode.generated.h"

class ASolidCore1CompanionCharacter;

UCLASS()
class SOLIDCORE1_API ASolidCore1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASolidCore1GameMode();

	/** When true, spawns a terrain streamer at BeginPlay if the level does not already have one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAutoSpawnTerrainStreamer = true;

	/** Spawn Quinn as a follower once the player pawn exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCore1CompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCore1CompanionCharacter> CompanionClass;

	ASolidCore1CompanionCharacter* GetCompanion() const { return SpawnedCompanion.Get(); }

protected:
	virtual void BeginPlay() override;

	void EnsureTerrainStreamer();
	void EnsureCompanion();

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidCore1CompanionCharacter> SpawnedCompanion;

	FTimerHandle CompanionSpawnTimer;
};
