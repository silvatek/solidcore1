#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Monster/SolidEncounters.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * One battle between a real squad and a real monster group.
 * The squad and the group keep their own plans. The battle only links them.
 * A new game fields no companion, and the captain wears the starting loadout.
 * An existing encounter is the definition of that name.
 */
struct FSolidBattleSimulation
{
	USolidCompany* Company = nullptr;
	USolidSquad* Squad = nullptr;
	/** The captain's worn boxes and pack. A new game uses the starting kit. */
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidEncounter Encounter;
	FSolidBattle Battle;

	FSolidBattleSimulation()
	{
		Company = NewObject<USolidCompany>();
		Squad = NewObject<USolidSquad>();
		Squad->InitializeFromCompany(Company);
		Squad->SetSelectedCompanions({});
		Battle = FSolidBattle(Squad, nullptr);
	}

	/** The encounter the game builds from that definition. The battle links its first group. */
	void UseEncounter(const SolidEncounter::FDefinition& Definition)
	{
		Encounter = FSolidEncounter(Definition);
		Battle.Group = Encounter.Groups.Num() > 0 ? &Encounter.Groups[0] : nullptr;
	}

	/** The shipped encounter of that name. False when the list has no such name. */
	bool UseEncounter(const TCHAR* Name)
	{
		const SolidEncounter::FDefinition* Definition = FindEncounter(Name);
		if (!Definition)
		{
			Encounter = FSolidEncounter();
			Battle.Group = nullptr;
			return false;
		}
		UseEncounter(*Definition);
		return true;
	}

	/** Name of the item in that worn box. Empty when the box is empty. */
	FString WornName(const SolidInventory::ESlot Slot) const
	{
		const int32 Index = SolidInventory::WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return FString();
		}
		const FSolidItemType* Type = SolidItemType::FindById(Inventory.Worn[Index].Id());
		return Type ? FString(Type->Name) : FString();
	}

	const SolidEncounter::FDefinition* FindEncounter(const TCHAR* Name) const
	{
		if (!Name || !Name[0])
		{
			return nullptr;
		}
		for (const SolidEncounter::FDefinition& Candidate : SolidEncounter::Definitions)
		{
			if (Candidate.Name && FCString::Strcmp(Candidate.Name, Name) == 0)
			{
				return &Candidate;
			}
		}
		return nullptr;
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
	Sim.UseEncounter(TEXT("Iglin Bridge"));
	if (!Sim.Battle.Group)
	{
		AddError(TEXT("Iglin Bridge gave the battle no monster group"));
		return false;
	}

	TestTrue(TEXT("the battle links the squad"), Sim.Battle.Squad.Get() == Sim.Squad);
	TestTrue(TEXT("the battle links the Iglin Bridge group"), Sim.Battle.Group == &Sim.Encounter.Groups[0]);
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
	TestNull(TEXT("no monster group yet"), Sim.Battle.Group);

	Sim.SquadPlan(TEXT("Defend"));
	TestEqual(TEXT("the battle reads Defend"), Sim.SquadPlanName(), FString(TEXT("Defend")));
	TestTrue(TEXT("Defend stores retreat"), Sim.SquadRetreats());

	SolidEncounter::FDefinition Bare{ TEXT("Bare"), SolidEncounter::ESite::IglinBridgeRamp };
	Bare = Bare.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 1);
	Sim.UseEncounter(Bare);
	if (!Sim.Battle.Group)
	{
		AddError(TEXT("the bare definition gave the battle no monster group"));
		return false;
	}
	TestTrue(TEXT("a group with no plan name has none"), Sim.GroupPlanName().IsEmpty());
	TestEqual(TEXT("that group has its one raptor"), Sim.Battle.Group->Num(), 1);

	Sim.UseEncounter(TEXT("Iglin Bridge"));
	if (!Sim.Battle.Group)
	{
		AddError(TEXT("Iglin Bridge gave the battle no monster group"));
		return false;
	}
	TestEqual(TEXT("Iglin Bridge reads Attack"), Sim.GroupPlanName(), FString(TEXT("Attack")));
	TestTrue(TEXT("Attack stores attack"), Sim.GroupAttacks());
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
	if (!TestNotNull(TEXT("the squad"), Squad))
	{
		return false;
	}
	USolidCaptain* Captain = Squad->GetCompany() ? Squad->GetCompany()->GetCaptain() : nullptr;
	if (!TestNotNull(TEXT("the captain is with the squad"), Captain))
	{
		return false;
	}
	TestEqual(TEXT("no companion is out"), Squad->GetSelectedCompanionCount(), 0);
	TestEqual(TEXT("the main hand is the crude spear"), Sim.WornName(SolidInventory::ESlot::MainHand), FString(TEXT("Crude spear")));
	TestEqual(TEXT("the body is Rags"), Sim.WornName(SolidInventory::ESlot::Body), FString(TEXT("Rags")));
	TestEqual(TEXT("the head is the dented helmet"), Sim.WornName(SolidInventory::ESlot::Head), FString(TEXT("Dented helmet")));
	TestNull(TEXT("no monster group yet"), Sim.Battle.Group);

	Sim.UseEncounter(TEXT("Iglin Bridge"));
	if (!Sim.Battle.Group || Sim.Encounter.Definition.GroupCount < 1)
	{
		AddError(TEXT("Iglin Bridge gave the battle no monster group"));
		return false;
	}

	const SolidEncounter::FGroup& Spec = Sim.Encounter.Definition.Groups[0];
	TestEqual(TEXT("the group has the definition's raptors"), Sim.Battle.Group->Num(), Spec.Count);
	for (int32 Index = 0; Index < Sim.Battle.Group->Num(); ++Index)
	{
		const FSolidMonster* Monster = Sim.Battle.Group->Get(Index);
		const FString Which = FString::Printf(TEXT("raptor %d"), Index + 1);
		if (!TestNotNull(*Which, Monster))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("%s is a raptor"), *Which), FString(Monster->Kind()), FString(TEXT("Raptor")));
		TestEqual(*FString::Printf(TEXT("%s is Small"), *Which), static_cast<int32>(Monster->Size), static_cast<int32>(ESolidMonsterSize::Small));
		TestEqual(*FString::Printf(TEXT("%s has claws"), *Which), FString(Monster->Weapon.Name), FString(TEXT("Claws")));
		TestEqual(*FString::Printf(TEXT("%s has hide"), *Which), FString(Monster->Armour.Name), FString(TEXT("Hide")));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
