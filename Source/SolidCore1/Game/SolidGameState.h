#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidStorage.h"
#include "Journal/SolidJournalEntry.h"
#include "SolidStronghold.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"

/**
 * Live company state. The game starts in true sight with an empty squad.
 * The game mode holds the current copy. The travel session carries it across the cave trip.
 * Copying clones the company stronghold and each previous one, so a tower stays a tower.
 */
struct FGameState
{
	FGameState();
	FGameState(const FGameState& Other);
	FGameState& operator=(const FGameState& Other);
	FGameState(FGameState&&) = default;
	FGameState& operator=(FGameState&&) = default;

	int32 MaxSquadSize = 0;
	const TCHAR* CaptainTitle = SolidEvents::TitleOutcast;
	bool bTrueSightEnabled = true;
	bool bRavenSightEnabled = false;
	/** Enter Relion restores the Iglin span. It starts broken. */
	bool bIglinBridgeRepaired = false;
	/** Strongholds start closed. A size unlock or EnableCave opens them. */
	bool bStrongholdsEnabled = false;
	/**
	 * Lairs an EnableLair change has opened, by id.
	 * A new game has none. The change carries the lair id. The event id does not.
	 */
	const TCHAR* EnabledLairs[SolidEvents::MaxEnabledLairs] = {};
	int32 EnabledLairCount = 0;
	/** Numbered cell the captain is standing on, or INDEX_NONE. */
	int32 OccupiedTown = INDEX_NONE;
	bool bFired[SolidEvents::MaxEvents] = {};
	/** Definition indexes, in the order the events fired. */
	int32 FiredOrder[SolidEvents::MaxEvents] = {};
	int32 FiredCount = 0;
	/** Journal, including repeated stronghold trips. */
	TArray<SolidJournal::FJournalEntry> Journal;
	/** Worn boxes and the captain's pack. Each box stores an item id. A new game starts in the starting kit. Killed by monsters clears it to Rags. */
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	/**
	 * Indexes in the forest pile scatter the captain has picked up.
	 * A new game has none. Closing the monster message does not put those piles back.
	 */
	TArray<int32> TakenForagePiles;
	/** The cave chest. It starts with a rusty sword, old boots, coins, sticks, and pitch. Taking an item keeps it out. */
	SolidStorage::FContainer CaveChest = SolidStorage::StartingCaveChest();
	/** The company base. A new game is the Meagre Dungeon on its map cell. */
	TUniquePtr<FSolidStronghold> ActiveStronghold;
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
		const TCHAR* CaptainTitle = TitleOutcast;
		/** A Repair bridge change ran. */
		bool bRepairBridge = false;
		/** An Enable cave change ran. The captain enters the stronghold. */
		bool bEnterStronghold = false;
		/** A Show message change ran. MessageId names the line to show. */
		bool bShowMessage = false;
		const TCHAR* MessageId = nullptr;
	};

	int32 SquadCount(const FGameState& State);
	bool IsSightEnabled(const FGameState& State, ESolidSight Sight);

	/** True when this size is within the highest size an event has unlocked. */
	inline bool IsStrongholdSizeUnlocked(const FGameState& State, const ESolidStrongholdSize Size)
	{
		return static_cast<int32>(Size) <= static_cast<int32>(State.MaxStrongholdSize);
	}

	/** True when an EnableLair change has recorded this lair id. */
	inline bool IsLairEnabled(const FGameState& State, const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return false;
		}
		const int32 Count = State.EnabledLairCount < MaxEnabledLairs ? State.EnabledLairCount : MaxEnabledLairs;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (State.EnabledLairs[Index] && FCString::Strcmp(State.EnabledLairs[Index], Id) == 0)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * Run one change. An event runs each of its changes this way.
	 * A size unlock is recorded on the company. No shipped event carries one yet.
	 */
	void ApplyChange(FGameState& State, const FChange& Change, FResult& Result);

	void Remember(
		FGameState& State,
		const SolidWorldTime::FStamp& When,
		SolidJournal::EJournalType Type,
		const FString& Text,
		int32 EventIndex = INDEX_NONE);

	/**
	 * Enter or exit. The line does not name the stronghold.
	 * An upgrade will be its own entry. The same trip can be noted again.
	 */
	void RememberStronghold(
		FGameState& State,
		const SolidWorldTime::FStamp& When,
		bool bEnter);

	/** Enter or exit the Iglin tunnels. The same trip can be noted again. */
	void RememberLair(
		FGameState& State,
		const SolidWorldTime::FStamp& When,
		bool bEnter);

	/**
	 * The captain is now on TownId (INDEX_NONE when the cell is not numbered).
	 * A change of town fires every unfired EnterTown event whose parameter matches.
	 */
	FResult NotifyOccupiedTown(
		FGameState& State,
		int32 TownId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * The captain is standing on a ramp of the Iglin bridge.
	 * Fires every unfired ReachBrokenBridge event whose requirements are met.
	 * An unmet requirement is not recorded. A later call retries.
	 */
	FResult NotifyReachBrokenBridge(
		FGameState& State,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * The captain closed the message named by MessageId.
	 * Fires every unfired MessageClosed event for that id whose requirements are met.
	 */
	FResult NotifyMessageClosed(
		FGameState& State,
		const TCHAR* MessageId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());

	/**
	 * Record one event as already fired, including its journal line and changes,
	 * even when its requirements have not fired. A second call does nothing.
	 */
	FResult MarkComplete(
		FGameState& State,
		const TCHAR* EventId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());
}
