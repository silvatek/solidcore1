#include "SolidItem.h"

namespace
{
	/** How many of this type share the slot. A weapon stays at one. */
	int32 CountFor(const FSolidItemType& ItemType, const int32 Count)
	{
		if (ItemType.Type == ESolidItemKind::Weapon)
		{
			return 1;
		}
		const int32 AtLeastOne = Count < 1 ? 1 : Count;
		if (ItemType.MaxStacked > 0 && AtLeastOne > ItemType.MaxStacked)
		{
			return ItemType.MaxStacked;
		}
		return AtLeastOne;
	}
}

bool FSolidItem::IsEmpty() const
{
	return Type == nullptr || Count <= 0;
}

bool FSolidItem::IsWeapon() const
{
	return Weapon.Name != nullptr && Weapon.Name[0] != 0;
}

bool FSolidItem::IsArmour() const
{
	return Armour.Name != nullptr && Armour.Name[0] != 0;
}

bool FSolidItem::HasPower() const
{
	return IsWeapon();
}

const TCHAR* FSolidItem::Id() const
{
	return Type ? Type->Id : TEXT("");
}

int32 FSolidItem::Power() const
{
	return IsWeapon() ? Weapon.Power : 0;
}

bool FSolidItem::Is(const TCHAR* ItemId) const
{
	return !IsEmpty() && ItemId && ItemId[0] && FCString::Strcmp(Id(), ItemId) == 0;
}

FSolidItem FSolidItem::Make(const FSolidItemType& ItemType, const int32 Count, const double StandardNormal)
{
	FSolidItem Item;
	Item.Type = &ItemType;
	Item.Count = CountFor(ItemType, Count);
	if (ItemType.Type == ESolidItemKind::Weapon)
	{
		Item.Weapon.Name = ItemType.Name;
		Item.Weapon.Speed = 1.f;
		Item.Weapon.Power = SolidItemType::PowerFor(ItemType.Quality, StandardNormal);
	}
	else if (ItemType.Type == ESolidItemKind::Armor)
	{
		Item.Armour.Name = ItemType.Name;
		Item.Armour.Strength = FCString::Strcmp(ItemType.Id, SolidItemType::IdRags) == 0
			? SolidItemType::RagsStrength
			: 1.f;
	}
	return Item;
}

FSolidItem FSolidItem::FromId(const TCHAR* ItemId, const int32 Count)
{
	const FSolidItemType* ItemType = SolidItemType::FindById(ItemId);
	if (!ItemType || Count <= 0)
	{
		return FSolidItem();
	}
	return Make(*ItemType, Count, StartingDraw);
}
