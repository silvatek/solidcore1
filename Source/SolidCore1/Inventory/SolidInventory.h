#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/**
 * Captain's inventory. Worn boxes follow a person. The pack is a grid beside them.
 * # opens the page. On a UK keyboard that key is beside Enter (Unreal calls it Backslash).
 * On a US keyboard # is Shift+3.
 */
namespace SolidInventory
{
	inline constexpr const TCHAR* Title = TEXT("Inventory");
	inline constexpr const TCHAR* PackTitle = TEXT("Pack");
	inline constexpr const TCHAR* CloseHint = TEXT("# to close");
	inline constexpr const TCHAR* ToggleKeyLabel = TEXT("#");

	inline constexpr const TCHAR* DentedHelmet = TEXT("Dented helmet");
	inline constexpr const TCHAR* RagShirt = TEXT("Rag shirt");
	inline constexpr const TCHAR* RagTrews = TEXT("Rag trews");
	inline constexpr const TCHAR* CrudeSpear = TEXT("Crude spear");

	enum class ESlot : uint8
	{
		Head,
		Body,
		Legs,
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
		case ESlot::Legs: return TEXT("Legs");
		case ESlot::LeftHand: return TEXT("Left hand");
		case ESlot::RightHand: return TEXT("Right hand");
		case ESlot::Belt: return TEXT("Belt");
		default: return TEXT("");
		}
	}

	/**
	 * True for the # key. Shift+# is ~ on a UK keyboard, so a shifted Backslash is not the toggle.
	 * Shift+3 is # on a US keyboard.
	 */
	inline bool IsToggleKey(const FKey& Key, const bool bShiftDown)
	{
		if (Key == EKeys::Backslash)
		{
			return !bShiftDown;
		}
		return Key == EKeys::Three && bShiftDown;
	}

	struct FLoadout
	{
		FString Worn[WornCount];
		FString Pack[PackCount];
	};

	/** What the captain is wearing when a play session starts. The pack is empty. */
	inline FLoadout StartingLoadout()
	{
		FLoadout Loadout;
		Loadout.Worn[static_cast<int32>(ESlot::Head)] = DentedHelmet;
		Loadout.Worn[static_cast<int32>(ESlot::Body)] = RagShirt;
		Loadout.Worn[static_cast<int32>(ESlot::Legs)] = RagTrews;
		Loadout.Worn[static_cast<int32>(ESlot::RightHand)] = CrudeSpear;
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

	/** Put Name in a worn box. An empty name clears it. False when the slot is not a worn box. */
	inline bool PlaceWorn(FLoadout& Loadout, const ESlot Slot, const FString& Name)
	{
		const int32 Index = WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Loadout.Worn[Index] = Name;
		return true;
	}

	/** Put Name in one pack cell. An empty name clears it. False when the cell is outside the grid. */
	inline bool PlacePack(FLoadout& Loadout, const int32 Column, const int32 Row, const FString& Name)
	{
		const int32 Index = PackIndex(Column, Row);
		if (Index == INDEX_NONE)
		{
			return false;
		}
		Loadout.Pack[Index] = Name;
		return true;
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
		TArray<FBox> Boxes;
	};

	/** Head, body, and legs in a column, hands beside the body, belt at the waist, pack to the right. */
	void Layout(const FLoadout& Loadout, FPage& OutPage);
}
