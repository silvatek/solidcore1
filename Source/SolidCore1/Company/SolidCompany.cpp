#include "Company/SolidCompany.h"
#include "Inventory/SolidItemType.h"
#include "SolidBodyMesh.h"
#include "SolidClipLocomotion.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	void AddDefaultBattlePlans(TArray<FSolidBattlePlan>& Plans)
	{
		Plans.Add({
			TEXT("Starter"),
			ESolidBattleFormation::Line,
			ESolidBattleSpacing::Standard});
		Plans.Add(FSolidBattlePlan::Defined(TEXT("Attack")));
		Plans.Add({
			TEXT("Defend"),
			ESolidBattleFormation::Line,
			ESolidBattleSpacing::Standard});
		Plans.Add({
			TEXT("Parade"),
			ESolidBattleFormation::Parade,
			ESolidBattleSpacing::Standard});
		Plans.Add({
			TEXT("Line"),
			ESolidBattleFormation::Line,
			ESolidBattleSpacing::Standard});
		Plans.Add({
			TEXT("Column"),
			ESolidBattleFormation::Column,
			ESolidBattleSpacing::Standard});
		Plans.Add({
			TEXT("Tight mob"),
			ESolidBattleFormation::Mob,
			ESolidBattleSpacing::Narrow});
		Plans.Add({
			TEXT("Loose mob"),
			ESolidBattleFormation::Mob,
			ESolidBattleSpacing::Wide});
	}
}

USolidCaptain* USolidCompany::GetCaptain()
{
	if (!Captain)
	{
		Captain = NewObject<USolidCaptain>(this, TEXT("Captain"));
	}
	return Captain;
}

void USolidCompany::InitializeDefaultBattlePlans()
{
	if (AllBattlePlans.Num() > 0)
	{
		return;
	}

	AddDefaultBattlePlans(AllBattlePlans);
}

bool USolidCompany::HasOnlyDefaultBattlePlans() const
{
	TArray<FSolidBattlePlan> Defaults;
	AddDefaultBattlePlans(Defaults);
	if (AllBattlePlans.Num() != Defaults.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < Defaults.Num(); ++Index)
	{
		if (!AllBattlePlans[Index].SameAs(Defaults[Index]))
		{
			return false;
		}
	}
	return true;
}

const FSolidBattlePlan* USolidCompany::GetBattlePlan(const int32 Index) const
{
	return AllBattlePlans.IsValidIndex(Index) ? &AllBattlePlans[Index] : nullptr;
}

int32 USolidCompany::FindBattlePlanIndexByName(const FString& Name) const
{
	for (int32 Index = 0; Index < AllBattlePlans.Num(); ++Index)
	{
		if (AllBattlePlans[Index].Is(Name))
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

int32 USolidCompany::AddCompanion(const FSolidCompanion& Companion)
{
	return AllCompanions.Add(Companion);
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
}

void USolidCompany::InitializeDefaultCompanions()
{
	if (AllCompanions.Num() > 0)
	{
		return;
	}

	InitializeDefaultCharacterMeshes();
	const int32 Viking = FindCharacterMeshIndexByName(TEXT("Viking"));

	auto Add = [this](const TCHAR* Name, const int32 MeshIndex, const ESolidVocation Vocation)
	{
		FSolidCompanion Companion;
		Companion.Name = Name;
		Companion.CharacterMeshIndex = MeshIndex;
		Companion.Vocation = Vocation;
		SolidCompanionColors::ColorsForName(
			Companion.Name, Companion.TorsoColor, Companion.LegColor, Companion.SkinColor);
		const FSolidBodyParams Seed = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(Companion.Name));
		const FSolidBodyControls Controls = SolidBodyControls::FromSeed(Seed);
		Companion.BodyHeight = Controls.Height;
		Companion.BodyBuild = Controls.Build;
		Companion.BodyGender = Controls.Gender;
		Companion.WeaponId = SolidItemType::IdCrudeSpear;
		Companion.ArmourId = SolidItemType::IdRags;
		AllCompanions.Add(Companion);
	};
	Add(TEXT("Sam"), Viking, ESolidVocation::Rogue);
	Add(TEXT("Alex"), Viking, ESolidVocation::Warrior);
	Add(TEXT("Max"), Viking, ESolidVocation::Cleric);
	Add(TEXT("Morgan"), Viking, ESolidVocation::Mage);
	Add(TEXT("Quinn"), Viking, ESolidVocation::Warrior);
}

void USolidCompany::InitializeStartingStronghold()
{
	if (ActiveStronghold)
	{
		return;
	}

	ActiveStronghold = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		SolidStronghold::StartingCell(),
		SolidStronghold::WorldXY(SolidStronghold::StartingCell()));
	if (ActiveStronghold)
	{
		ActiveStronghold->Orientation = ESolidStrongholdOrientation::North;
		ActiveStronghold->Name = SolidStronghold::StartingStrongholdName;
	}
}

