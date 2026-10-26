#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Events/SolidEvents.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidStorage.h"
#include "Journal/SolidJournal.h"
#include "SolidStronghold.h"
#include "Company/SolidCaptain.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidGameState.generated.h"

class USolidCompany;
class USolidSquad;

/**
 * Live play state. The game starts with an empty squad.
 * The game instance owns one for the play session. OpenLevel keeps this object.
 * The company owns the captain and the active stronghold. Previous strongholds stay on this object.
 * This object owns the journal.
 */
UCLASS()
class SOLIDCORE1_API USolidGameState : public UObject
{
	GENERATED_BODY()

public:
	USolidGameState();
	/**
	 * The play state's one journal. Created on first ask. OpenLevel keeps this object.
	 * Lines live on the journal. Callers ask here, then call the journal method.
	 */
	USolidJournal* GetJournal();

	int32 MaxSquadSize = 0;
	/** Enter Relion restores the Iglin span. It starts broken. */
	bool bIglinBridgeRepaired = false;
	/**
	 * The Auto playthrough row stays dim for the rest of the game.
	 * Choosing the row sets it. So does leaving Iglin, including a portal.
	 * Spawn arrives from none and leaves it clear.
	 */
	bool bAutoPlaythroughDim = false;
	/** Strongholds start closed. A size unlock or EnableCave opens them. */
	bool bStrongholdsEnabled = false;
	/**
	 * Lairs an EnableLair change has opened, by id.
	 * A new game has none. The change carries the lair id. The event id does not.
	 */
	TArray<const TCHAR*, TInlineAllocator<SolidEvents::MaxEnabledLairs>> EnabledLairs;
	/**
	 * Encounter names ChooseMonsters skips. A new game has none.
	 * The Auto playthrough can disable Iglin Bridge after the cave wake.
	 */
	TArray<const TCHAR*, TInlineAllocator<8>> DisabledEncounters;
	bool bFired[SolidEvents::MaxEvents] = {};
	/** Definition indexes, in the order the events fired. */
	TArray<int32, TInlineAllocator<SolidEvents::MaxEvents>> FiredOrder;
	/** Worn boxes and the captain's pack. Each box stores an item id. A new game starts in the starting kit. Killed by monsters clears it to Rags. */
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	/**
	 * Indexes in the forest pile scatter the captain has picked up.
	 * A new game has none. Closing the monster message does not put those piles back.
	 */
	TArray<int32> TakenForagePiles;
	/** The cave chest. It starts with a rusty sword, old boots, coins, sticks, and pitch. Taking an item keeps it out. */
	SolidStorage::FContainer CaveChest = SolidStorage::StartingCaveChest();
	/**
	 * Highest size an event has unlocked. A new game is Meagre, the company base.
	 * A larger size unlocks every size up to it. Sizes are not skipped.
	 */
	ESolidStrongholdSize MaxStrongholdSize = ESolidStrongholdSize::Meagre;
	/**
	 * When set, a site offers only a size larger than the company stronghold.
	 * Off for the moment, so a site can offer Meagre Dungeon.
	 */
	bool bStrongholdSiteRestrictions = false;
	/** Strongholds the company moved on from, oldest first. The exterior stays. It is no longer a passage. */
	TArray<TUniquePtr<FSolidStronghold>> PreviousStrongholds;
	/** Furnishings in the active stronghold. A new game carries the Meagre stronghold's pieces. */
	TArray<SolidStrongholdFurniture::FUsed> Furnishings = SolidStrongholdFurniture::StartingFurnishings();
	/**
	 * This session's furnishing catalogue. A new game copies FurnitureCatalogue.
	 * bEnabled on a row is the unlock for this session.
	 */
	TArray<SolidStrongholdFurniture::FItem> FurnishingCatalog;
	/**
	 * Catalog plans that are on, by name.
	 * A new game lists Starter once the catalog has been seeded. Every other plan is off.
	 */
	TArray<FString> EnabledBattlePlans;
	/** Set when the catalog has been seeded. A second seed leaves the flags alone. */
	bool bBattlePlansSeeded = false;

protected:
	UPROPERTY()
	TObjectPtr<USolidJournal> Journal;
};

