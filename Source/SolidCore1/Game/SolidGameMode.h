#pragma once

#include "CoreMinimal.h"
#include "SolidGameState.h"
#include "SolidStronghold.h"
#include "GameFramework/GameModeBase.h"
#include "Vegetation/SolidForage.h"
#include "Squad/SolidCompany.h"
#include "SolidSight.h"
#include "SolidGameMode.generated.h"

class ASolidBridge;
class ASolidBuilding;
class ASolidForagePile;
class ASolidDungeonEntrance;
class ASolidDungeonInterior;
class ASolidLairInterior;
class ASolidCompanionCharacter;
class ASolidTownSign;
class ASolidTowerInterior;
class ASolidStrongholdStone;
class ASolidTowerShell;
class ASolidTree;
class USolidTerrainMap;
class USkeletalMesh;
class USolidCompany;
class USolidSquad;

UCLASS()
class SOLIDCORE1_API ASolidGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASolidGameMode();

	/** When true, spawns a terrain streamer at BeginPlay if the level does not already have one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAutoSpawnTerrainStreamer = true;

	/** Spawn the squad's selected companions, up to the event max squad size (starts empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCompanionCharacter> CompanionClass;

	/** Spawn Forest trees, Town buildings, welcome signs, cave entrances, and tower shells. */
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

	const TArray<TObjectPtr<ASolidBridge>>& GetBridges() const { return SpawnedBridges; }

	USolidCompany* GetCompany() const { return Company; }
	USolidSquad* GetSquad() const { return Squad; }

	/** Starting selection (Sam, then Alex). The event roster is the whole company. The live squad starts empty. */
	static constexpr int32 DefaultCompanionCount = 2;

	/** Companions an event currently allows in the squad. Starts at 0. */
	int32 GetMaxSquadSize() const { return Configuration.MaxSquadSize; }

	/** Spawn or remove companions so the field matches the current selection. */
	void RefreshFieldedSquad();

	/** Live configuration, including which events have fired. */
	const FGameState& GetConfiguration() const { return Configuration; }

	/**
	 * Move one cave-chest cell into a worn box or a pack cell.
	 * An occupied destination swaps. An empty cell or a bad index does nothing.
	 */
	bool TakeFromCaveChest(
		int32 ChestIndex,
		bool bPack,
		SolidInventory::ESlot Slot,
		int32 Column,
		int32 Row)
	{
		return SolidInventory::TakeFromChest(
			Configuration.CaveChest,
			Configuration.Inventory,
			ChestIndex,
			bPack,
			Slot,
			Column,
			Row);
	}

	/**
	 * Drag between worn boxes, pack cells, and, when the chest is open, its cells.
	 * An occupied destination swaps. The same slot does nothing.
	 */
	bool MoveCarriedItem(
		const SolidInventory::FSlotHit& From,
		const SolidInventory::FSlotHit& To,
		bool bChestOpen)
	{
		return SolidInventory::Move(
			bChestOpen ? &Configuration.CaveChest : nullptr,
			Configuration.Inventory,
			From,
			To);
	}

	/** Press the Craft button. Spends the recipe and puts one result in the pack. */
	bool CraftFromInventory()
	{
		return SolidInventory::Craft(Configuration.Inventory);
	}

	/** True when F9 may switch to this sight. Raven sight starts locked. */
	bool IsSightEnabled(ESolidSight Sight) const;

	/**
	 * Select Squad assigned battle-plan slot (0 = F1). Returns false if empty/out of range.
	 * Parade first fires Enter Relion and Enter Dragongate, so a fresh launch
	 * still has companions and raven sight. The captain does not move.
	 */
	bool SelectBattlePlanSlot(int32 SlotIndex);

	/**
	 * Move the captain onto one cell of TownId and shift companions by the same planar delta.
	 * Arriving fires the enter-town event for that place.
	 */
	bool TeleportSquadToTown(int32 TownId);

	/**
	 * Stand the squad on the grass just north of a tunnel mouth.
	 * Records Driven from the village, including when its requirements have not fired.
	 */
	bool TeleportSquadToTunnelMouth(TCHAR EndMarker);

	/** True after F10 Enter cave, until F10 Leave cave. */
	bool IsInDungeon() const;

	/** True after a click on an Iglin tunnel mouth, until a click leaves. */
	bool IsInLair() const;

	/** True after a click on a tower door, until a click leaves. */
	bool IsInTower() const;

	/** Spawn chosen piles whose fog is now below full. Safe to call again. */
	void SeedForagePiles();

	/**
	 * Pick up one seeded pile into the pack.
	 * False when the pack cannot hold it. The pile stays, and nothing else changes.
	 */
	bool TakeForagePile(ASolidForagePile* Pile);

	/** Closing the monster message opens the starting cave. It starts closed. */
	bool IsCaveEnabled() const { return Configuration.bCaveEnabled; }

	/** Driven from the village opens the Iglin tunnel mouths. They start shut. */
	bool AreTunnelsOpen() const;

	/** The captain closed a shown message. Fires MessageClosed for that id. */
	void NoteMessageClosed(const TCHAR* MessageId);

	/** Unload the open world (companions included) and load the dungeon interior. */
	void TravelToDungeon();

	/**
	 * Load the open world again and stand the captain at the dungeon mouth.
	 * Companions spawn beside him. The squad builder stays closed.
	 */
	void TravelToWorld();

	/** Unload the open world and load the Iglin tunnels at one end. X or Y. */
	void TravelToLair(TCHAR EndMarker);

	/** Load the open world again and stand the captain at that tunnel mouth. */
	void TravelFromLair(TCHAR EndMarker);

	/** Unload the open world and load the tower at this map cell. Companions stay outside. */
	void TravelToTower(FIntPoint Cell);

	/**
	 * A click on a site stone chose this type.
	 * The company moves, then the interior opens before the new exterior is on screen.
	 */
	bool EnterChosenStronghold(FIntPoint Cell, ESolidStrongholdSize Size, ESolidStrongholdType Type);

	/** Load the open world again and stand the captain on the grass outside that door. */
	void TravelFromTower();

	/**
	 * Create the company catalog and squad on the first call.
	 * Later calls (event refresh, companion spawn) keep the active battle plan
	 * and the companions selected for the squad.
	 */
	void EnsureCompanyAndSquad();

