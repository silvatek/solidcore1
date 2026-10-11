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

void USolidCompany::InitializeDefaultCharacterMeshes()
{
	if (AllCharacterMeshes.Num() > 0)
	{
		return;
	}

	AllCharacterMeshes.Add({
		TEXT("Viking"),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath)),
		TEXT("SK_Viking")});
	// Paths are filled after Launcher Add to Project. The token finds the mesh by name.
	AllCharacterMeshes.Add({
		TEXT("Assassin"),
		TSoftObjectPtr<USkeletalMesh>(),
		TEXT("Assassin")});
	AllCharacterMeshes.Add({
		TEXT("Blacksmith"),
		TSoftObjectPtr<USkeletalMesh>(),
		TEXT("Blacksmith")});
}

void USolidCompany::InitializeDefaultCompanions()
{
	if (AllCompanions.Num() > 0)
	{
		return;
	}

	InitializeDefaultCharacterMeshes();
	const int32 Assassin = FindCharacterMeshIndexByName(TEXT("Assassin"));
	const int32 Blacksmith = FindCharacterMeshIndexByName(TEXT("Blacksmith"));

	AllCompanions.Add({ TEXT("Sam"), Assassin, ESolidVocation::Mage });
	AllCompanions.Add({ TEXT("Alex"), Blacksmith, ESolidVocation::Warrior });
	AllCompanions.Add({ TEXT("Max"), Blacksmith, ESolidVocation::Cleric });
	AllCompanions.Add({ TEXT("Morgan"), Assassin, ESolidVocation::Rogue });
	AllCompanions.Add({ TEXT("Quinn"), Assassin, ESolidVocation::Warrior });
}

const FSolidCharacterMesh* USolidCompany::GetCharacterMesh(const int32 Index) const
{
	return AllCharacterMeshes.IsValidIndex(Index) ? &AllCharacterMeshes[Index] : nullptr;
}

int32 USolidCompany::FindCharacterMeshIndexByName(const FString& Name) const
{
	for (int32 Index = 0; Index < AllCharacterMeshes.Num(); ++Index)
	{
		if (AllCharacterMeshes[Index].Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 USolidCompany::AddCharacterMesh(const FSolidCharacterMesh& CharacterMesh)
{
	return AllCharacterMeshes.Add(CharacterMesh);
}

TSoftObjectPtr<USkeletalMesh> USolidCompany::GetMeshForCompanion(const FSolidCompanion& Companion) const
{
	if (const FSolidCharacterMesh* CharacterMesh = GetCharacterMesh(Companion.CharacterMeshIndex))
	{
		return CharacterMesh->Mesh;
	}
	return TSoftObjectPtr<USkeletalMesh>();
}

const FSolidCompanion* USolidCompany::GetCompanion(const int32 Index) const
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