void USolidCompany::SetStronghold(TUniquePtr<FSolidStronghold> Stronghold)
{
	ActiveStronghold = MoveTemp(Stronghold);
}

TUniquePtr<FSolidStronghold> USolidCompany::TakeStronghold()
{
	return MoveTemp(ActiveStronghold);
}

void SolidCompanionColors::PaletteForName(
	const FString& Name,
	FLinearColor& OutTorso,
	FLinearColor& OutLegs)
{
	const FString Lower = Name.ToLower();
	FRandomStream Stream(static_cast<int32>(FCrc::StrCrc32(*Lower) ^ 0xC0107u));
	const uint8 Hue = static_cast<uint8>(Stream.GetFraction() * 255.f);
	const uint8 Sat = static_cast<uint8>(FMath::Lerp(90.f, 190.f, Stream.GetFraction()));
	const uint8 Val = static_cast<uint8>(FMath::Lerp(80.f, 180.f, Stream.GetFraction()));
	OutTorso = FLinearColor::MakeFromHSV8(Hue, Sat, Val);

	const uint8 LegHue = static_cast<uint8>(
		(static_cast<int32>(Hue) + 48 + static_cast<int32>(Stream.GetFraction() * 70.f)) & 255);
	const uint8 LegSat = static_cast<uint8>(FMath::Lerp(15.f, 70.f, Stream.GetFraction()));
	const uint8 LegVal = static_cast<uint8>(FMath::Lerp(22.f, 65.f, Stream.GetFraction()));
	OutLegs = FLinearColor::MakeFromHSV8(LegHue, LegSat, LegVal);
}

void SolidCompanionColors::ColorsForName(
	const FString& Name,
	FLinearColor& OutTorso,
	FLinearColor& OutLegs,
	FLinearColor& OutSkin)
{
	PaletteForName(Name, OutTorso, OutLegs);
	OutSkin = SkinPink();
	if (Name.Equals(TEXT("Sam"), ESearchCase::IgnoreCase))
	{
		OutTorso = ShirtRed();
		OutLegs = TrouserBlue();
	}
	else if (Name.Equals(TEXT("Alex"), ESearchCase::IgnoreCase))
	{
		OutTorso = TrouserBlue();
		OutLegs = ShirtRed();
	}
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

bool USolidCompany::SetCompanionBodyControls(
	const int32 Index,
	const float Height,
	const float Build,
	const float Gender)
{
	if (!AllCompanions.IsValidIndex(Index))
	{
		return false;
	}
	FSolidCompanion& Companion = AllCompanions[Index];
	if (Companion.BodyHeight == Height && Companion.BodyBuild == Build && Companion.BodyGender == Gender)
	{
		return false;
	}
	Companion.BodyHeight = Height;
	Companion.BodyBuild = Build;
	Companion.BodyGender = Gender;
	return true;
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
