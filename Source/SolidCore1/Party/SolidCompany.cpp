#include "Party/SolidCompany.h"

void USolidCompany::InitializeDefaultBattlePlans()
{
	if (AllBattlePlans.Num() > 0)
	{
		return;
	}

	AllBattlePlans.Add({TEXT("Line"), ESolidBattleFormation::Line});
	AllBattlePlans.Add({TEXT("Column"), ESolidBattleFormation::Column});
	AllBattlePlans.Add({TEXT("Mob"), ESolidBattleFormation::Mob});
}

const FSolidBattlePlan* USolidCompany::GetBattlePlan(const int32 Index) const
{
	return AllBattlePlans.IsValidIndex(Index) ? &AllBattlePlans[Index] : nullptr;
}

int32 USolidCompany::FindBattlePlanIndexByName(const FString& Name) const
{
	for (int32 Index = 0; Index < AllBattlePlans.Num(); ++Index)
	{
		if (AllBattlePlans[Index].Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 USolidCompany::AddBattlePlan(const FSolidBattlePlan& Plan)
{
	return AllBattlePlans.Add(Plan);
}
