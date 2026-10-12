#pragma once

#include "CoreMinimal.h"
#include "SolidWorldTime.h"

/**
 * One journal line. Event lines point at the event catalog.
 * Stronghold lines are the visit text, and the same trip can be written again.
 */
namespace SolidJournal
{
	enum class EJournalType : uint8
	{
		Stronghold,
		Event,
	};

	/** Oldest line drops off once the page is this full. */
	inline constexpr int32 MaxEntries = 64;

	struct FJournalEntry
	{
		EJournalType Type = EJournalType::Event;
		int32 Year = SolidWorldTime::StartYear;
		int32 Day = SolidWorldTime::StartDay;
		int32 MinuteOfSession = 0;
		/** Catalog index when Type is Event. */
		int32 EventIndex = INDEX_NONE;
		/** Visit text when Type is Stronghold. */
		FString Text;
	};
}
