#pragma once

#include "CoreMinimal.h"

class USolidSquad;

/**
 * Squad builder opened from the F10 menu.
 * Lists every companion in the company. The player toggles who is in the squad,
 * up to the event max squad size. The menu entry stays disabled below that floor.
 */
namespace SolidSquadBuilder
{
	/** The Build squad entry stays disabled until the events allow this many. */
	inline constexpr int32 MinSquadSizeToOpen = 2;

	inline bool CanOpen(const int32 MaxSquadSize)
	{
		return MaxSquadSize >= MinSquadSizeToOpen;
	}

	struct FRow
	{
		int32 CompanyIndex = INDEX_NONE;
		FString Label;
		bool bSelected = false;
	};

	void CollectRows(const USolidSquad* Squad, TArray<FRow>& OutRows);

	/**
	 * Add or remove CompanyIndex.
	 * A companion already in the squad is removed.
	 * A new one is refused once MaxSquadSize are selected, or when the index is not in the company.
	 */
	bool Toggle(USolidSquad* Squad, int32 CompanyIndex, int32 MaxSquadSize);

	/** A selected row can be cleared. An empty slot can be filled only while under the cap. */
	inline bool IsChoiceAvailable(const bool bSelected, const int32 SelectedCount, const int32 MaxSquadSize)
	{
		return bSelected || SelectedCount < MaxSquadSize;
	}
}
