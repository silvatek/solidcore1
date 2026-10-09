#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidGameMode.generated.h"

class ASolidCompanionCharacter;
class ASolidMonolith;
class ASolidTree;
class USolidCompany;
class USolidParty;

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

	/** Spawn the starter monolith and scatter trees in Forest biomes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	bool bAutoSpawnVegetation = true;

	/**
	 * Fallback world XY for the starter monolith (cm) when WorldMap has no Z town.
	 * When WorldMap loads, the monolith is placed at the Z-cell centroid instead.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	FVector2D StarterMonolithOffsetXY = FVector2D(1400.f, 900.f);

	/** Fraction of Forest TerrainPoints that get a tree [0, 1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ForestTreeDensity = 0.15f;

	/** Hard cap on scattered forest trees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0", ClampMax = "2000"))
	int32 MaxForestTrees = 256;

	/** Planar jitter around each forest grid point (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0"))
	float ForestTreeJitterCm = 90.f;

	/** RNG seed for forest scatter / tree sizes (0 = fixed fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	int32 ForestTreeSeed = 42;

	/** First spawned companion (Sam), if any. */
	ASolidCompanionCharacter* GetCompanion() const;

	const TArray<TObjectPtr<ASolidCompanionCharacter>>& GetCompanions() const { return SpawnedCompanions; }

	USolidCompany* GetCompany() const { return Company; }
	USolidParty* GetParty() const { return Party; }

	/** Expected starter Party size (Sam + Alex). */
	static constexpr int32 DefaultCompanionCount = 2;

	/** Select Party assigned battle-plan slot (0 = F1). Returns false if empty/out of range. */
	bool SelectBattlePlanSlot(int32 SlotIndex);

protected:
	virtual void BeginPlay() override;

	void EnsureCompanyAndParty();
	void EnsureTerrainStreamer();
	void EnsureCompanion();
	void EnsureVegetation();

	ASolidCompanionCharacter* SpawnCompanion(
		UWorld* World,
		APawn* PlayerPawn,
		UClass* ClassToSpawn,
		const FString& DisplayName,
		int32 PartySlotIndex);

	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	UPROPERTY(Transient)
	TObjectPtr<USolidParty> Party;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidCompanionCharacter>> SpawnedCompanions;

	UPROPERTY(Transient)
	TWeakObjectPtr<ASolidMonolith> SpawnedMonolith;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTree>> SpawnedForestTrees;

	FTimerHandle CompanionSpawnTimer;
	FTimerHandle VegetationSpawnTimer;
};
