#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidItem.h"
#include "Inventory/SolidItemType.h"
#include "Monster/SolidRaptor.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * One battle between a real squad and a real monster group.
 * The squad and the group keep their own plans. The battle only links them.
 * The company has the five companions. The squad starts with none of them out.
 */
struct FSolidBattleSimulation
{
	USolidCompany* Company = nullptr;
	USolidSquad* Squad = nullptr;
	FSolidMonsterGroup Group;
	FSolidBattle Battle;

	FSolidBattleSimulation()
	{
		Company = NewObject<USolidCompany>();
		Squad = NewObject<USolidSquad>();
		Squad->InitializeFromCompany(Company);
		Squad->SetSelectedCompanions({});
		Battle = FSolidBattle(Squad, &Group);
	}

	/** Put that companion in the squad. */
	void InSquad(const TCHAR* Name)
	{
		if (!Company || !Squad || !Name)
		{
			return;
		}
		const int32 Index = Company->FindCompanionIndexByName(Name);
		if (Index == INDEX_NONE)
		{
			return;
		}
		TArray<int32> Selected = Squad->GetSelectedCompanionIndices();
		Selected.Add(Index);
		Squad->SetSelectedCompanions(Selected);
	}

	/** One Small raptor, with claws and hide, in the group. */
	void Raptor()
	{
		TUniquePtr<FSolidRaptor> Monster = MakeUnique<FSolidRaptor>();
		Monster->Size = ESolidMonsterSize::Small;
		Monster->Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
		Group.Members.Add(MoveTemp(Monster));
	}

	bool SquadHas(const TCHAR* Name) const
	{
		if (!Company || !Squad || !Name)
		{
			return false;
		}
		const int32 Index = Company->FindCompanionIndexByName(Name);
		return Index != INDEX_NONE && Squad->GetSelectedCompanionIndices().Contains(Index);
	}

	FString CompanionWeapon(const TCHAR* Name) const
	{
		const FSolidCompanion* Companion = FindCompanion(Name);
		if (!Companion)
		{
			return FString();
		}
		const FSolidItemType* Type = SolidItemType::FindById(*Companion->WeaponId);
		return Type ? FString(Type->Name) : FString();
	}

	FString CompanionArmour(const TCHAR* Name) const
	{
		const FSolidCompanion* Companion = FindCompanion(Name);
		if (!Companion)
		{
			return FString();
		}
		const FSolidItemType* Type = SolidItemType::FindById(*Companion->ArmourId);
		return Type ? FString(Type->Name) : FString();
	}

	const FSolidCompanion* FindCompanion(const TCHAR* Name) const
	{
		if (!Company || !Name)
		{
			return nullptr;
		}
		return Company->GetCompanion(Company->FindCompanionIndexByName(Name));
	}

	/** Select that assigned plan on the squad. */
	void SquadPlan(const TCHAR* Name)
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

	/** Give the group that company plan, and make it the active one. */
	void GroupPlan(const TCHAR* Name)
	{
		Group.Available.Reset();
		Group.ActivePlan = INDEX_NONE;
		if (!Company || !Name)
		{
			return;
		}
		const FSolidBattlePlan* Plan = Company->GetBattlePlan(Company->FindBattlePlanIndexByName(Name));
		if (!Plan)
		{
			return;
		}
		Group.Available.Add(*Plan);
		Group.ActivePlan = 0;
	}

	FString SquadPlanName() const
	{
		const FSolidBattlePlan* Plan = Battle.GetSquadPlan();
		return Plan ? Plan->Name : FString();
	}

	FString GroupPlanName() const
	{
		const FSolidBattlePlan* Plan = Battle.GetGroupPlan();
		return Plan ? Plan->Name : FString();
	}

	bool SquadAttacks() const
	{
		const FSolidBattlePlan* Plan = Battle.GetSquadPlan();
		return Plan && Plan->Attacks();
	}

	bool SquadRetreats() const
	{
		const FSolidBattlePlan* Plan = Battle.GetSquadPlan();
		return Plan && Plan->Retreats();
	}

