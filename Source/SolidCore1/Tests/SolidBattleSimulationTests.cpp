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

	/**
	 * The summary of that log line.
	 * A missing line is reported, and the summary comes back empty, so an assertion can use it directly.
	 */
	FString LogSummary(FAutomationTestBase& Test, const FSolidBattle& Battle, const int32 Index)
	{
		if (!Battle.Log.IsValidIndex(Index))
		{
			Test.AddError(FString::Printf(
				TEXT("the log has %d lines, so there is no entry at index %d"),
				Battle.Log.Num(),
				Index));
			return FString();
		}
		return Battle.Log[Index].Summary();
	}
}

/** Visible here so each fight line can call it without the namespace. */
using SolidBattleTestPrivate::LogSummary;

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

		Battle.AdvanceToNextEvent();
		TestTrue(TEXT("they prepare at the start"), FMath::IsNearlyEqual(Battle.Time, 0.f));
		TestEqual(TEXT("the captain prepares first"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
		TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
		TestEqual(TEXT("the hide is still whole"), Raptor->Armour.Strength, Hide);
		TestEqual(TEXT("the rags are still whole"), Battle.GetSquadDefence(), Rags);

		Battle.AdvanceToNextEvent();
		TestTrue(TEXT("the spear is the first blow"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed));
		TestEqual(TEXT("the spear line"), LogSummary(*this, Battle, 2), TEXT("Captain attacks Raptor with Crude spear."));
		TestTrue(TEXT("the spear comes off that hide"), Raptor->Armour.Strength < Hide);
		const float AfterSpear = Raptor->Armour.Strength;
		TestEqual(TEXT("the claws are still preparing"), Battle.GetSquadDefence(), Rags);

		Battle.AdvanceToNextEvent();
		TestTrue(TEXT("the claws land on their own Speed"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed * 2.f));
		TestEqual(TEXT("recover ends in another prepare"), LogSummary(*this, Battle, 3), TEXT("Captain prepares."));
		TestEqual(TEXT("the claws line"), LogSummary(*this, Battle, 4), TEXT("Raptor attacks Captain with Claws."));
		TestEqual(TEXT("five lines"), Battle.Log.Num(), 5);
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

		Battle.AdvanceToNextEvent();
		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the captain prepares first"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
		TestEqual(TEXT("the captain attacks before the raptors"), LogSummary(*this, Battle, 1 + Count), TEXT("Captain attacks Raptor with Crude spear."));
		for (int32 Index = 0; Index < Count; ++Index)
		{
			TestEqual(TEXT("a raptor prepares"), LogSummary(*this, Battle, 1 + Index), TEXT("Raptor prepares."));
			const FString Blow = LogSummary(*this, Battle, 2 + Count + Index);
			if (Index + 1 < Count)
			{
				TestEqual(TEXT("a raptor attacks"), Blow, TEXT("Raptor attacks Captain with Claws."));
			}
			else
			{
				TestTrue(
					TEXT("the last raptor strikes the captain"),
					Blow.StartsWith(TEXT("Raptor ")) && Blow.EndsWith(TEXT(" with Claws.")));
			}
		}
		TestEqual(TEXT("everyone prepared, then everyone attacked"), Battle.Log.Num(), 2 * (1 + Count));
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
		Battle.AdvanceToNextEvent();
		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 0), TEXT("Raptor prepares."));
		TestEqual(TEXT("the raptor attacks"), LogSummary(*this, Battle, 1), TEXT("Raptor attacks Captain with Claws."));
		TestEqual(TEXT("a retreat leaves the raptor's two lines"), Battle.Log.Num(), 2);
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
		Battle.AdvanceToNextEvent();
		SolidBattleTestPrivate::SelectSquadPlan(Squad, TEXT("Defend"));
		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
		TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
		TestEqual(TEXT("the captain abandons the attack"), LogSummary(*this, Battle, 2), TEXT("Captain abandons the attack."));
		TestEqual(TEXT("three lines"), Battle.Log.Num(), 3);
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
		Battle.AdvanceToNextEvent();
		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 0), TEXT("Raptor prepares."));
		TestEqual(TEXT("the raptor attacks"), LogSummary(*this, Battle, 1), TEXT("Raptor attacks Captain with Claws."));
		TestEqual(TEXT("an empty main hand leaves the raptor's two lines"), Battle.Log.Num(), 2);
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

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
	TestEqual(TEXT("the first raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
	TestEqual(TEXT("the second raptor prepares"), LogSummary(*this, Battle, 2), TEXT("Raptor prepares."));
	TestEqual(TEXT("the third raptor prepares"), LogSummary(*this, Battle, 3), TEXT("Raptor prepares."));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), SquadStart);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), GroupStart);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear"), LogSummary(*this, Battle, 4), TEXT("Captain attacks Raptor with Crude spear."));
	TestEqual(TEXT("the first claws"), LogSummary(*this, Battle, 5), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("the second claws"), LogSummary(*this, Battle, 6), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("the third claws"), LogSummary(*this, Battle, 7), TEXT("Raptor attacks Captain with Claws."));
	const float SquadAfter = SquadStart - static_cast<float>(Battle.Group->Num()) * Claw;
	const float GroupAfter = GroupStart - Spear;
	TestEqual(TEXT("the rags after three claws"), Battle.GetSquadDefence(), SquadAfter);
	TestEqual(TEXT("the hides after one spear"), Battle.GetGroupDefence(), GroupAfter);
	TestFalse(TEXT("both sides still have armour"), Battle.Finished());

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares again"), LogSummary(*this, Battle, 8), TEXT("Captain prepares."));
	TestEqual(TEXT("the first raptor prepares again"), LogSummary(*this, Battle, 9), TEXT("Raptor prepares."));
	TestEqual(TEXT("the second raptor prepares again"), LogSummary(*this, Battle, 10), TEXT("Raptor prepares."));
	TestEqual(TEXT("the third raptor prepares again"), LogSummary(*this, Battle, 11), TEXT("Raptor prepares."));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), SquadAfter);
	TestEqual(TEXT("the hides hold"), Battle.GetGroupDefence(), GroupAfter);
	TestFalse(TEXT("the fight is still going"), Battle.Finished());

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear finishes that hide"), LogSummary(*this, Battle, 12), TEXT("Captain kills Raptor with Crude spear."));
	const float GroupNow = GroupAfter - (FSolidRaptor::HideStrength - Spear);
	TestEqual(TEXT("the wounded hide is gone"), Battle.GetGroupDefence(), GroupNow);
	TestEqual(TEXT("the first claws kill"), LogSummary(*this, Battle, 13), TEXT("Raptor kills Captain with Claws."));
	TestEqual(TEXT("the second claws abort"), LogSummary(*this, Battle, 14), TEXT("Raptor aborts the attack on Captain."));
	TestEqual(TEXT("the third claws abort"), LogSummary(*this, Battle, 15), TEXT("Raptor aborts the attack on Captain."));
	TestEqual(TEXT("sixteen lines"), Battle.Log.Num(), 16);
	TestEqual(TEXT("the rags are gone"), Battle.GetSquadDefence(), 0.f);
	TestTrue(TEXT("the group still has hide"), Battle.GetGroupDefence() > 0.f);
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

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
	TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), SquadStart);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), GroupStart);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear line"), LogSummary(*this, Battle, 2), TEXT("Captain attacks Raptor with Crude spear."));
	TestEqual(TEXT("the squad is still whole"), Battle.GetSquadDefence(), SquadStart);
	TestTrue(TEXT("the spear comes off the hide"), Battle.GetGroupDefence() < GroupStart);
	TestTrue(TEXT("the hide stays at or above 0"), Battle.GetGroupDefence() >= 0.f);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("recover ends in another prepare"), LogSummary(*this, Battle, 3), TEXT("Captain prepares."));
	TestEqual(TEXT("the claws line"), LogSummary(*this, Battle, 4), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("five lines"), Battle.Log.Num(), 5);
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
		Battle.AdvanceToNextEvent();
		if (++Guard > 40)
		{
			AddError(TEXT("the hide did not reach 0"));
			return false;
		}
	}
	TestEqual(TEXT("the hide reaches 0"), Battle.GetGroupDefence(), 0.f);
	const int32 HideActs = ActCount(TEXT("Captain"));
	while (ActCount(TEXT("Captain")) == HideActs)
	{
		Battle.AdvanceToNextEvent();
		if (++Guard > 40)
		{
			AddError(TEXT("the hide was not struck again"));
			return false;
		}
	}
	TestEqual(TEXT("another spear stays at 0"), Battle.GetGroupDefence(), 0.f);

	while (Battle.GetSquadDefence() > 0.f)
	{
		Battle.AdvanceToNextEvent();
		if (++Guard > 40)
		{
			AddError(TEXT("the rags did not reach 0"));
			return false;
		}
	}
	TestEqual(TEXT("the rags reach 0"), Battle.GetSquadDefence(), 0.f);
	const int32 RagActs = ActCount(TEXT("Raptor"));
	while (ActCount(TEXT("Raptor")) == RagActs)
	{
		Battle.AdvanceToNextEvent();
		if (++Guard > 40)
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

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
	TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), Rags);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), Hide);

	Battle.AdvanceToNextEvent();
	const float AfterSpear = FMath::Max(0.f, Hide - Spear);
	const float AfterClaw = FMath::Max(0.f, Rags - Claws);
	TestEqual(TEXT("the spear"), LogSummary(*this, Battle, 2), TEXT("Captain attacks Raptor with Crude spear."));
	TestEqual(TEXT("the claws"), LogSummary(*this, Battle, 3), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("the rags after one claw"), Battle.GetSquadDefence(), AfterClaw);
	TestEqual(TEXT("the hide after one spear"), Battle.GetGroupDefence(), AfterSpear);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares again"), LogSummary(*this, Battle, 4), TEXT("Captain prepares."));
	TestEqual(TEXT("the raptor prepares again"), LogSummary(*this, Battle, 5), TEXT("Raptor prepares."));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), AfterClaw);
	TestEqual(TEXT("the hide holds"), Battle.GetGroupDefence(), AfterSpear);

	Battle.AdvanceToNextEvent();
	const float SecondClaw = FMath::Max(0.f, AfterClaw - Claws);
	TestEqual(TEXT("the spear kills"), LogSummary(*this, Battle, 6), TEXT("Captain kills Raptor with Crude spear."));
	TestEqual(TEXT("the claws still attack"), LogSummary(*this, Battle, 7), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("eight lines"), Battle.Log.Num(), 8);
	TestTrue(TEXT("two claws leave the rags up"), SecondClaw > 0.f);
	TestEqual(TEXT("the rags after two claws"), Battle.GetSquadDefence(), SecondClaw);
	TestEqual(TEXT("the hide is gone"), Battle.GetGroupDefence(), 0.f);

	const FString KillLine(TEXT("Raptor kills Captain with Claws."));
	int32 Guard = 0;
	while (Battle.Log.Num() == 0 || Battle.Log.Last().Summary() != KillLine)
	{
		Battle.AdvanceToNextEvent();
		if (++Guard > 40)
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
			Battle.AdvanceToNextEvent();
			if (++Guard > 40)
			{
				AddError(TEXT("the spear did not kill"));
				return false;
			}
		}
		TestEqual(TEXT("the hide is gone"), Battle.GetGroupDefence(), 0.f);

		while (!HasLine(Battle, TEXT("Captain aborts the attack on Raptor.")))
		{
			Battle.AdvanceToNextEvent();
			if (++Guard > 40)
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
			Battle.AdvanceToNextEvent();
			if (++Guard > 40)
			{
				AddError(TEXT("the claws did not reach a dead captain"));
				return false;
			}
			if (KillAt == INDEX_NONE)
			{
				for (int32 Index = 0; Index < Battle.Log.Num(); ++Index)
				{
					if (LogSummary(*this, Battle, Index) == TEXT("Raptor kills Captain with Claws."))
					{
						KillAt = Index;
						break;
					}
				}
			}
		}
		TestEqual(TEXT("the second raptor aborts"), LogSummary(*this, Battle, KillAt + 1), TEXT("Raptor aborts the attack on Captain."));
		TestEqual(TEXT("the third raptor aborts"), LogSummary(*this, Battle, KillAt + 2), TEXT("Raptor aborts the attack on Captain."));
		TestEqual(TEXT("the rags are gone"), Battle.GetSquadDefence(), 0.f);
		TestTrue(TEXT("the group still has hide"), Battle.GetGroupDefence() > 0.f);
		TestTrue(TEXT("one side has no armour left"), Battle.Finished());
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
