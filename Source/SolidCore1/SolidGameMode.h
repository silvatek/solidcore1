#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidGameMode.generated.h"

class ASolidCompanionCharacter;
class ASolidMonolith;
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

	/** Spawn Party companions (Sam + Alex) once the player pawn exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCompanionCharacter> CompanionClass;

	/** Spawn a grey monolith at the start and a line of trees into the fog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	bool bAutoSpawnStarterTrees = true;

	/** World XY of the starter monolith (cm). Tree line continues from here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FVector2D StarterTreeOffsetXY = FVector2D(1400.f, 900.f);

	/** Horizontal direction of the tree line (normalized at spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FVector2D StarterTreeLineDirection = FVector2D(1.f, 0.35f);

	/** How many trees in the line (most should fall beyond the 25–50m fog bands). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "1", ClampMax = "64"))
	int32 StarterTreeCount = 16;

	/** Base spacing along the line (cm). Per-tree jitter is applied on top. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "200.0"))
	float StarterTreeSpacingCm = 1000.f;

	/** RNG seed for tree sizes / lateral jitter (0 = derive from world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	int32 StarterTreeSeed = 42;

	/** First spawned companion (Sam), if any. */
	ASolidCompanionCharacter* GetCompanion() const;

	const TArray<TObjectPtr<ASolidCompanionCharacter>>& GetCompanions() const { return SpawnedCompanions; }

	/** Expected starter Party size (Sam + Alex). */
	static constexpr int32 DefaultCompanionCount = 2;

protected:
	virtual void BeginPlay() override;

	void EnsureTerrainStreamer();
	void EnsureCompanion();
	void EnsureStarterTrees();

	ASolidCompanionCharacter* SpawnCompanion(
		UWorld* World,
		APawn* PlayerPawn,
		UClass* ClassToSpawn,
		const FString& DisplayName,
		float FollowDistance,
		float SideOffset,
		float CatchUpDistance);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidCompanionCharacter>> SpawnedCompanions;

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidMonolith> SpawnedMonolith;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTree>> SpawnedStarterTrees;

	FTimerHandle CompanionSpawnTimer;
	FTimerHandle StarterTreeSpawnTimer;
};
