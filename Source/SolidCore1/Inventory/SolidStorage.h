#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidItem.h"

/**
 * A storage container's slots. The cave chest is the first one.
 * A slot stores one item. A count above one is a stack. An empty item is an empty cell.
 */
namespace SolidStorage
{
	inline constexpr const TCHAR* ChestTitle = TEXT("Chest");
	/** Width, then height. Six cells. */
	inline constexpr int32 Columns = 2;
	inline constexpr int32 Rows = 3;
	inline constexpr int32 Count = Columns * Rows;
	/** Coins in the cave chest when a play session starts. */
	inline constexpr int32 StartingCoinCount = 20;
	/** Sticks in the cave chest when a play session starts. */
	inline constexpr int32 StartingStickCount = 5;
	/** Pitch in the cave chest when a play session starts. */
	inline constexpr int32 StartingPitchCount = 3;

	struct FContainer
	{
		FSolidItem Slots[Count];
	};

	/** Slots a placed piece owns. Empty when the kind holds nothing. A chest has Count cells. */
	using FSlots = TArray<FSolidItem, TInlineAllocator<Count>>;

	inline void WriteSlot(FContainer& Container, const int32 Index, const FSolidItem& Item)
	{
		if (Index < 0 || Index >= Count)
		{
			return;
		}
		Container.Slots[Index] = Item.IsEmpty() ? FSolidItem() : Item;
	}

	inline void WriteSlot(FContainer& Container, const int32 Index, const TCHAR* ItemId, const int32 StackCount)
	{
		WriteSlot(Container, Index, FSolidItem::FromId(ItemId, StackCount));
	}

	inline int32 Index(const int32 Column, const int32 Row)
	{
		if (Column < 0 || Column >= Columns || Row < 0 || Row >= Rows)
		{
			return INDEX_NONE;
		}
		return Row * Columns + Column;
	}

	/**
	 * What the cave chest holds when a play session starts.
	 * The rusty sword, a pair of old boots, a stack of coins, five sticks, and three pitch.
	 * The last cell is empty.
	 */
	inline FContainer StartingCaveChest()
	{
		FContainer Chest;
		WriteSlot(Chest, Index(0, 0), SolidItemType::IdRustySword, 1);
		WriteSlot(Chest, Index(1, 0), SolidItemType::IdOldBoots, 1);
		WriteSlot(Chest, Index(0, 1), SolidItemType::IdCoins, StartingCoinCount);
		WriteSlot(Chest, Index(1, 1), SolidItemType::IdStick, StartingStickCount);
		WriteSlot(Chest, Index(0, 2), SolidItemType::IdPitch, StartingPitchCount);
		return Chest;
	}

	/**
	 * The next or previous storage container.
	 * ContainerCount is how many the stronghold has. One container stays at zero. The ends wrap.
	 */
	inline int32 StepContainer(const int32 Index, const int32 Delta, const int32 ContainerCount)
	{
		if (ContainerCount <= 1)
		{
			return 0;
		}
		int32 Place = Index % ContainerCount;
		if (Place < 0)
		{
			Place += ContainerCount;
		}
		Place += Delta;
		Place %= ContainerCount;
		if (Place < 0)
		{
			Place += ContainerCount;
		}
		return Place;
	}

	/**
	 * The chest heading. Place is the 1-based position in the cycle.
	 * One container is Chest. More than one is Chest, a space, and that place.
	 */
	inline FString ContainerHeading(const int32 Place, const int32 ContainerCount)
	{
		if (ContainerCount <= 1)
		{
			return FString(ChestTitle);
		}
		return FString::Printf(TEXT("%s %d"), ChestTitle, Place);
	}
}
