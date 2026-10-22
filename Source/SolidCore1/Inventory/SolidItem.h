#pragma once

#include "CoreMinimal.h"
#include "Combat/SolidArmour.h"
#include "Combat/SolidWeapon.h"
#include "Inventory/SolidItemType.h"

/**
 * One item in a slot. A stack is this item when Count is greater than one.
 * A weapon's count stays one, and its Power is PowerFor of the type's quality.
 * A material has a count and no Power. An empty slot has no type and a count of zero.
 */
class FSolidItem
{
public:
	const FSolidItemType* Type = nullptr;
	int32 Count = 0;
	FSolidWeapon Weapon;
	FSolidArmour Armour;

	/** The middle of the grade. The starting kit uses this draw. */
	static constexpr double StartingDraw = 0.0;

	bool IsEmpty() const;
	bool IsWeapon() const;
	bool IsArmour() const;
	/** True when this item is a weapon. A stack has no Power, so it is not a weapon. */
	bool HasPower() const;
	/** Catalog id. Empty when the slot is empty. */
	const TCHAR* Id() const;
	/** This item's Power. Zero when it is not a weapon. */
	int32 Power() const;
	bool Is(const TCHAR* ItemId) const;

	/**
	 * One item of this type.
	 * A weapon's count is one and its Power is PowerFor of the type's quality for StandardNormal.
	 * Any other kind keeps Count, clamped to MaxStacked, and has no Power.
	 */
	static FSolidItem Make(const FSolidItemType& ItemType, int32 Count, double StandardNormal);

	/** An item of a catalog id, using StartingDraw. Empty when the id is unknown or the count is below one. */
	static FSolidItem FromId(const TCHAR* ItemId, int32 Count);
};
