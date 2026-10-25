#pragma once

#include "CoreMinimal.h"
#include "SolidGameState.h"
#include "SolidStronghold.h"
#include "GameFramework/GameModeBase.h"
#include "Terrain/SolidWorldChart.h"
#include "Vegetation/SolidForage.h"
#include "Events/SolidEncounter.h"
#include "Combat/SolidBattle.h"
#include "Company/SolidCompany.h"
#include "SolidSight.h"
#include "Strongholds/Furnishings/SolidArrange.h"
#include "SolidGameMode.generated.h"

class ASolidBridge;
class ASolidBuilding;
class ASolidForagePile;
class ASolidFurnishing;
class ASolidLairEntrance;
class ASolidLairInterior;
class ASolidCompanionCharacter;
class ASolidTownSign;
class ASolidNameSign;
class ASolidStrongholdExterior;
class ASolidStrongholdInterior;
class ASolidStrongholdSite;
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

	/**
	 * Cook reference for /Game/World/WorldChart.
	 * A path-string LoadObject is not one. LoadDefault reads this asset.
	 */
	UPROPERTY()
	TObjectPtr<USolidWorldChart> WorldChart;

	/** When true, spawns a terrain streamer at BeginPlay if the level does not already have one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAutoSpawnTerrainStreamer = true;

	/** Spawn the squad's selected companions, up to the event max squad size (starts empty). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	bool bAutoSpawnCompanion = true;

	/** Optional override; defaults to ASolidCompanionCharacter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion")
	TSubclassOf<ASolidCompanionCharacter> CompanionClass;

	/** Spawn Forest trees, Town buildings, welcome signs, the stronghold name sign, dungeon exteriors, tunnel mouths, and tower exteriors. */
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

	/** True once this visit has stood the captain inside the stronghold. */
	bool HasPlacedCaptainInStronghold() const { return bCaptainPlacedInStronghold; }

	/** True once this visit has stood the captain inside the lair. */
	bool HasPlacedCaptainInLair() const { return bCaptainPlacedInLair; }

	/** The game instance's company. Null when the instance is missing. */
	USolidCompany* GetCompany() const;
	USolidSquad* GetSquad() const { return Squad; }

	/**
	 * The fight underway. Null until a monster detects the captain.
	 * The board keeps it after the last blow.
	 */
	const FSolidBattle* GetBattle() const { return Battle.Get(); }

	/** Open-world encounters after Place. Empty before the monsters are chosen. */
	const TArray<FSolidEncounter>& GetEncounters() const { return Encounters; }

	/** Record the name so later Place skips it, and drop a live encounter of that name. */
	void DisableEncounter(const TCHAR* Name);

	/**
	 * Start a fight when a monster detects the captain, then move that fight forward.
	 * The log reads a copy of the kit. When that copy's captain dies, Killed by monsters fires.
	 * The event clears the play kit to Rags, opens the cave, and sends the captain inside.
	 * Once the captain's body is at 0, the log stays and the fight does not move.
	 */
	void AdvanceBattle(float DeltaSeconds, const FVector& CaptainLocation);

	/** The event roster is the whole company. The squad boxes start empty. */
	static constexpr int32 DefaultCompanionCount = 2;

	/** Companions an event currently allows in the squad. Starts at 0. */
	int32 GetMaxSquadSize() const;

	/** Spawn or remove companions so the field matches the current selection. */
	void RefreshFieldedSquad();

	/**
	 * The game instance's play state. Null when the instance is missing.
	 * Inventory, journal, chest, events, previous strongholds, and furnishings live here.
	 */
	USolidGameState* GetConfiguration() const;

	/**
	 * Drag between worn boxes, pack cells, and, when the chest is open, its cells.
	 * An occupied destination swaps. The same slot does nothing.
	 */
	bool MoveCarriedItem(
		const SolidInventory::FSlotHit& From,
		const SolidInventory::FSlotHit& To,
		bool bChestOpen);

	/** Press the Craft button. Spends the recipe and puts one result in the pack. */
	bool CraftFromInventory();

	/** True when F9 may switch to this sight. Raven sight starts locked. */
	bool IsSightEnabled(ESolidSight Sight) const;

	/**
	 * Select Squad assigned battle-plan slot (0 = F1). Returns false if empty/out of range.
	 * Parade fires Enter Iglin if it has not fired, then Enter Relion and Enter Dragongate.
	 * The captain does not move. The chosen plan stays active when it is still enabled.
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

	/** True while the company stronghold interior is loaded. */
	bool IsInStronghold() const;

	/** True after a click on an Iglin tunnel mouth, until a click leaves. */
	bool IsInLair() const;

	/** A stronghold interior or the Iglin tunnels. */
	bool IsInClosedVisit() const { return IsInStronghold() || IsInLair(); }

	/** Spawn chosen piles whose fog is now below full. Safe to call again. */
	void SeedForagePiles();

	/** Spawn each encounter member whose fog is now below full. Safe to call again. */
	void SeedMonsters();

	/**
	 * Pick up one seeded pile into the pack.
	 * False when the pack cannot hold it. The pile stays, and nothing else changes.
	 */
	bool TakeForagePile(ASolidForagePile* Pile);

	/** An EnableLair change opens the Iglin tunnel mouths. They start shut. */
	bool AreTunnelsOpen() const;

	/** The captain closed a shown message. Fires MessageClosed for that id. */
	void NoteMessageClosed(const TCHAR* MessageId);

	/** Unload the open world and load the Iglin tunnels at one end. X or Y. */
	void TravelToLair(TCHAR EndMarker);

	/** Load the open world again and stand the captain at that tunnel mouth. */
	void TravelFromLair(TCHAR EndMarker);

	/** Unload the open world and load the company stronghold. Companions stay outside. */
	void EnterStronghold();

	/**
	 * A click on a stronghold site chose this type.
	 * The company moves, then the interior opens before the new exterior is on screen.
	 */
	bool EnterChosenStronghold(FIntPoint Cell, ESolidStrongholdSize Size, ESolidStrongholdType Type);

	/**
	 * Load the open world again.
	 * The captain and party stand just outside the entrance, facing away along its orientation.
	 * Build squad opens on the grass when there is something to arrange.
	 */
	void ExitStronghold();

	/** Arrange mode is on inside the company stronghold. */
	bool IsArrangingFurniture() const { return ArrangeMode.IsOn(); }

	/**
	 * `\` or the Arrange furniture plate.
	 * bOverlay is a menu, the inventory, the journal, a message, or the squad builder.
	 * An overlay or the open world leaves the mode as it is.
	 * Turning the mode off puts a held piece back.
	 */
	void ToggleArrangeFurniture(bool bOverlay);

	/** Esc with nothing held, or the start of leaving the stronghold. A held piece goes back first. */
	void LeaveArrangeFurniture();

	/** Left click or Enter on the captain's floor. */
	void ConfirmArrangeFurniture();

	/** Q and wheel up are positive. E and wheel down are negative. */
	void TurnArrangeFurniture(int32 Steps);

	/** Esc while a piece is held. The mode stays on. False when nothing is held. */
	bool PutBackArrangeFurniture();

	/**
	 * The hex under the pointer, when the pointer is showing.
	 * bPointer is false while the right button looks, so nothing is highlighted.
	 */
	void UpdateArrangePointer(const FVector& Origin, const FVector& Direction, bool bPointer);

	/** Catalog name beside the pointer. Empty when the hex is bare. */
	const TCHAR* ArrangeHoverName() const;

	/**
	 * Bind the game instance's company and create the squad on the first call.
	 * Later calls (event refresh, companion spawn) keep the active battle plan
	 * and the companions selected for the squad. Does not create another company.
	 */
	void EnsureCompanyAndSquad();