namespace SolidEvents
{
	struct FResult
	{
		bool bFired = false;
		/** An Enable sight change ran. Sight is the mode to switch to. */
		bool bSetSight = false;
		ESolidSight Sight = ESolidSight::True;
		int32 MaxSquadSize = 0;
		bool bSetTitle = false;
		ESolidCaptainTitle CaptainTitle = ESolidCaptainTitle::Outcast;
		/** A Repair bridge change ran. */
		bool bRepairBridge = false;
		/** An Enable cave change ran. The captain enters the stronghold. */
		bool bEnterStronghold = false;
		/** A Show message change ran. MessageId names the line to show. */
		bool bShowMessage = false;
		const TCHAR* MessageId = nullptr;
	};

	int32 SquadCount(const USolidGameState& State);
	bool IsSightEnabled(const USolidCaptain& Captain, ESolidSight Sight);

	/** True when this size is within the highest size an event has unlocked. */
	inline bool IsStrongholdSizeUnlocked(const USolidGameState& State, const ESolidStrongholdSize Size)
	{
		return static_cast<int32>(Size) <= static_cast<int32>(State.MaxStrongholdSize);
	}

	/**
	 * A new game enables Starter and leaves every other catalog plan off.
	 * Does nothing once that has been recorded, and does nothing while the catalog is empty.
	 */
	void SeedBattlePlans(USolidGameState& State, const USolidCompany& Company);

	/** True when this catalog plan is on. A missing name is off. */
	bool IsBattlePlanEnabled(const USolidGameState& State, const TCHAR* Name);

	/**
	 * Put the enabled plans back on the keys when fewer than eight are on,
	 * then select the plan that was active when that plan is still on.
	 * Eight or more leaves the keys as they were.
	 */
	void RestoreAssignedBattlePlans(
		USolidSquad& Squad,
		USolidGameState& State,
		const TArray<int32>& AssignedIndices,
		int32 ActiveSlot,
		bool bActive,
		uint8 HeldFormation,
		uint8 HeldSpacing);

	/** True when an EnableLair change has recorded this lair id. */
	inline bool IsLairEnabled(const USolidGameState& State, const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return false;
		}
		for (const TCHAR* Enabled : State.EnabledLairs)
		{
			if (Enabled && FCString::Strcmp(Enabled, Id) == 0)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Run one change. An event runs each of its changes this way.
	 * A size unlock is recorded on the company. No shipped event carries one yet.
	 * Title, town, and sight flags write on the captain.
	 */
	void ApplyChange(
		USolidGameState& State,
		USolidCaptain& Captain,
		const FChange& Change,
		FResult& Result);

	/**
	 * The captain is now on TownId (INDEX_NONE when the cell is not numbered).
	 * A change of town fires every unfired EnterTown event whose parameter matches.
	 */
	FResult NotifyOccupiedTown(
		USolidGameState& State,
		USolidCaptain& Captain,
		int32 TownId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * The captain is standing on a ramp of the Iglin bridge.
	 * Fires every unfired ReachBrokenBridge event whose requirements are met.
	 * An unmet requirement is not recorded. A later call retries.
	 */
	FResult NotifyReachBrokenBridge(
		USolidGameState& State,
		USolidCaptain& Captain,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * The captain closed the message named by MessageId.
	 * Fires every unfired MessageClosed event for that id whose requirements are met.
	 */
	FResult NotifyMessageClosed(
		USolidGameState& State,
		USolidCaptain& Captain,
		const TCHAR* MessageId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * The captain was killed in a battle.
	 * Records Killed by monsters when that event has not already fired.
	 * A second call does nothing.
	 */
	FResult NotifyCaptainKilled(
		USolidGameState& State,
		USolidCaptain& Captain,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * Record one event as already fired, including its journal line and changes,
	 * even when its requirements have not fired. A second call does nothing.
	 */
	FResult MarkComplete(
		USolidGameState& State,
		USolidCaptain& Captain,
		const TCHAR* EventId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());
}