protected:
	virtual void BeginPlay() override;
	void EnsureTerrainStreamer();
	void RefreshEvents();
	/** Record one enter-town event and apply its sight, title, and bridge repair. */
	void NoteTownEntry(int32 TownId);
	/** Record Reach broken bridge when the captain stands on an Iglin ramp, and show its message. */
	void NoteBrokenBridgeRamp(const FVector& Location);
	/** Show a message, or send the captain into the cave, when a result asks for it. */
	void PresentEventResult(const SolidEvents::FResult& Result);
	/** Draw the center of every broken bridge. Enter Relion calls this. */
	void RepairBrokenBridges();
	/** Fire Enter Iglin if needed, then Enter Relion and Enter Dragongate, and field the squad. */
	void FireParadeEvents();
	void EnsureCompanion();
	void EnsureVegetation();
	void ChooseForagePiles(const USolidTerrainMap* Map);
	void SaveTravelSession(
		bool bInDungeon,
		bool bReturnToDungeonMouth,
		bool bInLair = false,
		bool bReturnToLairMouth = false,
		TCHAR LairEnd = 0,
		bool bInTower = false,
		bool bReturnToTowerDoor = false,
		FIntPoint TowerCell = FIntPoint::ZeroValue,
		FSolidStronghold Tower = FSolidStronghold());
	void RestoreTravelSession();
	void BeginDungeonInterior();
	void BeginLairInterior();
	void BeginTowerInterior();
	/** Sight, title, and the closed-visit spawn. Retries while the pawn is still absent. */
	void FinishTravelArrival();
	void ApplyCaptainFromSession();
	void PlaceCaptainInDungeon();
	void PlaceCaptainInLair();
	void PlaceCaptainInTower();
	/** The third-person playground blocks sit inside the room. Hide them for this visit. */
	void HideTemplateProps();
	SolidWorldTime::FStamp WorldStamp() const;

	ASolidCompanionCharacter* SpawnCompanion(
		UWorld* World,
		APawn* PlayerPawn,
		UClass* ClassToSpawn,
		const FString& DisplayName,
		int32 SquadSlotIndex,
		const TSoftObjectPtr<USkeletalMesh>& Mesh,
		ESolidVocation Vocation);

	UPROPERTY(Transient)
	TObjectPtr<USolidCompany> Company;

	UPROPERTY(Transient)
	TObjectPtr<USolidSquad> Squad;

	/** Company/squad configuration. Enter-town events mutate this. */
	FGameState Configuration;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidCompanionCharacter>> SpawnedCompanions;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTree>> SpawnedForestTrees;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidForagePile>> SpawnedForagePiles;

	/** Scatter indexes already given a pile actor this visit. */
	TArray<int32> SeededForage;

	/** Choices for this open-world visit. The same map and seed rebuild the same list. */
	TArray<SolidForage::FChoice> ForageChoices;
	bool bForageChosen = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidBuilding>> SpawnedTownBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTownSign>> SpawnedTownSigns;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidDungeonEntrance>> SpawnedDungeonEntrances;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTowerShell>> SpawnedTowerShells;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidStrongholdStone>> SpawnedStrongholdStones;

	/** Site stones and site shells are placed together, including when every site is a shell. */
	bool bSpawnedStrongholdSites = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidBridge>> SpawnedBridges;

	FTimerHandle EventRefreshTimer;
	FTimerHandle VegetationSpawnTimer;
	FTimerHandle CaptainArrivalTimer;

	/** Interior spawned for this dungeon visit. The world keeps the actor alive. */
	ASolidDungeonInterior* DungeonInterior = nullptr;
	/** Interior spawned for this tunnel visit. The world keeps the actor alive. */
	ASolidLairInterior* LairInterior = nullptr;
	/** Interior spawned for this tower visit. The world keeps the actor alive. */
	ASolidTowerInterior* TowerInterior = nullptr;
	bool bCaptainTravelApplied = false;
	bool bCaptainPlacedInDungeon = false;
	bool bCaptainPlacedInLair = false;
	bool bCaptainPlacedInTower = false;
};
