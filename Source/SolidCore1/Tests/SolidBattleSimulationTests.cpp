#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/SolidBattle.h"
#include "Company/SolidCompany.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * One battle between a real squad and a real monster group.
 * The squad and the group keep their own plans. The battle only links them.
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
		Battle = FSolidBattle(Squad, &Group);
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

#endif // WITH_DEV_AUTOMATION_TESTS
