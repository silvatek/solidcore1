#include "SolidBattlePlan.h"

bool FSolidBattlePlan::SameAs(const FSolidBattlePlan& Other) const
{
	return Name == Other.Name
		&& Formation == Other.Formation
		&& Spacing == Other.Spacing
		&& AttackOrRetreat == Other.AttackOrRetreat;
}

bool FSolidBattlePlan::Is(const FString& PlanName) const
{
	return Name == PlanName;
}

bool FSolidBattlePlan::IsParade() const
{
	return Formation == ESolidBattleFormation::Parade;
}

bool FSolidBattlePlan::Attacks() const
{
	return AttackOrRetreat == ESolidAggression::Attack;
}

bool FSolidBattlePlan::Holds() const
{
	return AttackOrRetreat == ESolidAggression::Hold;
}

bool FSolidBattlePlan::Retreats() const
{
	return AttackOrRetreat == ESolidAggression::Retreat;
}

FSolidBattlePlan FSolidBattlePlan::Defined(const TCHAR* PlanName)
{
	if (PlanName && FCString::Strcmp(PlanName, TEXT("Attack")) == 0)
	{
		return {
			TEXT("Attack"),
			ESolidBattleFormation::Mob,
			ESolidBattleSpacing::Standard,
			ESolidAggression::Attack};
	}
	return {};
}
