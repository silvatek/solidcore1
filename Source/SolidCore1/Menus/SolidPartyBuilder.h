#pragma once

#include "CoreMinimal.h"

class USolidParty;

/**
 * Party builder opened from the F10 menu.
 * Lists every companion in the company. The player toggles who is in the party,
 * up to the event max party size. The menu entry stays disabled below that floor.
 */
namespace SolidPartyBuilder
{
	/** The Build party entry stays disabled until the events allow this many. */
	inline constexpr int32 MinPartySizeToOpen = 2;

	inline bool CanOpen(const int32 MaxPartySize)
	{
		return MaxPartySize >= MinPartySizeToOpen;
	}

	struct FRow
	{
		int32 CompanyIndex = INDEX_NONE;
		FString Label;
		bool bSelected = false;
	};

	void CollectRows(const USolidParty* Party, TArray<FRow>& OutRows);

	/**
	 * Add or remove CompanyIndex.
	 * A companion already in the party is removed.
	 * A new one is refused once MaxPartySize are selected, or when the index is not in the company.
	 */
	bool Toggle(USolidParty* Party, int32 CompanyIndex, int32 MaxPartySize);

	/** A selected row can be cleared. An empty slot can be filled only while under the cap. */
	inline bool IsChoiceAvailable(const bool bSelected, const int32 SelectedCount, const int32 MaxPartySize)
	{
		return bSelected || SelectedCount < MaxPartySize;
	}
}
