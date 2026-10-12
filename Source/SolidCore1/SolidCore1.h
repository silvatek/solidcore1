#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"
#include "Journal/SolidJournalEntry.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSolid, Log, All);

/**
 * Live company state. The game starts in true sight with an empty party.
 * Copied with the travel session, so the journal survives the cave trip.
 */
struct FGameState
{
	int32 MaxPartySize = 0;
	const TCHAR* CaptainTitle = SolidEvents::TitleOutcast;
	bool bTrueSightEnabled = true;
	bool bRavenSightEnabled = false;
	/** Numbered cell the captain is standing on, or INDEX_NONE. */
	int32 OccupiedTown = INDEX_NONE;
	bool bFired[SolidEvents::MaxEvents] = {};
	/** Definition indexes, in the order the events fired. */
	int32 FiredOrder[SolidEvents::MaxEvents] = {};
	int32 FiredCount = 0;
	/** Journal, including repeated stronghold trips. */
	TArray<SolidJournal::FJournalEntry> Journal;
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
	};

	int32 PartyCount(const FGameState& State);
	bool IsSightEnabled(const FGameState& State, ESolidSight Sight);

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

	/**
	 * The captain is now on TownId (INDEX_NONE when the cell is not numbered).
	 * A change of town fires every unfired EnterTown event whose parameter matches.
	 */
	FResult NotifyOccupiedTown(
		FGameState& State,
		int32 TownId,
		const SolidWorldTime::FStamp& When = SolidWorldTime::FStamp());
}
