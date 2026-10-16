#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Crafting/SolidRecipe.h"
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
	inline constexpr const TCHAR* CraftButtonLabel = TEXT("Craft");
	inline constexpr const TCHAR* CloseHint = TEXT("/ to close");
	inline constexpr const TCHAR* ToggleKeyLabel = TEXT("/");

	enum class ESlot : uint8
	{
		Head,
		Body,
		Boots,
		OffHand,
		MainHand,
		Belt,
		Count
	};

	inline constexpr int32 WornCount = static_cast<int32>(ESlot::Count);
	/** Width, then height. Six cells. */
	inline constexpr int32 PackColumns = 2;
	inline constexpr int32 PackRows = 3;
	inline constexpr int32 PackCount = PackColumns * PackRows;
	/** One cell for each material a recipe can list. */
	inline constexpr int32 CraftCount = SolidCrafting::MaxInputs;

	inline const TCHAR* SlotName(const ESlot Slot)
	{
		switch (Slot)
		{
		case ESlot::Head: return TEXT("Head");
		case ESlot::Body: return TEXT("Body");
		case ESlot::Boots: return TEXT("Boots");
		case ESlot::OffHand: return TEXT("Off hand");
		case ESlot::MainHand: return TEXT("Main hand");
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
		int32 WornStack[WornCount] = {};
		const TCHAR* Pack[PackCount] = {};
		int32 PackStack[PackCount] = {};
		/** Crafting slots on the inventory page. Empty until an item is dragged in. */
		const TCHAR* Craft[CraftCount] = {};
		int32 CraftStack[CraftCount] = {};
	};

	inline void WriteWorn(FLoadout& Loadout, const int32 Index, const TCHAR* ItemId, const int32 Count)
	{
		if (Index < 0 || Index >= WornCount)
		{
			return;
		}
		if (!ItemId || !ItemId[0] || Count <= 0)
		{
			Loadout.Worn[Index] = nullptr;
			Loadout.WornStack[Index] = 0;
			return;
		}
		Loadout.Worn[Index] = ItemId;
		Loadout.WornStack[Index] = Count;
	}

	inline void WritePack(FLoadout& Loadout, const int32 Index, const TCHAR* ItemId, const int32 Count)
	{
		if (Index < 0 || Index >= PackCount)
		{
			return;
		}
		if (!ItemId || !ItemId[0] || Count <= 0)
		{
			Loadout.Pack[Index] = nullptr;
			Loadout.PackStack[Index] = 0;
			return;
		}
		Loadout.Pack[Index] = ItemId;
		Loadout.PackStack[Index] = Count;
	}

	inline void WriteCraft(FLoadout& Loadout, const int32 Index, const TCHAR* ItemId, const int32 Count)
	{
		if (Index < 0 || Index >= CraftCount)
		{
			return;
		}
		if (!ItemId || !ItemId[0] || Count <= 0)
		{
			Loadout.Craft[Index] = nullptr;
			Loadout.CraftStack[Index] = 0;
			return;
		}
		Loadout.Craft[Index] = ItemId;
		Loadout.CraftStack[Index] = Count;
	}

	/** How many of an item to store. At least one, and no more than that item allows. */
	inline int32 StoredCount(const FSolidItemType& Item, const int32 Count)
	{
		const int32 AtLeastOne = Count < 1 ? 1 : Count;
		if (Item.MaxStacked > 0 && AtLeastOne > Item.MaxStacked)
		{
			return Item.MaxStacked;
		}
		return AtLeastOne;
	}

	/** What the captain is wearing when a play session starts. Boots and the pack are empty. */
	inline FLoadout StartingLoadout()
	{
		FLoadout Loadout;
		WriteWorn(Loadout, static_cast<int32>(ESlot::Head), SolidItemType::IdDentedHelmet, 1);
		WriteWorn(Loadout, static_cast<int32>(ESlot::Body), SolidItemType::IdRags, 1);
		WriteWorn(Loadout, static_cast<int32>(ESlot::MainHand), SolidItemType::IdCrudeSpear, 1);
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
	 * Armor fits only the body. A helmet fits only the head, boots only the boots, and a belt only the belt.
	 * A weapon, a tool, food, and a material fit any worn box.
	 */
	inline bool WornSlotAccepts(const ESlot Slot, const ESolidItemKind Kind)
	{
		switch (Kind)
		{
		case ESolidItemKind::Armor:
			return Slot == ESlot::Body;
		case ESolidItemKind::Helmet:
			return Slot == ESlot::Head;
		case ESolidItemKind::Boots:
			return Slot == ESlot::Boots;
		case ESolidItemKind::Belt:
			return Slot == ESlot::Belt;
		default:
			return true;
		}
	}

	/**
	 * Store an item id in a worn box.
	 * A null or empty id clears it. An id that is not in the catalog is refused.
	 * False when the slot is not a worn box, the id is unknown, or the kind does not fit.
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
			WriteWorn(Loadout, Index, nullptr, 0);
			return true;
		}
		const FSolidItemType* Item = SolidItemType::FindById(ItemId);
		if (!Item || !WornSlotAccepts(Slot, Item->Type))
		{
			return false;
		}
		WriteWorn(Loadout, Index, Item->Id, 1);
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
			WritePack(Loadout, Index, nullptr, 0);
			return true;
		}
		const FSolidItemType* Item = SolidItemType::FindById(ItemId);
		if (!Item)
		{
			return false;
		}
		WritePack(Loadout, Index, Item->Id, 1);
		return true;
	}

	/** Empty every worn box and every pack cell, then put Rags on the body. */
	inline void Clear(FLoadout& Loadout)
	{
		for (int32 Index = 0; Index < WornCount; ++Index)
		{
			WriteWorn(Loadout, Index, nullptr, 0);
		}
		for (int32 Index = 0; Index < PackCount; ++Index)
		{
			WritePack(Loadout, Index, nullptr, 0);
		}
		for (int32 Index = 0; Index < CraftCount; ++Index)
		{
			WriteCraft(Loadout, Index, nullptr, 0);
		}
		const int32 Body = WornIndex(ESlot::Body);
		if (Body != INDEX_NONE)
		{
			WriteWorn(Loadout, Body, SolidItemType::IdRags, 1);
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
		/** A crafting slot. Any item can sit here. The Craft button is separate. */
		bool bCraft = false;
		/** Top of the slot name. Worn boxes only. The name sits above the box, like Pack and Chest. */
		float LabelY = 0.f;
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
		/** The Craft button, to the right of the crafting slots. It stays off until crafting is wired. */
		float CraftButtonX = 0.f;
		float CraftButtonY = 0.f;
		float CraftButtonW = 0.f;
		float CraftButtonH = 0.f;
		bool bCraftEnabled = false;
		TArray<FBox> Boxes;
	};

	/** Where a pointer landed on the page, in content pixels. */
	struct FSlotHit
	{
		bool bHit = false;
		bool bChest = false;
		bool bPack = false;
		bool bCraft = false;
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
	 * Head, body, and boots in a column, the off hand and the main hand beside the body, belt at the waist, pack to the right.
	 * Each worn name sits above its box. Chest, when set, adds that container to the right of the pack.
	 * Four crafting slots and a Craft button sit on the bottom right. The button stays disabled.
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
			OutHit.bCraft = Box.bCraft;
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
			else if (Box.bCraft)
			{
				OutHit.Index = Box.Column;
			}
			else
			{
				OutHit.Index = WornIndex(Box.Slot);
			}
			return true;
		}
		return false;
	}

	/** True when both hits are the same worn box, pack cell, or chest cell. */
	inline bool SamePlace(const FSlotHit& A, const FSlotHit& B)
	{
		if (!A.bHit || !B.bHit || A.bChest != B.bChest || A.bPack != B.bPack || A.bCraft != B.bCraft)
		{
			return false;
		}
		if (A.bChest || A.bCraft)
		{
			return A.Index == B.Index;
		}
		if (A.bPack)
		{
			return A.Column == B.Column && A.Row == B.Row;
		}
		return A.Slot == B.Slot;
	}

	/** The item in a worn box, a pack cell, a chest cell, or a crafting slot. Null when the slot is empty or missing. */
	inline const TCHAR* ItemAt(const FLoadout& Loadout, const SolidStorage::FContainer* Chest, const FSlotHit& Hit)
	{
		if (!Hit.bHit)
		{
			return nullptr;
		}
		if (Hit.bChest)
		{
			if (!Chest || Hit.Index < 0 || Hit.Index >= SolidStorage::Count)
			{
				return nullptr;
			}
			return Chest->Slots[Hit.Index];
		}
		if (Hit.bPack)
		{
			const int32 Index = PackIndex(Hit.Column, Hit.Row);
			if (Index == INDEX_NONE)
			{
				return nullptr;
			}
			return Loadout.Pack[Index];
		}
		if (Hit.bCraft)
		{
			if (Hit.Index < 0 || Hit.Index >= CraftCount)
			{
				return nullptr;
			}
			return Loadout.Craft[Hit.Index];
		}
		const int32 Index = WornIndex(Hit.Slot);
		if (Index == INDEX_NONE)
		{
			return nullptr;
		}
		return Loadout.Worn[Index];
	}

	/** How many share the slot. Zero when the slot is empty or missing. */
	inline int32 CountAt(const FLoadout& Loadout, const SolidStorage::FContainer* Chest, const FSlotHit& Hit)
	{
		if (!Hit.bHit)
		{
			return 0;
		}
		if (Hit.bChest)
		{
			if (!Chest || Hit.Index < 0 || Hit.Index >= SolidStorage::Count)
			{
				return 0;
			}
			return Chest->Counts[Hit.Index];
		}
		if (Hit.bPack)
		{
			const int32 Index = PackIndex(Hit.Column, Hit.Row);
			if (Index == INDEX_NONE)
			{
				return 0;
			}
			return Loadout.PackStack[Index];
		}
		if (Hit.bCraft)
		{
			if (Hit.Index < 0 || Hit.Index >= CraftCount)
			{
				return 0;
			}
			return Loadout.CraftStack[Hit.Index];
		}
		const int32 Index = WornIndex(Hit.Slot);
		if (Index == INDEX_NONE)
		{
			return 0;
		}
		return Loadout.WornStack[Index];
	}

	/** Put an item id into a slot. A null or empty id clears it. False when the slot or the id is refused. */
	inline bool PutAt(
		FLoadout& Loadout,
		SolidStorage::FContainer* Chest,
		const FSlotHit& Hit,
		const TCHAR* ItemId,
		const int32 Count = 1)
	{
		if (!Hit.bHit)
		{
			return false;
		}
		if (Hit.bChest)
		{
			if (!Chest || Hit.Index < 0 || Hit.Index >= SolidStorage::Count)
			{
				return false;
			}
			if (!ItemId || !ItemId[0])
			{
				SolidStorage::WriteSlot(*Chest, Hit.Index, nullptr, 0);
				return true;
			}
			const FSolidItemType* Item = SolidItemType::FindById(ItemId);
			if (!Item)
			{
				return false;
			}
			SolidStorage::WriteSlot(*Chest, Hit.Index, Item->Id, StoredCount(*Item, Count));
			return true;
		}
		if (Hit.bPack)
		{
			const int32 Index = PackIndex(Hit.Column, Hit.Row);
			if (Index == INDEX_NONE)
			{
				return false;
			}
			if (!ItemId || !ItemId[0])
			{
				WritePack(Loadout, Index, nullptr, 0);
				return true;
			}
			const FSolidItemType* Item = SolidItemType::FindById(ItemId);
			if (!Item)
			{
				return false;
			}
			WritePack(Loadout, Index, Item->Id, StoredCount(*Item, Count));
			return true;
		}
		if (Hit.bCraft)
		{
			if (Hit.Index < 0 || Hit.Index >= CraftCount)
			{
				return false;
			}
			if (!ItemId || !ItemId[0])
			{
				WriteCraft(Loadout, Hit.Index, nullptr, 0);
				return true;
			}
			const FSolidItemType* Item = SolidItemType::FindById(ItemId);
			if (!Item)
			{
				return false;
			}
			WriteCraft(Loadout, Hit.Index, Item->Id, StoredCount(*Item, Count));
			return true;
		}
		const int32 Index = WornIndex(Hit.Slot);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		if (!ItemId || !ItemId[0])
		{
			WriteWorn(Loadout, Index, nullptr, 0);
			return true;
		}
		const FSolidItemType* Item = SolidItemType::FindById(ItemId);
		if (!Item || !WornSlotAccepts(Hit.Slot, Item->Type))
		{
			return false;
		}
		WriteWorn(Loadout, Index, Item->Id, StoredCount(*Item, Count));
		return true;
	}

	/**
	 * Move an item from one slot to another.
	 * Worn boxes, pack cells, chest cells, and crafting slots all count. An occupied destination swaps when both items fit.
	 * The same slot, an empty source, a kind that does not fit, or a chest move without a chest does nothing.
	 */
	inline bool Move(
		SolidStorage::FContainer* Chest,
		FLoadout& Loadout,
		const FSlotHit& From,
		const FSlotHit& To)
	{
		if (!From.bHit || !To.bHit || SamePlace(From, To))
		{
			return false;
		}
		if ((From.bChest || To.bChest) && !Chest)
		{
			return false;
		}
		const TCHAR* Moving = ItemAt(Loadout, Chest, From);
		if (!Moving || !Moving[0] || !SolidItemType::FindById(Moving))
		{
			return false;
		}
		const int32 MovingCount = CountAt(Loadout, Chest, From);
		const TCHAR* Displaced = ItemAt(Loadout, Chest, To);
		const int32 DisplacedCount = CountAt(Loadout, Chest, To);
		if (!PutAt(Loadout, Chest, To, Moving, MovingCount))
		{
			return false;
		}
		if (!PutAt(Loadout, Chest, From, Displaced, DisplacedCount))
		{
			PutAt(Loadout, Chest, To, Displaced, DisplacedCount);
			return false;
		}
		return true;
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
		FSlotHit From;
		From.bHit = true;
		From.bChest = true;
		From.Index = ChestIndex;
		FSlotHit To;
		To.bHit = true;
		To.bPack = bPack;
		To.Slot = Slot;
		To.Column = Column;
		To.Row = Row;
		To.Index = bPack ? PackIndex(Column, Row) : WornIndex(Slot);
		return Move(&Chest, Loadout, From, To);
	}
}
