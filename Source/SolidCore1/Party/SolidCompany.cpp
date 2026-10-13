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

	auto Add = [this](const TCHAR* Name, const int32 MeshIndex, const ESolidVocation Vocation)
	{
		FSolidCompanion Companion;
		Companion.Name = Name;
		Companion.CharacterMeshIndex = MeshIndex;
		Companion.Vocation = Vocation;
		SolidCompanionColors::ColorsForName(
			Companion.Name, Companion.TorsoColor, Companion.LegColor, Companion.SkinColor);
		AllCompanions.Add(Companion);
	};
	Add(TEXT("Sam"), Assassin, ESolidVocation::Rogue);
	Add(TEXT("Alex"), Blacksmith, ESolidVocation::Warrior);
	Add(TEXT("Max"), Blacksmith, ESolidVocation::Cleric);
	Add(TEXT("Morgan"), Assassin, ESolidVocation::Mage);
	Add(TEXT("Quinn"), Assassin, ESolidVocation::Warrior);
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
