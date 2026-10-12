#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"
#include "GameFramework/GameModeBase.h"
#include "Party/SolidCompany.h"
#include "SolidSight.h"
#include "SolidGameMode.generated.h"

class ASolidBuilding;
class ASolidDungeonEntrance;
class ASolidDungeonInterior;
class ASolidCompanionCharacter;
class ASolidTownSign;
class ASolidTree;
class USkeletalMesh;
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

	/** Spawn the party's selected companions, up to the event max party size (starts empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCompanionCharacter> CompanionClass;

	/** Spawn Forest trees, Town buildings, welcome signs, and cave entrances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	bool bAutoSpawnVegetation = true;

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

	/** Fraction of Town TerrainPoints considered as building candidate sites [0, 1]. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TownBuildingDensity = 0.35f;

	/** Hard cap on packed town buildings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0", ClampMax = "500"))
	int32 MaxTownBuildings = 48;

	/** Extra gap between building packing circles (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0"))
	float TownBuildingMinSeparationCm = 100.f;

	/** Keep the start-town centroid and each welcome sign clear of buildings (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation", meta = (ClampMin = "0.0"))
	float TownBuildingClearRadiusCm = 450.f;

	/** RNG seed for town building pack / sizes (0 = fixed fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation")
	int32 TownBuildingSeed = 77;

	/** First spawned companion (Sam), if any. */
	ASolidCompanionCharacter* GetCompanion() const;

	const TArray<TObjectPtr<ASolidCompanionCharacter>>& GetCompanions() const { return SpawnedCompanions; }

	USolidCompany* GetCompany() const { return Company; }
	USolidParty* GetParty() const { return Party; }

	/** Event roster size (Sam, then Alex). The live party starts empty. The company catalog is larger. */
	static constexpr int32 DefaultCompanionCount = 2;

	/** Companions an event currently allows in the party. Starts at 0. */
	int32 GetMaxPartySize() const { return Configuration.MaxPartySize; }

	/** Spawn or remove companions so the field matches the current selection. */
	void RefreshFieldedParty();

	/** Live configuration, including which events have fired. */
	const FSolidEventConfiguration& GetConfiguration() const { return Configuration; }

	/** True when F9 may switch to this sight. Raven sight starts locked. */
	bool IsSightEnabled(ESolidSight Sight) const;

	/** Select Party assigned battle-plan slot (0 = F1). Returns false if empty/out of range. */
	bool SelectBattlePlanSlot(int32 SlotIndex);

	/**
	 * Move the captain onto one cell of TownId and shift companions by the same planar delta.
	 * Arriving fires the enter-town event for that place.
	 */
	bool TeleportPartyToTown(int32 TownId);

	/** True after F10 Enter cave, until F10 Leave cave. */
	bool IsInDungeon() const;

	/** Unload the open world (companions included) and load the dungeon interior. */
	void TravelToDungeon();

	/**
	 * Load the open world again and stand the captain at the dungeon mouth.
	 * Companions spawn beside him. The party builder stays closed.
	 */
	void TravelToWorld();

	/**
	 * Create the company catalog and party on the first call.
	 * Later calls (event refresh, companion spawn) keep the active battle plan
	 * and the companions selected for the party.
	 */
	void EnsureCompanyAndParty();

protected:
	virtual void BeginPlay() override;
	void EnsureTerrainStreamer();
	void RefreshEvents();
	void EnsureCompanion();
	void EnsureVegetation();
	void SaveTravelSession(bool bInDungeon, bool bReturnToDungeonMouth);
	void RestoreTravelSession();
	void BeginDungeonInterior();
	/** Sight, title, and the dungeon spawn. Retries while the pawn is still absent. */
	void FinishTravelArrival();
	void ApplyCaptainFromSession();
	void PlaceCaptainInDungeon();
	/** The third-person playground blocks sit inside the room. Hide them for this visit. */
	void HideTemplateProps();
	SolidWorldTime::FStamp WorldStamp() const;

	ASolidCompanionCharacter* SpawnCompanion(
		UWorld* World,
		APawn* PlayerPawn,
		UClass* ClassToSpawn,
		const FString& DisplayName,
		int32 PartySlotIndex,
		const TSoftObjectPtr<USkeletalMesh>& Mesh,
		ESolidVocation Vocation);

	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	UPROPERTY(Transient)
	TObjectPtr<USolidParty> Party;

	/** Company/party configuration. Enter-town events mutate this. */
	FSolidEventConfiguration Configuration;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidCompanionCharacter>> SpawnedCompanions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTree>> SpawnedForestTrees;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidBuilding>> SpawnedTownBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTownSign>> SpawnedTownSigns;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidDungeonEntrance>> SpawnedDungeonEntrances;

	FTimerHandle EventRefreshTimer;
	FTimerHandle VegetationSpawnTimer;
	FTimerHandle CaptainArrivalTimer;

	/** Interior spawned for this dungeon visit. The world keeps the actor alive. */
	ASolidDungeonInterior* DungeonInterior = nullptr;
	bool bCaptainTravelApplied = false;
	bool bCaptainPlacedInDungeon = false;
};