protected:
	virtual void BeginPlay() override;
	void EnsureTerrainStreamer();
	void RefreshEvents();
	/** Record one enter-town event and apply its sight, title, and bridge repair. */
	void NoteTownEntry(int32 TownId);
	/** Record Reach broken bridge when the captain stands on an Iglin ramp. */
	void NoteBrokenBridgeRamp(const FVector& Location);
	/** Show a message, send the captain into the cave, or place encounters waiting on a fired event. */
	void PresentEventResult(const SolidEvents::FResult& Result);
	/**
	 * The fight's copy of the captain has died.
	 * Fires Killed by monsters once. That event does the rest.
	 */
	void WakeFromBattleDeath();
	/** Draw the center of every broken bridge. Enter Relion calls this. */
	void RepairBrokenBridges();
	/** Fire Enter Iglin if needed, then Enter Relion and Enter Dragongate, and field the squad. */
	void FireParadeEvents();
	void EnsureCompanion();
	/** Keys follow the enabled plans while fewer than eight are on. */
	void SyncBattlePlans();
	void EnsureVegetation();
	void ChooseForagePiles(const USolidTerrainMap* Map);
	void ChooseMonsters(const USolidTerrainMap* Map);
	/** Place catalog marks whose event has fired and that are not already in Encounters. */
	int32 PlaceEncounterMarks(const USolidTerrainMap* Map);
	/** After an event fires, place and seed the encounters that were waiting. */
	void PlaceFiredEncounters();
	void SaveTravelSession(
		bool bInStronghold,
		bool bInLair = false,
		bool bReturnToLairMouth = false,
		TCHAR LairEnd = 0,
		bool bExitStronghold = false);
	void RestoreTravelSession();
	void BeginStrongholdInterior();
	void BeginLairInterior();
	/** Sight, title, and the closed-visit spawn. Retries while the pawn is still absent. */
	void FinishTravelArrival();
	void ApplyCaptainFromSession();
	void PlaceCaptainInStronghold();
	void PlaceCaptainInLair();
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
	TObjectPtr<USolidSquad> Squad;

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

	/** Open-world encounters from the catalog, after Place. The same again on the next visit. */
	TArray<FSolidEncounter> Encounters;
	bool bMonstersChosen = false;

	/** The current fight. The group it points at lives in Encounters. */
	TUniquePtr<FSolidBattle> Battle;
	/** Copy of the kit the fight reads. The play kit stays as it is until the copy's captain dies. */
	SolidInventory::FLoadout BattleLoadout;
	/** The copy's death has already fired Killed by monsters. */
	bool bWokeFromBattleDeath = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidBuilding>> SpawnedTownBuildings;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidTownSign>> SpawnedTownSigns;

	/** The company stronghold's name. A stronghold the company has left has none. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidNameSign>> SpawnedNameSigns;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidLairEntrance>> SpawnedLairEntrances;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidStrongholdExterior>> SpawnedStrongholdExteriors;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidStrongholdSite>> SpawnedStrongholdSites;

	/** Stronghold sites that have no exterior of their own. Each is drawn as a stone. */
	bool bSpawnedStrongholdSites = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ASolidBridge>> SpawnedBridges;

	FTimerHandle EventRefreshTimer;
	FTimerHandle VegetationSpawnTimer;
	FTimerHandle CaptainArrivalTimer;

	/** Interior spawned for this stronghold visit. The world keeps the actor alive. */
	ASolidStrongholdInterior* StrongholdInterior = nullptr;

	/** The mode, the held piece, and the captain's floor. */
	FSolidArrange ArrangeMode;

	/** Pieces on the floor that was open when the mode turned on. The interior owns them. */
	TArray<TWeakObjectPtr<ASolidFurnishing>> ArrangeActors;
	/** The visit interior. The world owns it. */
	TWeakObjectPtr<ASolidStrongholdInterior> ArrangeRoom;
	/** The pointer is on the floor plane, off every furniture cell, while a piece is held. */
	bool bArrangeOffFloor = false;
	FVector ArrangeOffFloorLocal = FVector::ZeroVector;

	void ApplyArrangeBody();
	void BindArrangeFloor();
	void ApplyArrangeActors();
	/** Interior spawned for this tunnel visit. The world keeps the actor alive. */
	ASolidLairInterior* LairInterior = nullptr;
	bool bCaptainTravelApplied = false;
	bool bCaptainPlacedInStronghold = false;
	bool bCaptainPlacedInLair = false;

#if WITH_DEV_AUTOMATION_TESTS
	friend class FSolidBattleDeathWakesTheCaptainTest;
#endif
};
