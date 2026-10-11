#include "Party/SolidCompany.h"
#include "SolidClipLocomotion.h"
#include "UObject/SoftObjectPath.h"

void USolidCompany::InitializeDefaultBattlePlans()
{
	if (AllBattlePlans.Num() > 0)
	{
		return;
	}

	AllBattlePlans.Add({
		TEXT("Line"),
		ESolidBattleFormation::Line,
		ESolidBattleSpacing::Standard});
	AllBattlePlans.Add({
		TEXT("Column"),
		ESolidBattleFormation::Column,
		ESolidBattleSpacing::Standard});
	AllBattlePlans.Add({
		TEXT("Tight mob"),
		ESolidBattleFormation::Mob,
		ESolidBattleSpacing::Narrow});
	AllBattlePlans.Add({
		TEXT("Loose mob"),
		ESolidBattleFormation::Mob,
		ESolidBattleSpacing::Wide});
	AllBattlePlans.Add({
		TEXT("Parade"),
		ESolidBattleFormation::Parade,
		ESolidBattleSpacing::Standard});
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

void USolidCompany::InitializeDefaultCompanions()
{
	if (AllCompanions.Num() > 0)
	{
		return;
	}

	const TSoftObjectPtr<USkeletalMesh> VikingMesh(
		FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));

	static const TCHAR* Names[] = {
		TEXT("Sam"),
		TEXT("Alex"),
		TEXT("Max"),
		TEXT("Morgan"),
		TEXT("Quinn"),
	};

	for (const TCHAR* Name : Names)
	{
		FSolidCompanyCompanion Companion;
		Companion.Name = Name;
		Companion.Mesh = VikingMesh;
		AllCompanions.Add(Companion);
	}
}

const FSolidCompanyCompanion* USolidCompany::GetCompanion(const int32 Index) const
{
	return AllCompanions.IsValidIndex(Index) ? &AllCompanions[Index] : nullptr;
}

int32 USolidCompany::FindCompanionIndexByName(const FString& Name) const
{
	for (int32 Index = 0; Index < AllCompanions.Num(); ++Index)
	{
		if (AllCompanions[Index].Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}
