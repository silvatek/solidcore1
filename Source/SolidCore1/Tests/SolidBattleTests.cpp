#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"
#include "Inventory/SolidItemType.h"
#include "Monster/SolidEncounters.h"
#include "Monster/SolidRaptor.h"
#include "SolidBattleTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidBattleTestPrivate
{
	/** Name of the item in that worn box. Empty when the box is empty. */
	FString WornName(const SolidInventory::FLoadout& Inventory, const SolidInventory::ESlot Slot)
	{
		const int32 Index = SolidInventory::WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return FString();
		}
		const FSolidItemType* Type = SolidItemType::FindById(Inventory.Worn[Index].Id());
		return Type ? FString(Type->Name) : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleSquadAndGroupTest,
	"SolidCore1.Battle.SquadAndGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleSquadAndGroupTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
	FSolidEncounter Encounter = SolidBattleTestPrivate::BuildEncounter(TEXT("Iglin Bridge"));
	if (Encounter.Groups.Num() < 1)
	{
		AddError(TEXT("Iglin Bridge gave the battle no monster group"));
		return false;
	}

	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Encounter.Groups[0]);
	Battle.Inventory = &Inventory;

	const FSolidBattlePlan* SquadPlan = Battle.GetSquadPlan();
	const FSolidBattlePlan* GroupPlan = Battle.GetGroupPlan();
	TestTrue(TEXT("the battle links the squad"), Battle.Squad.Get() == Squad);
	TestTrue(TEXT("the battle links the Iglin Bridge group"), Battle.Group == &Encounter.Groups[0]);
	TestTrue(TEXT("the squad plan is Starter"), SquadPlan && SquadPlan->Is(TEXT("Starter")));
	TestTrue(TEXT("the group plan is Attack"), GroupPlan && GroupPlan->Is(TEXT("Attack")));
	TestTrue(TEXT("Starter stores attack"), SquadPlan && SquadPlan->Attacks());
	TestTrue(TEXT("Attack stores attack"), GroupPlan && GroupPlan->Attacks());
	TestEqual(TEXT("nothing is logged yet"), Battle.Log.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleActivePlanTest,
	"SolidCore1.Battle.ActivePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleActivePlanTest::RunTest(const FString& Parameters)
{
	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(nullptr);
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;

		const FSolidBattlePlan* SquadPlan = Battle.GetSquadPlan();
		TestTrue(TEXT("a new squad reads Starter"), SquadPlan && SquadPlan->Is(TEXT("Starter")));
		TestNull(TEXT("a group with no plan name has none"), Battle.GetGroupPlan());
		TestEqual(TEXT("that group has its one raptor"), Battle.Group->Num(), 1);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Defend"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(nullptr);
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;

		const FSolidBattlePlan* SquadPlan = Battle.GetSquadPlan();
		TestTrue(TEXT("the battle reads Defend"), SquadPlan && SquadPlan->Is(TEXT("Defend")));
		TestTrue(TEXT("Defend stores retreat"), SquadPlan && SquadPlan->Retreats());
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidEncounter Encounter = SolidBattleTestPrivate::BuildEncounter(TEXT("Iglin Bridge"));
		if (Encounter.Groups.Num() < 1)
		{
			AddError(TEXT("Iglin Bridge gave the battle no monster group"));
			return false;
		}
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Encounter.Groups[0]);
		Battle.Inventory = &Inventory;

		const FSolidBattlePlan* GroupPlan = Battle.GetGroupPlan();
		TestTrue(TEXT("Iglin Bridge reads Attack"), GroupPlan && GroupPlan->Is(TEXT("Attack")));
		TestTrue(TEXT("Attack stores attack"), GroupPlan && GroupPlan->Attacks());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleSidesTest,
	"SolidCore1.Battle.Sides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleSidesTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
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

	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	TestEqual(TEXT("the main hand is the crude spear"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::MainHand), TEXT("Crude spear"));
	TestEqual(TEXT("the body is Rags"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::Body), TEXT("Rags"));
	TestEqual(TEXT("the head is the dented helmet"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::Head), TEXT("Dented helmet"));

	FSolidEncounter Encounter = SolidBattleTestPrivate::BuildEncounter(TEXT("Iglin Bridge"));
	if (Encounter.Groups.Num() < 1 || Encounter.Definition.GroupCount < 1)
	{
		AddError(TEXT("Iglin Bridge gave the battle no monster group"));
		return false;
	}
	FSolidBattle Battle(Squad, &Encounter.Groups[0]);
	Battle.Inventory = &Inventory;

	const SolidEncounter::FGroup& Spec = Encounter.Definition.Groups[0];
	TestEqual(TEXT("the group has the definition's raptors"), Battle.Group->Num(), Spec.Count);

	const FSolidItemType* Spear = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	const int32 SpearPower = Spear
		? SolidItemType::PowerFor(Spear->Quality, FSolidItem::StartingDraw)
		: 0;
	const float RagsStrength = FSolidItem::FromId(SolidItemType::IdRags, 1).Armour.Strength;
	FSolidRaptor One;
	One.Size = ESolidMonsterSize::Small;
	One.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
	TestEqual(TEXT("the squad's power is the crude spear"), Battle.GetSquadPower(), SpearPower);
	TestEqual(TEXT("the squad's defence is the rags"), Battle.GetSquadDefence(), RagsStrength);
	TestEqual(
		TEXT("the group's power is its claws"),
		Battle.GetGroupPower(),
		Spec.Count * One.Weapon.Power);
	TestEqual(
		TEXT("the group's defence is its hides"),
		Battle.GetGroupDefence(),
		static_cast<float>(Spec.Count) * One.Armour.Strength);
	for (int32 Index = 0; Index < Battle.Group->Num(); ++Index)
	{
		const FSolidMonster* Monster = Battle.Group->Get(Index);
		const FString Which = FString::Printf(TEXT("raptor %d"), Index + 1);
		if (!TestNotNull(*Which, Monster))
		{
			return false;
		}
		TestEqual(*FString::Printf(TEXT("%s is a raptor"), *Which), FString(Monster->Kind()), TEXT("Raptor"));
		TestEqual(*FString::Printf(TEXT("%s is Small"), *Which), static_cast<int32>(Monster->Size), static_cast<int32>(ESolidMonsterSize::Small));
		TestEqual(*FString::Printf(TEXT("%s has claws"), *Which), FString(Monster->Weapon.Name), TEXT("Claws"));
		TestEqual(*FString::Printf(TEXT("%s has hide"), *Which), FString(Monster->Armour.Name), TEXT("Hide"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleRagsTest,
	"SolidCore1.Battle.Rags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleRagsTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
	FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Group);
	Battle.Inventory = &Inventory;

	TestEqual(TEXT("rags are 70"), SolidItemType::RagsStrength, 70.f);
	TestEqual(TEXT("the squad wears those rags"), Battle.GetSquadDefence(), 70.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
