#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidItemType.h"

/**
 * A storage container's slots. The cave chest is the first one.
 * A slot stores an item id. Null is empty.
 */
namespace SolidStorage
{
	inline constexpr const TCHAR* ChestTitle = TEXT("Chest");
	/** Width, then height. Four cells. */
	inline constexpr int32 Columns = 2;
	inline constexpr int32 Rows = 2;
	inline constexpr int32 Count = Columns * Rows;

	struct FContainer
	{
		const TCHAR* Slots[Count] = {};
	};

	inline int32 Index(const int32 Column, const int32 Row)
	{
		if (Column < 0 || Column >= Columns || Row < 0 || Row >= Rows)
		{
			return INDEX_NONE;
		}
		return Row * Columns + Column;
	}

	/** What the cave chest holds when a play session starts. The rusty sword is in the first cell. */
	inline FContainer StartingCaveChest()
	{
		FContainer Chest;
		const int32 First = Index(0, 0);
		if (First != INDEX_NONE)
		{
			Chest.Slots[First] = SolidItemType::IdRustySword;
		}
		return Chest;
	}
}
