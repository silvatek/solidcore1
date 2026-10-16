#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "Journal/SolidJournal.h"

/**
 * Captain's inventory. Worn boxes follow a person. The pack is a grid beside them.
 * A box stores an item id from SolidItemType. The page shows that item's name.
 * / opens the page. F10, then Inventory, opens it too.
 */
namespace SolidInventory
{
	inline constexpr const TCHAR* Title = TEXT("Inventory");
	inline constexpr const TCHAR* PackTitle = TEXT("Pack");
	inline constexpr const TCHAR* CloseHint = TEXT("/ to close");
	inline constexpr const TCHAR* ToggleKeyLabel = TEXT("/");

	enum class ESlot : uint8
	{
		Head,
		Body,
		Boots,
		LeftHand,
		RightHand,
		Belt,
		Count
	};

	inline constexpr int32 WornCount = static_cast<int32>(ESlot::Count);
	/** Width, then height. Six cells. */
	inline constexpr int32 PackColumns = 2;
	inline constexpr int32 PackRows = 3;
	inline constexpr int32 PackCount = PackColumns * PackRows;

	inline const TCHAR* SlotName(const ESlot Slot)
	{
		switch (Slot)
		{
		case ESlot::Head: return TEXT("Head");
		case ESlot::Body: return TEXT("Body");
		case ESlot::Boots: return TEXT("Boots");
		case ESlot::LeftHand: return TEXT("Left hand");
		case ESlot::RightHand: return TEXT("Right hand");
		case ESlot::Belt: return TEXT("Belt");
		default: return TEXT("");
		}
	}

	/** True for /. Shift+/ is ?, so a shifted slash is not the toggle. */
	inline bool IsToggleKey(const FKey& Key, const bool bShiftDown)
	{
		return Key == EKeys::Slash && !bShiftDown;
	}

	struct FLoadout
	{
		/** Item ids. Null is an empty box. */
		const TCHAR* Worn[WornCount] = {};
		const TCHAR* Pack[PackCount] = {};
	};

	/** What the captain is wearing when a play session starts. Boots and the pack are empty. */
	inline FLoadout StartingLoadout()
	{
		FLoadout Loadout;
		Loadout.Worn[static_cast<int32>(ESlot::Head)] = SolidItemType::IdDentedHelmet;
		Loadout.Worn[static_cast<int32>(ESlot::Body)] = SolidItemType::IdRags;
		Loadout.Worn[static_cast<int32>(ESlot::RightHand)] = SolidItemType::IdCrudeSpear;
		return Loadout;
	}

	inline int32 WornIndex(const ESlot Slot)
	{
		const int32 Index = static_cast<int32>(Slot);
		if (Index < 0 || Index >= WornCount)
		{
			return INDEX_NONE;
		}
		return Index;
	}

	inline int32 PackIndex(const int32 Column, const int32 Row)
	{
		if (Column < 0 || Column >= PackColumns || Row < 0 || Row >= PackRows)
		{
			return INDEX_NONE;
		}
		return Row * PackColumns + Column;
	}

	/**
	 * Store an item id in a worn box.
	 * A null or empty id clears it. An id that is not in the catalog is refused.
	 * False when the slot is not a worn box, or the id is unknown.
	 */
	inline bool PlaceWorn(FLoadout& Loadout, const ESlot Slot, const TCHAR* ItemId)
	{
		const int32 Index = WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		if (!ItemId || !ItemId[0])
		{
			Loadout.Worn[Index] = nullptr;
			return true;
		}
		const FSolidItemType* Item = SolidItemType::FindById(ItemId);
		if (!Item)
		{
			return false;
		}
		Loadout.Worn[Index] = Item->Id;
		return true;
	}

	/**
	 * Store an item id in one pack cell.
	 * A null or empty id clears it. An id that is not in the catalog is refused.
	 * False when the cell is outside the grid, or the id is unknown.
	 */
	inline bool PlacePack(FLoadout& Loadout, const int32 Column, const int32 Row, const TCHAR* ItemId)
	{
		const int32 Index = PackIndex(Column, Row);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		if (!ItemId || !ItemId[0])
		{
			Loadout.Pack[Index] = nullptr;
			return true;
		}
		const FSolidItemType* Item = SolidItemType::FindById(ItemId);
		if (!Item)
		{
			return false;
		}
		Loadout.Pack[Index] = Item->Id;
		return true;
	}

	/** Empty every worn box and every pack cell, then put Rags on the body. */
	inline void Clear(FLoadout& Loadout)
	{
		for (int32 Index = 0; Index < WornCount; ++Index)
		{
			Loadout.Worn[Index] = nullptr;
		}
		for (int32 Index = 0; Index < PackCount; ++Index)
		{
			Loadout.Pack[Index] = nullptr;
		}
		const int32 Body = WornIndex(ESlot::Body);
		if (Body != INDEX_NONE)
		{
			Loadout.Worn[Body] = SolidItemType::IdRags;
		}
	}