	bool GroupAttacks() const
	{
		const FSolidBattlePlan* Plan = Battle.GetGroupPlan();
		return Plan && Plan->Attacks();
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleSquadAndGroupTest,
	"SolidCore1.Battle.SquadAndGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleSquadAndGroupTest::RunTest(const FString& Parameters)
{
	FSolidBattleSimulation Sim;
	Sim.SquadPlan(TEXT("Starter"));
	Sim.GroupPlan(TEXT("Attack"));

	TestTrue(TEXT("the battle links the squad"), Sim.Battle.Squad.Get() == Sim.Squad);
	TestTrue(TEXT("the battle links the group"), Sim.Battle.Group == &Sim.Group);
	TestEqual(TEXT("the squad plan is Starter"), Sim.SquadPlanName(), FString(TEXT("Starter")));
	TestEqual(TEXT("the group plan is Attack"), Sim.GroupPlanName(), FString(TEXT("Attack")));
	TestTrue(TEXT("Starter stores attack"), Sim.SquadAttacks());
	TestTrue(TEXT("Attack stores attack"), Sim.GroupAttacks());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleActivePlanTest,
	"SolidCore1.Battle.ActivePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleActivePlanTest::RunTest(const FString& Parameters)
{
	FSolidBattleSimulation Sim;

	TestEqual(TEXT("a new squad reads Starter"), Sim.SquadPlanName(), FString(TEXT("Starter")));
	TestTrue(TEXT("the group has no plan yet"), Sim.GroupPlanName().IsEmpty());

	Sim.SquadPlan(TEXT("Defend"));
	TestEqual(TEXT("the battle reads Defend"), Sim.SquadPlanName(), FString(TEXT("Defend")));
	TestTrue(TEXT("Defend stores retreat"), Sim.SquadRetreats());

	Sim.GroupPlan(TEXT("Starter"));
	TestEqual(TEXT("the battle reads the group's Starter"), Sim.GroupPlanName(), FString(TEXT("Starter")));
	TestTrue(TEXT("the group's Starter stores attack"), Sim.GroupAttacks());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleSidesTest,
	"SolidCore1.Battle.Sides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleSidesTest::RunTest(const FString& Parameters)
{
	FSolidBattleSimulation Sim;

	USolidSquad* Squad = Sim.Battle.Squad.Get();
	TestNotNull(TEXT("the squad"), Squad);
	if (!Squad)
	{
		return false;
	}
	USolidCaptain* Captain = Squad->GetCompany() ? Squad->GetCompany()->GetCaptain() : nullptr;
	TestNotNull(TEXT("the captain is with the squad"), Captain);
	TestEqual(TEXT("no companion is out yet"), Squad->GetSelectedCompanionCount(), 0);
	TestEqual(TEXT("the group is empty"), Sim.Battle.Group->Num(), 0);

	Sim.InSquad(TEXT("Sam"));
	Sim.Raptor();
	Sim.Raptor();
	Sim.Raptor();

	TestTrue(TEXT("Sam is in the squad"), Sim.SquadHas(TEXT("Sam")));
	TestFalse(TEXT("Alex is in the squad"), Sim.SquadHas(TEXT("Alex")));
	TestEqual(TEXT("Sam's weapon is the crude spear"), Sim.CompanionWeapon(TEXT("Sam")), FString(TEXT("Crude spear")));
	TestEqual(TEXT("Sam's armour is Rags"), Sim.CompanionArmour(TEXT("Sam")), FString(TEXT("Rags")));
	TestEqual(TEXT("one companion is out"), Squad->GetSelectedCompanionCount(), 1);

	TestEqual(TEXT("three raptors are in the group"), Sim.Battle.Group->Num(), 3);
	for (int32 Index = 0; Index < Sim.Battle.Group->Num(); ++Index)
	{
		const FSolidMonster* Monster = Sim.Battle.Group->Get(Index);
		const FString Which = FString::Printf(TEXT("raptor %d"), Index + 1);
		TestNotNull(*Which, Monster);
		if (!Monster)
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s is a raptor"), *Which), FString(Monster->Kind()), FString(TEXT("Raptor")));
		TestEqual(*FString::Printf(TEXT("%s is Small"), *Which), static_cast<int32>(Monster->Size), static_cast<int32>(ESolidMonsterSize::Small));
		TestEqual(*FString::Printf(TEXT("%s has claws"), *Which), FString(Monster->Weapon.Name), FString(TEXT("Claws")));
		TestEqual(*FString::Printf(TEXT("%s has hide"), *Which), FString(Monster->Armour.Name), FString(TEXT("Hide")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
