#pragma once

#include "CoreMinimal.h"
#include "Company/SolidSquad.h"
#include "Inventory/SolidItem.h"
#include "Companion/SolidCompanionTestSeed.h"
#include "Events/SolidEncounters.h"
#include "Monster/SolidRaptor.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Shared fixtures for the battle code tests and the battle simulations.
 * Both build a squad and a monster group the same way, then the test creates the battle.
 */
namespace SolidBattleTestPrivate
{
	/** Select that assigned plan on the squad. */
	inline void SelectSquadPlan(USolidSquad* Squad, const TCHAR* Name)
	{
		if (!Squad || !Name)
		{
			return;
		}
		for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
		{
			const FSolidBattlePlan* Plan = Squad->GetAssignedBattlePlan(Slot);
			if (Plan && Plan->Is(Name))
			{
				Squad->SelectAssignedSlot(Slot);
				return;
			}
		}
	}

	/** A new squad with no companion out. PlanName selects that assigned plan. */
	inline USolidSquad* BuildSquad(const TCHAR* PlanName)
	{
		USolidCompany* Company = NewObject<USolidCompany>();
		USolidSquad* Squad = NewObject<USolidSquad>(Company);
		Squad->InitializeFromCompany(Company);
		SolidCompanionTest::SeedCompanyCompanions(Company);
		Squad->SetSelectedCompanions({});
		SelectSquadPlan(Squad, PlanName);
		return Squad;
	}

	/**
	 * One Small raptor in claws and hide.
	 * The plan name is a monster group battle plan. A missing name leaves none.
	 */
	inline FSolidMonsterGroup OneRaptor(const TCHAR* PlanName)
	{
		FSolidMonsterGroup Built;
		TUniquePtr<FSolidRaptor> Raptor = MakeUnique<FSolidRaptor>();
		Raptor->Size = ESolidMonsterSize::Small;
		Raptor->Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
		Built.Members.Add(MoveTemp(Raptor));
		Built.GivePlan(PlanName);
		return Built;
	}

	/** The shipped encounter of that name. No groups when the list has no such name. */
	inline FSolidEncounter BuildEncounter(const TCHAR* Name)
	{
		if (!Name || !Name[0])
		{
			return FSolidEncounter();
		}
		for (const SolidEncounter::FStaticEncounter& Candidate : SolidEncounter::StaticEncounters)
		{
			if (Candidate.Name && FCString::Strcmp(Candidate.Name, Name) == 0)
			{
				return FSolidEncounter(Candidate);
			}
		}
		return FSolidEncounter();
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
