#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidGameMode.generated.h"

class ASolidCompanionCharacter;
class ASolidTree;

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

	/** Spawn one placeholder procedural tree near the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	bool bAutoSpawnStarterTree = true;

	/** World XY offset from origin for the starter tree (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FVector2D StarterTreeOffsetXY = FVector2D(1400.f, 900.f);

	ASolidCompanionCharacter* GetCompanion() const { return SpawnedCompanion.Get(); }

protected:
	virtual void BeginPlay() override;

	void EnsureTerrainStreamer();
	void EnsureCompanion();
	void EnsureStarterTree();

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidCompanionCharacter> SpawnedCompanion;

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidTree> SpawnedStarterTree;

	FTimerHandle CompanionSpawnTimer;
	FTimerHandle StarterTreeSpawnTimer;
};
