#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Battle/SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"
#include "Inventory/SolidItemType.h"
#include "Monster/SolidEncounters.h"
#include "Monster/SolidRaptor.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidBattleTestPrivate
{
	/** Select that assigned plan on the squad. */
	void SelectSquadPlan(USolidSquad* Squad, const TCHAR* Name)
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
	USolidSquad* BuildSquad(const TCHAR* PlanName)
	{
		USolidCompany* Company = NewObject<USolidCompany>();
		USolidSquad* Squad = NewObject<USolidSquad>(Company);
		Squad->InitializeFromCompany(Company);
		Squad->SetSelectedCompanions({});
		SelectSquadPlan(Squad, PlanName);
		return Squad;
	}

	/**
	 * One Small raptor in claws and hide.
	 * The plan name is a monster group battle plan. A missing name leaves none.
	 */
	FSolidMonsterGroup OneRaptor(const TCHAR* PlanName)
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
	FSolidEncounter BuildEncounter(const TCHAR* Name)
	{
		if (!Name || !Name[0])
		{
			return FSolidEncounter();
		}
		for (const SolidEncounter::FDefinition& Candidate : SolidEncounter::Definitions)
		{
			if (Candidate.Name && FCString::Strcmp(Candidate.Name, Name) == 0)
			{
				return FSolidEncounter(Candidate);
			}
		}
		return FSolidEncounter();
	}

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
	TestEqual(TEXT("the main hand is the crude spear"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::MainHand), FString(TEXT("Crude spear")));
	TestEqual(TEXT("the body is Rags"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::Body), FString(TEXT("Rags")));
	TestEqual(TEXT("the head is the dented helmet"), SolidBattleTestPrivate::WornName(Inventory, SolidInventory::ESlot::Head), FString(TEXT("Dented helmet")));

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
		TestEqual(*FString::Printf(TEXT("%s is a raptor"), *Which), FString(Monster->Kind()), FString(TEXT("Raptor")));
		TestEqual(*FString::Printf(TEXT("%s is Small"), *Which), static_cast<int32>(Monster->Size), static_cast<int32>(ESolidMonsterSize::Small));
		TestEqual(*FString::Printf(TEXT("%s has claws"), *Which), FString(Monster->Weapon.Name), FString(TEXT("Claws")));
		TestEqual(*FString::Printf(TEXT("%s has hide"), *Which), FString(Monster->Armour.Name), FString(TEXT("Hide")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleAttackTest,
	"SolidCore1.Battle.Attack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleAttackTest::RunTest(const FString& Parameters)
{
	FSolidRaptor Sample;
	Sample.Size = ESolidMonsterSize::Small;
	Sample.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
	const float Hide = Sample.Armour.Strength;
	const float Rags = FSolidItem::FromId(SolidItemType::IdRags, 1).Armour.Strength;

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		FSolidMonster* Raptor = Group.Get(0);
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
		const FSolidItem* MainHand = MainIndex != INDEX_NONE && Inventory.Worn[MainIndex].IsWeapon()
			? &Inventory.Worn[MainIndex]
			: nullptr;
		if (!Raptor || !MainHand || MainHand->Weapon.Speed <= KINDA_SMALL_NUMBER)
		{
			AddError(TEXT("the one raptor gave the battle no spear and no monster"));
			return false;
		}
		const float SpearSpeed = MainHand->Weapon.Speed;
		Raptor->Weapon.Speed = SpearSpeed * 2.f;

		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the fight did not start"));
			return false;
		}
		TestTrue(TEXT("they prepare at the start"), FMath::IsNearlyEqual(Battle.Time, 0.f));
		TestEqual(TEXT("two prepares"), Battle.Log.Num(), 2);
		TestEqual(TEXT("the captain prepares first"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
		TestEqual(TEXT("the raptor prepares"), Battle.Log[1].Summary(), FString(TEXT("Raptor prepares.")));
		TestEqual(TEXT("the hide is still whole"), Raptor->Armour.Strength, Hide);
		TestEqual(TEXT("the rags are still whole"), Battle.GetSquadDefence(), Rags);

		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the spear did not finish preparing"));
			return false;
		}
		TestTrue(TEXT("the spear is the first blow"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed));
		TestEqual(TEXT("the spear is the only new line"), Battle.Log.Num(), 3);
		TestEqual(TEXT("the spear line"), Battle.Log[2].Summary(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
		TestTrue(TEXT("the spear comes off that hide"), Raptor->Armour.Strength < Hide);
		const float AfterSpear = Raptor->Armour.Strength;
		TestEqual(TEXT("the claws are still preparing"), Battle.GetSquadDefence(), Rags);

		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the claws did not finish preparing"));
			return false;
		}
		TestTrue(TEXT("the claws land on their own Speed"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed * 2.f));
		TestEqual(TEXT("the captain prepares again and the raptor attacks"), Battle.Log.Num(), 5);
		TestEqual(TEXT("recover ends in another prepare"), Battle.Log[3].Summary(), FString(TEXT("Captain prepares.")));
		TestEqual(TEXT("the claws line"), Battle.Log[4].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
		TestEqual(TEXT("the spear has landed once"), Raptor->Armour.Strength, AfterSpear);
		TestTrue(TEXT("the claws come off the rags"), Battle.GetSquadDefence() < Rags);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidEncounter Encounter = SolidBattleTestPrivate::BuildEncounter(TEXT("Iglin Bridge"));
		if (Encounter.Groups.Num() < 1 || Encounter.Groups[0].Num() < 1)
		{
			AddError(TEXT("Iglin Bridge gave the battle no monster group"));
			return false;
		}
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Encounter.Groups[0]);
		Battle.Inventory = &Inventory;
		const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
		const FSolidItem* MainHand = MainIndex != INDEX_NONE ? &Inventory.Worn[MainIndex] : nullptr;
		const FSolidMonster* First = Encounter.Groups[0].Get(0);
		if (!MainHand || !MainHand->IsWeapon() || !First
			|| !FMath::IsNearlyEqual(MainHand->Weapon.Speed, First->Weapon.Speed))
		{
			AddError(TEXT("the spear and the claws do not share a Speed"));
			return false;
		}
		const int32 Count = Battle.Group->Num();

		if (!Battle.AdvanceToNextEvent() || !Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("Iglin Bridge did not reach the first blows"));
			return false;
		}
		TestEqual(TEXT("everyone prepared, then everyone attacked"), Battle.Log.Num(), 2 * (1 + Count));
		TestEqual(TEXT("the captain prepares first"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
		TestEqual(TEXT("the captain attacks before the raptors"), Battle.Log[1 + Count].Summary(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			TestEqual(TEXT("a raptor prepares"), Battle.Log[1 + Index].Summary(), FString(TEXT("Raptor prepares.")));
			const FString Blow = Battle.Log[2 + Count + Index].Summary();
			if (Index + 1 < Count)
			{
				TestEqual(TEXT("a raptor attacks"), Blow, FString(TEXT("Raptor attacks Captain with Claws.")));
			}
			else
			{
				TestTrue(
					TEXT("the last raptor strikes the captain"),
					Blow.StartsWith(TEXT("Raptor ")) && Blow.EndsWith(TEXT(" with Claws.")));
			}
		}
		TestTrue(TEXT("the claws come off the rags"), Battle.GetSquadDefence() < Rags);
		int32 Hit = 0;
		int32 Kept = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FSolidMonster* Monster = Battle.Group->Get(Index);
			if (!Monster)
			{
				AddError(TEXT("a raptor was missing after the attack"));
				return false;
			}
			const float Strength = Monster->Armour.Strength;
			if (Strength < Hide)
			{
				++Hit;
			}
			else if (FMath::IsNearlyEqual(Strength, Hide))
			{
				++Kept;
			}
		}
		TestEqual(TEXT("the spear hits one hide"), Hit, 1);
		TestEqual(TEXT("the other hides stay"), Kept, Count - 1);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Defend"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		if (!Group.Get(0))
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		if (!Battle.AdvanceToNextEvent() || !Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the raptor did not attack"));
			return false;
		}
		TestEqual(TEXT("a retreat leaves the raptor's two lines"), Battle.Log.Num(), 2);
		TestEqual(TEXT("the raptor prepares"), Battle.Log[0].Summary(), FString(TEXT("Raptor prepares.")));
		TestEqual(TEXT("the raptor attacks"), Battle.Log[1].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
		TestEqual(TEXT("the hide stays whole"), Group.Get(0)->Armour.Strength, Hide);
		TestTrue(TEXT("the claws still come off the rags"), Battle.GetSquadDefence() < Rags);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		FSolidMonster* Raptor = Group.Get(0);
		if (!Raptor)
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
		const FSolidItem* MainHand = MainIndex != INDEX_NONE && Inventory.Worn[MainIndex].IsWeapon()
			? &Inventory.Worn[MainIndex]
			: nullptr;
		if (!MainHand || MainHand->Weapon.Speed <= KINDA_SMALL_NUMBER)
		{
			AddError(TEXT("the one raptor gave the battle no spear"));
			return false;
		}
		Raptor->Weapon.Speed = MainHand->Weapon.Speed * 2.f;
		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the fight did not start"));
			return false;
		}
		SolidBattleTestPrivate::SelectSquadPlan(Squad, TEXT("Defend"));
		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the prepare did not finish"));
			return false;
		}
		TestEqual(TEXT("three lines"), Battle.Log.Num(), 3);
		TestEqual(TEXT("the captain prepares"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
		TestEqual(TEXT("the raptor prepares"), Battle.Log[1].Summary(), FString(TEXT("Raptor prepares.")));
		TestEqual(TEXT("the captain abandons the attack"), Battle.Log[2].Summary(), FString(TEXT("Captain abandons the attack.")));
		TestEqual(TEXT("the hide stays whole"), Raptor->Armour.Strength, Hide);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		if (!SolidInventory::PlaceWorn(Inventory, SolidInventory::ESlot::MainHand, nullptr))
		{
			AddError(TEXT("the main hand did not empty"));
			return false;
		}
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		if (!Group.Get(0))
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		if (!Battle.AdvanceToNextEvent() || !Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the raptor did not attack"));
			return false;
		}
		TestEqual(TEXT("an empty main hand leaves the raptor's two lines"), Battle.Log.Num(), 2);
		TestEqual(TEXT("the raptor prepares"), Battle.Log[0].Summary(), FString(TEXT("Raptor prepares.")));
		TestEqual(TEXT("the raptor attacks"), Battle.Log[1].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
		TestEqual(TEXT("the hide stays whole"), Group.Get(0)->Armour.Strength, Hide);
		TestTrue(TEXT("the claws still come off the rags"), Battle.GetSquadDefence() < Rags);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleIglinBridgeTest,
	"SolidCore1.Battle.IglinBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleIglinBridgeTest::RunTest(const FString& Parameters)
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
	const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const FSolidItem* MainHand = MainIndex != INDEX_NONE && Inventory.Worn[MainIndex].IsWeapon()
		? &Inventory.Worn[MainIndex]
		: nullptr;
	const FSolidMonster* Sample = Encounter.Groups[0].Get(0);
	if (!MainHand || !Sample)
	{
		AddError(TEXT("Iglin Bridge gave the battle no spear and no raptor"));
		return false;
	}
	const float Spear = static_cast<float>(MainHand->Power());
	const float Claw = static_cast<float>(Sample->Weapon.Power);

	const float SquadStart = Battle.GetSquadDefence();
	const float GroupStart = Battle.GetGroupDefence();

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the fight did not start"));
		return false;
	}
	TestEqual(TEXT("they prepare"), Battle.Log.Num(), 4);
	TestEqual(TEXT("the captain prepares"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the first raptor prepares"), Battle.Log[1].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the second raptor prepares"), Battle.Log[2].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the third raptor prepares"), Battle.Log[3].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), SquadStart);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), GroupStart);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the first blows did not land"));
		return false;
	}
	TestEqual(TEXT("they attack"), Battle.Log.Num(), 8);
	TestEqual(TEXT("the spear"), Battle.Log[4].Summary(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
	TestEqual(TEXT("the first claws"), Battle.Log[5].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	TestEqual(TEXT("the second claws"), Battle.Log[6].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	TestEqual(TEXT("the third claws"), Battle.Log[7].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	const float SquadAfter = SquadStart - static_cast<float>(Battle.Group->Num()) * Claw;
	const float GroupAfter = GroupStart - Spear;
	TestEqual(TEXT("the rags after three claws"), Battle.GetSquadDefence(), SquadAfter);
	TestEqual(TEXT("the hides after one spear"), Battle.GetGroupDefence(), GroupAfter);
	TestFalse(TEXT("both sides still have armour"), Battle.Finished());

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("they did not prepare again"));
		return false;
	}
	TestEqual(TEXT("they prepare again"), Battle.Log.Num(), 12);
	TestEqual(TEXT("the captain prepares again"), Battle.Log[8].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the first raptor prepares again"), Battle.Log[9].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the second raptor prepares again"), Battle.Log[10].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the third raptor prepares again"), Battle.Log[11].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), SquadAfter);
	TestEqual(TEXT("the hides hold"), Battle.GetGroupDefence(), GroupAfter);
	TestFalse(TEXT("the fight is still going"), Battle.Finished());

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the second blows did not land"));
		return false;
	}
	TestEqual(TEXT("they attack again"), Battle.Log.Num(), 16);
	const FString SecondSpear = Battle.Log[12].Summary();
	const float GroupNow = Battle.GetGroupDefence();
	const float SameHide = GroupAfter - (FSolidRaptor::HideStrength - Spear);
	const float FreshHide = GroupAfter - Spear;
	const bool bFinishedThatHide = FMath::IsNearlyEqual(GroupNow, SameHide);
	const bool bOpenedAnother = FMath::IsNearlyEqual(GroupNow, FreshHide);
	TestTrue(TEXT("the second spear lands on one hide"), bFinishedThatHide || bOpenedAnother);
	if (bFinishedThatHide)
	{
		TestEqual(
			TEXT("the spear finishes the wounded hide"),
			SecondSpear,
			FString(TEXT("Captain kills Raptor with Crude spear.")));
	}
	else
	{
		TestEqual(
			TEXT("the spear wounds a fresh hide"),
			SecondSpear,
			FString(TEXT("Captain attacks Raptor with Crude spear.")));
	}
	TestEqual(TEXT("the first claws kill"), Battle.Log[13].Summary(), FString(TEXT("Raptor kills Captain with Claws.")));
	TestTrue(TEXT("the second raptor follows"), Battle.Log[14].Summary().StartsWith(TEXT("Raptor ")));
	TestTrue(TEXT("the third raptor follows"), Battle.Log[15].Summary().StartsWith(TEXT("Raptor ")));
	TestEqual(TEXT("the rags are gone"), Battle.GetSquadDefence(), 0.f);
	TestTrue(TEXT("the group still has hide"), GroupNow > 0.f);
	TestTrue(TEXT("one side has no armour left"), Battle.Finished());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleArmourFloorTest,
	"SolidCore1.Battle.ArmourFloor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleArmourFloorTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
	FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
	FSolidMonster* Raptor = Group.Get(0);
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Group);
	Battle.Inventory = &Inventory;
	const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const FSolidItem* MainHand = MainIndex != INDEX_NONE && Inventory.Worn[MainIndex].IsWeapon()
		? &Inventory.Worn[MainIndex]
		: nullptr;
	if (!Raptor || !MainHand || MainHand->Weapon.Speed <= KINDA_SMALL_NUMBER)
	{
		AddError(TEXT("the one raptor gave the battle no spear and no monster"));
		return false;
	}
	Raptor->Weapon.Speed = MainHand->Weapon.Speed * 2.f;

	const float SquadStart = Battle.GetSquadDefence();
	const float GroupStart = Battle.GetGroupDefence();

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the fight did not start"));
		return false;
	}
	TestEqual(TEXT("they prepare"), Battle.Log.Num(), 2);
	TestEqual(TEXT("the captain prepares"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the raptor prepares"), Battle.Log[1].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), SquadStart);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), GroupStart);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the spear did not land"));
		return false;
	}
	TestEqual(TEXT("the spear"), Battle.Log.Num(), 3);
	TestEqual(TEXT("the spear line"), Battle.Log[2].Summary(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
	TestEqual(TEXT("the squad is still whole"), Battle.GetSquadDefence(), SquadStart);
	TestTrue(TEXT("the spear comes off the hide"), Battle.GetGroupDefence() < GroupStart);
	TestTrue(TEXT("the hide stays at or above 0"), Battle.GetGroupDefence() >= 0.f);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the claws did not land"));
		return false;
	}
	TestEqual(TEXT("the captain prepares again and the raptor attacks"), Battle.Log.Num(), 5);
	TestEqual(TEXT("recover ends in another prepare"), Battle.Log[3].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the claws line"), Battle.Log[4].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	TestTrue(TEXT("the claws come off the rags"), Battle.GetSquadDefence() < SquadStart);
	TestTrue(TEXT("the rags stay at or above 0"), Battle.GetSquadDefence() >= 0.f);

	auto ActCount = [&Battle](const TCHAR* Actor)
	{
		int32 Count = 0;
		for (const FSolidBattleLogEntry& Entry : Battle.Log)
		{
			if (Entry.Kind != ESolidBattleLogKind::Prepare && Entry.Actor == Actor)
			{
				++Count;
			}
		}
		return Count;
	};

	int32 Guard = 0;
	while (Battle.GetGroupDefence() > 0.f)
	{
		if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
		{
			AddError(TEXT("the hide did not reach 0"));
			return false;
		}
	}
	TestEqual(TEXT("the hide reaches 0"), Battle.GetGroupDefence(), 0.f);
	const int32 HideActs = ActCount(TEXT("Captain"));
	while (ActCount(TEXT("Captain")) == HideActs)
	{
		if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
		{
			AddError(TEXT("the hide was not struck again"));
			return false;
		}
	}
	TestEqual(TEXT("another spear stays at 0"), Battle.GetGroupDefence(), 0.f);

	while (Battle.GetSquadDefence() > 0.f)
	{
		if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
		{
			AddError(TEXT("the rags did not reach 0"));
			return false;
		}
	}
	TestEqual(TEXT("the rags reach 0"), Battle.GetSquadDefence(), 0.f);
	const int32 RagActs = ActCount(TEXT("Raptor"));
	while (ActCount(TEXT("Raptor")) == RagActs)
	{
		if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
		{
			AddError(TEXT("the rags were not struck again"));
			return false;
		}
	}
	TestEqual(TEXT("another claw stays at 0"), Battle.GetSquadDefence(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleKillTest,
	"SolidCore1.Battle.Kill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleKillTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
	FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
	FSolidMonster* Raptor = Group.Get(0);
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Group);
	Battle.Inventory = &Inventory;
	const int32 MainIndex = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const FSolidItem* MainHand = MainIndex != INDEX_NONE && Inventory.Worn[MainIndex].IsWeapon()
		? &Inventory.Worn[MainIndex]
		: nullptr;
	if (!Raptor || !MainHand)
	{
		AddError(TEXT("the one raptor gave the battle no spear and no monster"));
		return false;
	}

	const float Rags = Battle.GetSquadDefence();
	const float Hide = Battle.GetGroupDefence();
	const float Spear = static_cast<float>(MainHand->Power());
	const float Claws = static_cast<float>(Raptor->Weapon.Power);
	TestEqual(TEXT("the hide is 20"), Hide, 20.f);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the fight did not start"));
		return false;
	}
	TestEqual(TEXT("they prepare"), Battle.Log.Num(), 2);
	TestEqual(TEXT("the captain prepares"), Battle.Log[0].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the raptor prepares"), Battle.Log[1].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), Rags);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), Hide);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the first blows did not land"));
		return false;
	}
	const float AfterSpear = FMath::Max(0.f, Hide - Spear);
	const float AfterClaw = FMath::Max(0.f, Rags - Claws);
	TestEqual(TEXT("they attack"), Battle.Log.Num(), 4);
	TestEqual(TEXT("the spear"), Battle.Log[2].Summary(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
	TestEqual(TEXT("the claws"), Battle.Log[3].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	TestEqual(TEXT("the rags after one claw"), Battle.GetSquadDefence(), AfterClaw);
	TestEqual(TEXT("the hide after one spear"), Battle.GetGroupDefence(), AfterSpear);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("they did not prepare again"));
		return false;
	}
	TestEqual(TEXT("they prepare again"), Battle.Log.Num(), 6);
	TestEqual(TEXT("the captain prepares again"), Battle.Log[4].Summary(), FString(TEXT("Captain prepares.")));
	TestEqual(TEXT("the raptor prepares again"), Battle.Log[5].Summary(), FString(TEXT("Raptor prepares.")));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), AfterClaw);
	TestEqual(TEXT("the hide holds"), Battle.GetGroupDefence(), AfterSpear);

	if (!Battle.AdvanceToNextEvent())
	{
		AddError(TEXT("the second blows did not land"));
		return false;
	}
	const float SecondClaw = FMath::Max(0.f, AfterClaw - Claws);
	TestEqual(TEXT("the second blows"), Battle.Log.Num(), 8);
	TestEqual(TEXT("the spear kills"), Battle.Log[6].Summary(), FString(TEXT("Captain kills Raptor with Crude spear.")));
	TestEqual(TEXT("the claws still attack"), Battle.Log[7].Summary(), FString(TEXT("Raptor attacks Captain with Claws.")));
	TestTrue(TEXT("two claws leave the rags up"), SecondClaw > 0.f);
	TestEqual(TEXT("the rags after two claws"), Battle.GetSquadDefence(), SecondClaw);
	TestEqual(TEXT("the hide is gone"), Battle.GetGroupDefence(), 0.f);

	const FString KillLine(TEXT("Raptor kills Captain with Claws."));
	int32 Guard = 0;
	while (Battle.Log.Num() == 0 || Battle.Log.Last().Summary() != KillLine)
	{
		if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
		{
			AddError(TEXT("the claws did not kill"));
			return false;
		}
	}
	TestEqual(TEXT("the rags are gone"), Battle.GetSquadDefence(), 0.f);
	TestEqual(TEXT("the hide stays at 0"), Battle.GetGroupDefence(), 0.f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleAbortTest,
	"SolidCore1.Battle.Abort",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleAbortTest::RunTest(const FString& Parameters)
{
	auto HasLine = [](const FSolidBattle& Battle, const TCHAR* Sentence)
	{
		for (const FSolidBattleLogEntry& Entry : Battle.Log)
		{
			if (Entry.Summary() == Sentence)
			{
				return true;
			}
		}
		return false;
	};

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		if (!Group.Get(0))
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;

		int32 Guard = 0;
		while (!HasLine(Battle, TEXT("Captain kills Raptor with Crude spear.")))
		{
			if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
			{
				AddError(TEXT("the spear did not kill"));
				return false;
			}
		}
		TestEqual(TEXT("the hide is gone"), Battle.GetGroupDefence(), 0.f);

		while (!HasLine(Battle, TEXT("Captain aborts the attack on Raptor.")))
		{
			if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
			{
				AddError(TEXT("the spear was not aborted"));
				return false;
			}
		}
		TestEqual(TEXT("the aborted spear leaves the hide at 0"), Battle.GetGroupDefence(), 0.f);
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

		int32 Guard = 0;
		int32 KillAt = INDEX_NONE;
		while (KillAt == INDEX_NONE || Battle.Log.Num() < KillAt + 3)
		{
			if (!Battle.AdvanceToNextEvent() || ++Guard > 40)
			{
				AddError(TEXT("the claws did not reach a dead captain"));
				return false;
			}
			if (KillAt == INDEX_NONE)
			{
				for (int32 Index = 0; Index < Battle.Log.Num(); ++Index)
				{
					if (Battle.Log[Index].Summary() == TEXT("Raptor kills Captain with Claws."))
					{
						KillAt = Index;
						break;
					}
				}
			}
		}
		TestEqual(TEXT("the second raptor aborts"), Battle.Log[KillAt + 1].Summary(), FString(TEXT("Raptor aborts the attack on Captain.")));
		TestEqual(TEXT("the third raptor aborts"), Battle.Log[KillAt + 2].Summary(), FString(TEXT("Raptor aborts the attack on Captain.")));
		TestEqual(TEXT("the rags are gone"), Battle.GetSquadDefence(), 0.f);
		TestTrue(TEXT("the group still has hide"), Battle.GetGroupDefence() > 0.f);
		TestTrue(TEXT("one side has no armour left"), Battle.Finished());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
