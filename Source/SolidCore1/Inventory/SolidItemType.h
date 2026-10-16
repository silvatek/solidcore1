#pragma once

#include "CoreMinimal.h"

/**
 * What a carried thing is.
 * Armor is worn. A weapon is held. A tool is used.
 * Food, a valuable, and a material wait in the pack.
 */
enum class ESolidItemKind : uint8
{
	Armor,
	Weapon,
	Tool,
	Food,
	Valuable,
	Material,
};

/**
 * One kind of thing the captain can carry.
 * Id is eight hex digits. Name is the text on the inventory page.
 */
struct FSolidItemType
{
	const TCHAR* Id = TEXT("");
	const TCHAR* Name = TEXT("");
	ESolidItemKind Type = ESolidItemKind::Armor;
};

/** The items a slot can store. Inventory keeps the id, and the page shows the name. */
namespace SolidItemType
{
	inline constexpr int32 MaxItems = 16;
	inline constexpr int32 IdLength = 8;

	/** Stable ids. Eight hex digits. A worn box or a pack cell stores one of these. */
	inline constexpr const TCHAR* IdRags = TEXT("c3a91e04");
	inline constexpr const TCHAR* IdDentedHelmet = TEXT("7e2b58d1");
	inline constexpr const TCHAR* IdCrudeSpear = TEXT("91f6c0ab");
	inline constexpr const TCHAR* IdTorch = TEXT("4d8e27c5");
	inline constexpr const TCHAR* IdRustySword = TEXT("b5e20c78");

	inline const FSolidItemType Catalog[] = {
		{ IdRags, TEXT("Rags"), ESolidItemKind::Armor },
		{ IdDentedHelmet, TEXT("Dented helmet"), ESolidItemKind::Armor },
		{ IdCrudeSpear, TEXT("Crude spear"), ESolidItemKind::Weapon },
		{ IdTorch, TEXT("Torch"), ESolidItemKind::Tool },
		{ IdRustySword, TEXT("Rusty sword"), ESolidItemKind::Weapon },
	};

	static_assert(UE_ARRAY_COUNT(Catalog) <= MaxItems, "Raise SolidItemType::MaxItems");

	inline constexpr int32 CatalogCount = UE_ARRAY_COUNT(Catalog);

	inline const FSolidItemType* FindById(const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return nullptr;
		}
		for (int32 Index = 0; Index < CatalogCount; ++Index)
		{
			if (FCString::Strcmp(Catalog[Index].Id, Id) == 0)
			{
				return &Catalog[Index];
			}
		}
		return nullptr;
	}

	/** Page text for an id. Empty when the slot is empty or the id is unknown. */
	inline const TCHAR* NameFor(const TCHAR* Id)
	{
		const FSolidItemType* Item = FindById(Id);
		return Item ? Item->Name : TEXT("");
	}
}
