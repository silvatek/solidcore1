#pragma once

#include "CoreMinimal.h"
#include "Combat/SolidArmour.h"
#include "Combat/SolidWeapon.h"

/**
 * What a carried thing is.
 * Armor is worn on the body. A helmet, boots, and a belt each have their own box.
 * A weapon is held. A tool is used.
 * Food and a material can be carried. Any kind can sit in the pack or a chest.
 */
enum class ESolidItemKind : uint8
{
	Armor,
	Helmet,
	Boots,
	Belt,
	Weapon,
	Tool,
	Food,
	Material,
};

/** How well a carried thing was made. NA is not a grade. */
enum class ESolidItemQuality : uint8
{
	Inferior,
	Common,
	Superior,
	Exquisite,
	Legendary,
	NA,
};

/**
 * One kind of thing the captain can carry.
 * Id is eight hex digits. Name is the text on the inventory page.
 * Quality is the grade. MaxStacked is how many of that item share one slot.
 * A weapon points at its Speed and Power. Body armour points at its Strength.
 */
struct FSolidItemType
{
	const TCHAR* Id = TEXT("");
	const TCHAR* Name = TEXT("");
	ESolidItemKind Type = ESolidItemKind::Armor;
	ESolidItemQuality Quality = ESolidItemQuality::Inferior;
	int32 MaxStacked = 1;
	/** Set on a weapon. Null on anything else. */
	const FSolidWeapon* Weapon = nullptr;
	/** Set on body armour. A helmet, boots, and a belt leave this empty. */
	const FSolidArmour* Armour = nullptr;
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
	inline constexpr const TCHAR* IdOldBoots = TEXT("8e14a6c2");
	inline constexpr const TCHAR* IdCoins = TEXT("f39b50d7");
	inline constexpr const TCHAR* IdStick = TEXT("6c2a91e8");
	inline constexpr const TCHAR* IdPitch = TEXT("d4f07b3a");

	inline const FSolidItemType Catalog[] = {
		{ IdRags, TEXT("Rags"), ESolidItemKind::Armor, ESolidItemQuality::Inferior, 1, nullptr, &SolidArmour::Rags },
		{ IdDentedHelmet, TEXT("Dented helmet"), ESolidItemKind::Helmet, ESolidItemQuality::Inferior, 1 },
		{ IdCrudeSpear, TEXT("Crude spear"), ESolidItemKind::Weapon, ESolidItemQuality::Inferior, 1, &SolidWeapon::CrudeSpear, nullptr },
		{ IdTorch, TEXT("Torch"), ESolidItemKind::Tool, ESolidItemQuality::Common, 1 },
		{ IdRustySword, TEXT("Rusty sword"), ESolidItemKind::Weapon, ESolidItemQuality::Inferior, 1, &SolidWeapon::RustySword, nullptr },
		{ IdOldBoots, TEXT("Old boots"), ESolidItemKind::Boots, ESolidItemQuality::Inferior, 1 },
		{ IdCoins, TEXT("Coins"), ESolidItemKind::Material, ESolidItemQuality::NA, 999 },
		{ IdStick, TEXT("Stick"), ESolidItemKind::Material, ESolidItemQuality::Inferior, 10 },
		{ IdPitch, TEXT("Pitch"), ESolidItemKind::Material, ESolidItemQuality::Common, 10 },
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

	/** Page text for a stack. A count above one follows the name. */
	inline FString PageLabel(const TCHAR* Id, const int32 Count)
	{
		const TCHAR* Name = NameFor(Id);
		if (!Name || !Name[0])
		{
			return FString();
		}
		if (Count <= 1)
		{
			return FString(Name);
		}
		return FString::Printf(TEXT("%s %d"), Name, Count);
	}

	/** Inferior's hidden power at the middle of the roll. Each grade multiplies this by PowerMedianRatio. */
	inline constexpr double PowerMedian = 12.0;
	/** How many times higher the next grade's middle power is. */
	inline constexpr double PowerMedianRatio = 1.6;
	/** Log step of one standard deviation. The first four grades use this on both sides. */
	inline constexpr double PowerSpread = 0.28;
	/** Legendary's high side. The low side stays at PowerSpread, so Exquisite can still win. */
	inline constexpr double LegendaryUpside = 0.14;
	/** How wide the ease is, in standard deviations, from PowerSpread down to LegendaryUpside. */
	inline constexpr double PowerGlide = 0.4;

	/**
	 * Hidden power for one grade.
	 * StandardNormal is one draw from a standard normal. NA is not a grade and returns 0.
	 * A grade returns at least 1.
	 */
	int32 PowerFor(ESolidItemQuality Quality, double StandardNormal);
}
