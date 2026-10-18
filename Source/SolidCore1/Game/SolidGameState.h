#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidStorage.h"
#include "Journal/SolidJournalEntry.h"
#include "SolidStronghold.h"
#include "Strongholds/SolidStrongholdFurniture.h"

/** A stronghold the company has left. The exterior stays. It is no longer a passage. */
struct FLeftStronghold
{
	FSolidStronghold Stronghold;
	FIntPoint Cell = FIntPoint::ZeroValue;
};

/**
 * Live company state. The game starts in true sight with an empty party.
 * The game mode holds the current copy. The travel session carries it across the cave trip.
 */
struct FGameState
{
	int32 MaxPartySize = 0;
	const TCHAR* CaptainTitle = SolidEvents::TitleOutcast;
	bool bTrueSightEnabled = true;
	bool bRavenSightEnabled = false;
	/** Enter Relion restores the Iglin span. It starts broken. */
	bool bIglinBridgeRepaired = false;
	/** Closing the monster message opens the starting cave. It starts closed. */
	bool bCaveEnabled = false;
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
	/** The cave chest. It starts with a rusty sword, old boots, coins, sticks, and pitch. Taking an item keeps it out. */
	SolidStorage::FContainer CaveChest = SolidStorage::StartingCaveChest();
	/** The company base. A new game is the Meagre Dungeon on its map cell. */
	FSolidStronghold ActiveStronghold = SolidStronghold::Starting;
	/** Map cell of that base. X is the column and Y is the row, counting from 0. */
	FIntPoint ActiveStrongholdCell = SolidStronghold::StartingCell();
	/**
	 * Sizes an event has unlocked. The index matches ESolidStrongholdSize.
	 * A new game has none. The starting dungeon is the company base, not an unlock.
	 */
	bool bStrongholdSizeUnlocked[SolidStronghold::SizeCount] = {};
	/** Strongholds the company has left, oldest first. */
	TArray<FLeftStronghold> LeftStrongholds;
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
		int32 MaxPartySize = 0;
		bool bSetTitle = false;
		const TCHAR* CaptainTitle = TitleOutcast;
		/** A Repair bridge change ran. */
		bool bRepairBridge = false;
		/** An Enable cave change ran. The captain is sent inside. */
		bool bEnterCave = false;
		/** A Show message change ran. MessageId names the line to show. */
		bool bShowMessage = false;
		const TCHAR* MessageId = nullptr;
	};

	int32 PartyCount(const FGameState& State);
	bool IsSightEnabled(const FGameState& State, ESolidSight Sight);

	/** True when an event has unlocked this stronghold size. */
	inline bool IsStrongholdSizeUnlocked(const FGameState& State, const ESolidStrongholdSize Size)
	{
		const int32 Index = static_cast<int32>(Size);
		return Index >= 0 && Index < SolidStronghold::SizeCount && State.bStrongholdSizeUnlocked[Index];
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
