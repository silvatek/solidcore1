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

	int32 CountLog(const TArray<FSolidBattleLogEntry>& Log, const ESolidBattleLogKind Kind, const TCHAR* Actor)
	{
		int32 Count = 0;
		for (const FSolidBattleLogEntry& Entry : Log)
		{
			if (Entry.Kind == Kind && Entry.Actor == Actor)
			{
				++Count;
			}
		}
		return Count;
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
	const int32 Claws = Sample.Weapon.Power;
	const float Hide = Sample.Armour.Strength;
	const FSolidItemType* SpearType = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	const int32 Spear = SpearType
		? SolidItemType::PowerFor(SpearType->Quality, FSolidItem::StartingDraw)
		: 0;
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
		TestTrue(TEXT("the captain prepares first"), Battle.Log[0].Kind == ESolidBattleLogKind::Prepare && Battle.Log[0].Actor == TEXT("Captain"));
		TestEqual(TEXT("the captain's prepare line"), Battle.Log[0].Text(), FString(TEXT("Captain prepares.")));
		TestTrue(TEXT("the raptor prepares"), Battle.Log[1].Kind == ESolidBattleLogKind::Prepare && Battle.Log[1].Actor == TEXT("Raptor"));
		TestEqual(TEXT("the hide is still whole"), Raptor->Armour.Strength, Hide);
		TestEqual(TEXT("the rags are still whole"), Battle.GetSquadDefence(), Rags);

		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the spear did not finish preparing"));
			return false;
		}
		TestTrue(TEXT("the spear is the first blow"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed));
		TestEqual(TEXT("the spear is the only new line"), Battle.Log.Num(), 3);
		TestTrue(TEXT("the captain attacks"), Battle.Log[2].Kind == ESolidBattleLogKind::Attack && Battle.Log[2].Actor == TEXT("Captain"));
		TestEqual(TEXT("the spear names the raptor"), Battle.Log[2].Target, FString(TEXT("Raptor")));
		TestEqual(TEXT("the spear names itself"), Battle.Log[2].Weapon, FString(TEXT("Crude spear")));
		TestEqual(TEXT("the spear line"), Battle.Log[2].Text(), FString(TEXT("Captain attacks Raptor with Crude spear.")));
		TestEqual(TEXT("the spear comes off that hide"), Raptor->Armour.Strength, Hide - static_cast<float>(Spear));
		TestEqual(TEXT("the claws are still preparing"), Battle.GetSquadDefence(), Rags);

		if (!Battle.AdvanceToNextEvent())
		{
			AddError(TEXT("the claws did not finish preparing"));
			return false;
		}
		TestTrue(TEXT("the claws land on their own Speed"), FMath::IsNearlyEqual(Battle.Time, SpearSpeed * 2.f));
		TestEqual(TEXT("the captain prepares again and the raptor attacks"), Battle.Log.Num(), 5);
		TestTrue(TEXT("recover ends in another prepare"), Battle.Log[3].Kind == ESolidBattleLogKind::Prepare && Battle.Log[3].Actor == TEXT("Captain"));
		TestTrue(TEXT("the raptor attacks"), Battle.Log[4].Kind == ESolidBattleLogKind::Attack && Battle.Log[4].Actor == TEXT("Raptor"));
		TestEqual(TEXT("the claws name the captain"), Battle.Log[4].Target, FString(TEXT("Captain")));
		TestEqual(TEXT("the claws name themselves"), Battle.Log[4].Weapon, FString(TEXT("Claws")));
		TestEqual(TEXT("the claws line"), Battle.Log[4].Text(), FString(TEXT("Raptor attacks Captain with Claws.")));
		TestEqual(TEXT("the spear has landed once"), Raptor->Armour.Strength, Hide - static_cast<float>(Spear));
		TestEqual(
			TEXT("the captain attacked once"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Attack, TEXT("Captain")),
			1);
		TestEqual(TEXT("the claws come off the rags"), Battle.GetSquadDefence(), Rags - static_cast<float>(Claws));
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
		TestTrue(TEXT("the captain attacks before the raptors"), Battle.Log[1 + Count].Kind == ESolidBattleLogKind::Attack
			&& Battle.Log[1 + Count].Actor == TEXT("Captain"));
		TestEqual(
			TEXT("each raptor attacks"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Attack, TEXT("Raptor")),
			Count);
		TestEqual(
			TEXT("each raptor's claws come off the rags"),
			Battle.GetSquadDefence(),
			Rags - static_cast<float>(Count * Claws));
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
			if (FMath::IsNearlyEqual(Strength, Hide - static_cast<float>(Spear)))
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
		TestEqual(
			TEXT("a retreat does not prepare"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Prepare, TEXT("Captain")),
			0);
		TestEqual(
			TEXT("a retreat does not strike"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Attack, TEXT("Captain")),
			0);
		TestEqual(TEXT("the hide stays whole"), Group.Get(0)->Armour.Strength, Hide);
		TestEqual(TEXT("the claws still come off the rags"), Battle.GetSquadDefence(), Rags - static_cast<float>(Claws));
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
		TestEqual(TEXT("the last line is the abandon"), Battle.Log.Num(), 3);
		TestTrue(TEXT("the captain abandons the attack"), Battle.Log[2].Kind == ESolidBattleLogKind::Abandon && Battle.Log[2].Actor == TEXT("Captain"));
		TestEqual(TEXT("the abandon line"), Battle.Log[2].Text(), FString(TEXT("Captain abandons the attack.")));
		TestEqual(
			TEXT("the abandoned blow does not land"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Attack, TEXT("Captain")),
			0);
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
		TestEqual(
			TEXT("an empty main hand does not prepare"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Prepare, TEXT("Captain")),
			0);
		TestEqual(
			TEXT("an empty main hand does not strike"),
			SolidBattleTestPrivate::CountLog(Battle.Log, ESolidBattleLogKind::Attack, TEXT("Captain")),
			0);
		TestEqual(TEXT("the hide stays whole"), Group.Get(0)->Armour.Strength, Hide);
		TestEqual(TEXT("the claws still come off the rags"), Battle.GetSquadDefence(), Rags - static_cast<float>(Claws));
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
