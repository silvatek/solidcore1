#pragma once

#include "CoreMinimal.h"
#include "Company/SolidSquad.h"
#include "Monster/SolidMonsterGroup.h"

namespace SolidInventory
{
	struct FLoadout;
}

/**
 * One battle. It links one squad, the captain's loadout, and one monster group.
 * The squad and the group already hold their battle plans. This reads the active plan on each.
 * Power is weapon Power. Defence is armour Strength. A later gauge can show the totals.
 * Each squad member and each monster attacks on their own.
 */
class FSolidBattle
{
public:
	FSolidBattle() = default;
	FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup);

	/** The squad in this battle. Empty when that squad has gone. */
	TWeakObjectPtr<USolidSquad> Squad;

	/**
	 * The captain's worn boxes and pack. The play state owns the loadout.
	 * Null when this battle has not been given one.
	 */
	SolidInventory::FLoadout* Inventory = nullptr;

	/** The monster group in this battle. The encounter owns the group. */
	FSolidMonsterGroup* Group = nullptr;

	/** The squad's active plan, or null when this battle has no squad. */
	const FSolidBattlePlan* GetSquadPlan() const;

	/** The group's active plan, or null when this battle has no group. */
	const FSolidBattlePlan* GetGroupPlan() const;

	/** Power of the captain's main hand, plus each companion the squad has out. */
	int32 GetSquadPower() const;

	/** Armour Strength of the captain's body, plus each companion the squad has out. */
	float GetSquadDefence() const;

	/** Power of every monster in the group. */
	int32 GetGroupPower() const;

	/** Armour Strength of every monster in the group. */
	float GetGroupDefence() const;

	/**
	 * One exchange.
	 * Each squad member whose plan stores attack lands their own weapon's Power on one monster.
	 * Each monster whose plan stores attack lands their own weapon's Power on the captain's armour.
	 * A plan that stores retreat does not attack. An empty main hand does not attack.
	 */
	void Attack();
};
