#pragma once

#include "CoreMinimal.h"
#include "Character/SolidBodyParams.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidItemType.h"
#include "SolidBodyMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Puts Sam, Alex, Max, Morgan, and Quinn on the company when they are not already there.
 * InitializeDefaultCompanions still fills the company today, so this adds nothing.
 * A company that starts empty receives the same records that call builds now.
 */
namespace SolidCompanionTest
{
	inline void SeedCompanyCompanions(USolidCompany* Company)
	{
		if (!Company)
		{
			return;
		}

		Company->InitializeDefaultCompanions();
		if (Company->FindCompanionIndexByName(TEXT("Sam")) != INDEX_NONE)
		{
			return;
		}

		Company->InitializeDefaultCharacterMeshes();
		const int32 Viking = Company->FindCharacterMeshIndexByName(TEXT("Viking"));
		auto Add = [Company, Viking](const TCHAR* Name, const ESolidVocation Vocation)
		{
			FSolidCompanion Companion;
			Companion.Name = Name;
			Companion.CharacterMeshIndex = Viking;
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
			Company->AddCompanion(Companion);
		};
		Add(TEXT("Sam"), ESolidVocation::Rogue);
		Add(TEXT("Alex"), ESolidVocation::Warrior);
		Add(TEXT("Max"), ESolidVocation::Cleric);
		Add(TEXT("Morgan"), ESolidVocation::Mage);
		Add(TEXT("Quinn"), ESolidVocation::Warrior);
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