	/** One box on the page, in content pixels. The origin is the top-left of the figure. */
	struct FBox
	{
		float X = 0.f;
		float Y = 0.f;
		float W = 0.f;
		float H = 0.f;
		ESlot Slot = ESlot::Head;
		int32 Column = 0;
		int32 Row = 0;
		bool bPack = false;
		/** A cell of the open storage container. The cave chest. */
		bool bChest = false;
		FString Label;
		FString Item;
	};

	struct FPage
	{
		float ContentW = 0.f;
		float ContentH = 0.f;
		/** Left edge of the pack grid. The heading sits on this edge, above the cells. */
		float PackX = 0.f;
		float PackLabelY = 0.f;
		float PackGridW = 0.f;
		/** The cave chest, to the right of the pack. Absent when Chest is null. */
		bool bShowChest = false;
		float ChestX = 0.f;
		float ChestLabelY = 0.f;
		float ChestGridW = 0.f;
		TArray<FBox> Boxes;
	};

	/** Where a pointer landed on the page, in content pixels. */
	struct FSlotHit
	{
		bool bHit = false;
		bool bChest = false;
		bool bPack = false;
		ESlot Slot = ESlot::Head;
		int32 Column = 0;
		int32 Row = 0;
		int32 Index = INDEX_NONE;
	};

	/** Top-left of the page content on the canvas. The board math matches the drawn inventory. */
	struct FView
	{
		float OriginX = 0.f;
		float OriginY = 0.f;
	};

	/**
	 * Head, body, and boots in a column, hands beside the body, belt at the waist, pack to the right.
	 * Chest, when set, adds that container to the right of the pack.
	 */
	void Layout(const FLoadout& Loadout, FPage& OutPage, const SolidStorage::FContainer* Chest = nullptr);

	inline FView ContentOrigin(const float CanvasW, const float CanvasH, const FPage& Page, const float LineHeight)
	{
		float BoardW = 0.f;
		float BoardH = 0.f;
		SolidJournal::BoardSize(
			CanvasW,
			CanvasH,
			Page.ContentW,
			Page.ContentH + LineHeight + SolidJournal::LineGap,
			BoardW,
			BoardH);
		const float BoxX = (CanvasW - BoardW) * 0.5f;
		const float BoxY = (CanvasH - BoardH) * 0.5f;
		FView View;
		View.OriginX = BoxX + (BoardW - Page.ContentW) * 0.5f;
		View.OriginY = BoxY + SolidJournal::BoardPad;
		return View;
	}

	/** The first box that contains the point. Coordinates are content pixels, origin at the page. */
	inline bool HitSlot(const FPage& Page, const float X, const float Y, FSlotHit& OutHit)
	{
		OutHit = FSlotHit();
		for (const FBox& Box : Page.Boxes)
		{
			if (X < Box.X || Y < Box.Y || X >= Box.X + Box.W || Y >= Box.Y + Box.H)
			{
				continue;
			}
			OutHit.bHit = true;
			OutHit.bChest = Box.bChest;
			OutHit.bPack = Box.bPack;
			OutHit.Slot = Box.Slot;
			OutHit.Column = Box.Column;
			OutHit.Row = Box.Row;
			if (Box.bChest)
			{
				OutHit.Index = SolidStorage::Index(Box.Column, Box.Row);
			}
			else if (Box.bPack)
			{
				OutHit.Index = PackIndex(Box.Column, Box.Row);
			}
			else
			{
				OutHit.Index = WornIndex(Box.Slot);
			}
			return true;
		}
		return false;
	}

	/**
	 * Move one chest cell into a worn box or a pack cell.
	 * An occupied destination swaps. A miss, an empty cell, or another chest cell does nothing.
	 */
	inline bool TakeFromChest(
		SolidStorage::FContainer& Chest,
		FLoadout& Loadout,
		const int32 ChestIndex,
		const bool bPack,
		const ESlot Slot,
		const int32 Column,
		const int32 Row)
	{
		if (ChestIndex < 0 || ChestIndex >= SolidStorage::Count)
		{
			return false;
		}
		const TCHAR* Moving = Chest.Slots[ChestIndex];
		if (!Moving || !Moving[0] || !SolidItemType::FindById(Moving))
		{
			return false;
		}
		if (bPack)
		{
			const int32 Index = PackIndex(Column, Row);
			if (Index == INDEX_NONE)
			{
				return false;
			}
			const TCHAR* Displaced = Loadout.Pack[Index];
			if (!PlacePack(Loadout, Column, Row, Moving))
			{
				return false;
			}
			const FSolidItemType* Swap = (Displaced && Displaced[0]) ? SolidItemType::FindById(Displaced) : nullptr;
			Chest.Slots[ChestIndex] = Swap ? Swap->Id : nullptr;
			return true;
		}

		const int32 Index = WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		const TCHAR* Displaced = Loadout.Worn[Index];
		if (!PlaceWorn(Loadout, Slot, Moving))
		{
			return false;
		}
		const FSolidItemType* Swap = (Displaced && Displaced[0]) ? SolidItemType::FindById(Displaced) : nullptr;
		Chest.Slots[ChestIndex] = Swap ? Swap->Id : nullptr;
		return true;
	}
}
