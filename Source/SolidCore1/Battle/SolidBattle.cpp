#include "SolidBattle.h"

FSolidBattle::FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup)
	: Squad(InSquad)
	, Group(InGroup)
{
}

const FSolidBattlePlan* FSolidBattle::GetSquadPlan() const
{
	const USolidSquad* Side = Squad.Get();
	return Side ? Side->GetActiveBattlePlan() : nullptr;
}

const FSolidBattlePlan* FSolidBattle::GetGroupPlan() const
{
	return Group ? Group->GetActivePlan() : nullptr;
}
