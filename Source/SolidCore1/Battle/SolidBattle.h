#pragma once

#include "CoreMinimal.h"
#include "Company/SolidSquad.h"
#include "Monster/SolidMonsterGroup.h"

/**
 * One battle. It links one squad and one monster group.
 * Those two already hold their battle plans. This reads the active plan on each.
 */
class FSolidBattle
{
public:
	FSolidBattle() = default;
	FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup);

	/** The squad in this battle. Empty when that squad has gone. */
	TWeakObjectPtr<USolidSquad> Squad;

	/** The monster group in this battle. The encounter owns the group. */
	FSolidMonsterGroup* Group = nullptr;

	/** The squad's active plan, or null when this battle has no squad. */
	const FSolidBattlePlan* GetSquadPlan() const;

	/** The group's active plan, or null when this battle has no group. */
	const FSolidBattlePlan* GetGroupPlan() const;
};
