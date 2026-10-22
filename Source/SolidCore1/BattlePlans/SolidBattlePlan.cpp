#include "SolidBattlePlan.h"

bool FSolidBattlePlan::SameAs(const FSolidBattlePlan& Other) const
{
	return Name == Other.Name
		&& Formation == Other.Formation
		&& Spacing == Other.Spacing;
}

bool FSolidBattlePlan::Is(const FString& PlanName) const
{
	return Name == PlanName;
}

bool FSolidBattlePlan::IsParade() const
{
	return Formation == ESolidBattleFormation::Parade;
}

FSolidBattlePlan FSolidBattlePlan::Defined(const TCHAR* PlanName)
{
	FSolidBattlePlan Plan;
	if (PlanName && FCString::Strcmp(PlanName, TEXT("Attack")) == 0)
	{
		Plan.Name = TEXT("Attack");
		Plan.Formation = ESolidBattleFormation::Mob;
		Plan.Spacing = ESolidBattleSpacing::Standard;
	}
	return Plan;
}
