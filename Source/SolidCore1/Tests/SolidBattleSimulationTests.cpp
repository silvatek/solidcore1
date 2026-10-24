#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Combat/SolidBattle.h"
#include "Game/SolidGameState.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"
#include "Inventory/SolidItemType.h"
#include "Events/SolidEncounters.h"
#include "Monster/SolidRaptor.h"
#include "SolidBattleTestHelpers.h"
#include "SolidGameInstance.h"
#include "SolidGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidBattleTestPrivate
{
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
		const float SpearSpeed = Battle.SquadWeapon(0).Speed;
		if (!Raptor || SpearSpeed <= KINDA_SMALL_NUMBER)
		{
			AddError(TEXT("the one raptor gave the battle no spear and no monster"));
			return false;
		}
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
		if (!FMath::IsNearlyEqual(Battle.SquadWeapon(0).Speed, Battle.MonsterWeapon(0).Speed))
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
		const float SpearSpeed = Battle.SquadWeapon(0).Speed;
		if (SpearSpeed <= KINDA_SMALL_NUMBER)
		{
			AddError(TEXT("the one raptor gave the battle no spear"));
			return false;
		}
		Raptor->Weapon.Speed = SpearSpeed * 2.f;
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
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Encounter.Groups[0]);
	Battle.Inventory = &Inventory;

	const float Spear = Battle.SquadWeapon(0).Power;
	const float Claw = Battle.MonsterWeapon(0).Power;

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
	TestEqual(TEXT("the rags after three claws"), Battle.GetSquadDefence(), SquadStart - 3 * Claw);
	TestEqual(TEXT("the hides after one spear"), Battle.GetGroupDefence(), GroupStart - Spear);
	TestFalse(TEXT("both sides still have armour"), Battle.Finished());

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares again"), LogSummary(*this, Battle, 8), TEXT("Captain prepares."));
	TestEqual(TEXT("the first raptor prepares again"), LogSummary(*this, Battle, 9), TEXT("Raptor prepares."));
	TestEqual(TEXT("the second raptor prepares again"), LogSummary(*this, Battle, 10), TEXT("Raptor prepares."));
	TestEqual(TEXT("the third raptor prepares again"), LogSummary(*this, Battle, 11), TEXT("Raptor prepares."));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), SquadStart - 3 * Claw);
	TestEqual(TEXT("the hides hold"), Battle.GetGroupDefence(), GroupStart - Spear);
	TestFalse(TEXT("the fight is still going"), Battle.Finished());

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear finishes that hide"), LogSummary(*this, Battle, 12), TEXT("Captain kills Raptor with Crude spear."));
	TestEqual(TEXT("the wounded hide is gone"), Battle.GetGroupDefence(), GroupStart - FSolidRaptor::HideStrength);
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
	const float SpearSpeed = Battle.SquadWeapon(0).Speed;
	if (!Raptor || SpearSpeed <= KINDA_SMALL_NUMBER)
	{
		AddError(TEXT("the one raptor gave the battle no spear and no monster"));
		return false;
	}
	Raptor->Weapon.Speed = SpearSpeed * 2.f;

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
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Group);
	Battle.Inventory = &Inventory;

	const float Rags = Battle.GetSquadDefence();
	const float Hide = Battle.GetGroupDefence();
	const float Spear = Battle.SquadWeapon(0).Power;
	const float Claws = Battle.MonsterWeapon(0).Power;
	TestEqual(TEXT("the hide is 20"), Hide, 20.f);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
	TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 1), TEXT("Raptor prepares."));
	TestEqual(TEXT("the squad's armour"), Battle.GetSquadDefence(), Rags);
	TestEqual(TEXT("the group's armour"), Battle.GetGroupDefence(), Hide);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear"), LogSummary(*this, Battle, 2), TEXT("Captain attacks Raptor with Crude spear."));
	TestEqual(TEXT("the claws"), LogSummary(*this, Battle, 3), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("the rags after one claw"), Battle.GetSquadDefence(), Rags - Claws);
	TestEqual(TEXT("the hide after one spear"), Battle.GetGroupDefence(), Hide - Spear);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the captain prepares again"), LogSummary(*this, Battle, 4), TEXT("Captain prepares."));
	TestEqual(TEXT("the raptor prepares again"), LogSummary(*this, Battle, 5), TEXT("Raptor prepares."));
	TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), Rags - Claws);
	TestEqual(TEXT("the hide holds"), Battle.GetGroupDefence(), Hide - Spear);

	Battle.AdvanceToNextEvent();
	TestEqual(TEXT("the spear kills"), LogSummary(*this, Battle, 6), TEXT("Captain kills Raptor with Crude spear."));
	TestEqual(TEXT("the claws still attack"), LogSummary(*this, Battle, 7), TEXT("Raptor attacks Captain with Claws."));
	TestEqual(TEXT("eight lines"), Battle.Log.Num(), 8);
	TestTrue(TEXT("two claws leave the rags up"), Battle.GetSquadDefence() > 0.f);
	TestEqual(TEXT("the rags after two claws"), Battle.GetSquadDefence(), Rags - 2 * Claws);
	TestEqual(TEXT("the hide is gone"), Battle.GetGroupDefence(), 0.f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleRetreatTest,
	"SolidCore1.Battle.Retreat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleRetreatTest::RunTest(const FString& Parameters)
{
	const float Kept = 1.f - FSolidBattle::RetreatReduction;

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Defend"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;

		const float Claw = Battle.MonsterWeapon(0).Power;
		const float SquadStart = Battle.GetSquadDefence();
		const float GroupStart = Battle.GetGroupDefence();

		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the raptor prepares"), LogSummary(*this, Battle, 0), TEXT("Raptor prepares."));
		TestEqual(TEXT("the rags are still whole"), Battle.GetSquadDefence(), SquadStart);
		TestEqual(TEXT("the hide is still whole"), Battle.GetGroupDefence(), GroupStart);

		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the claws"), LogSummary(*this, Battle, 1), TEXT("Raptor attacks Captain with Claws."));
		TestEqual(TEXT("two lines while the squad retreats"), Battle.Log.Num(), 2);
		TestEqual(TEXT("the rags lose a quarter of the claws"), Battle.GetSquadDefence(), SquadStart - Claw * Kept);
		TestEqual(TEXT("the hide stays whole"), Battle.GetGroupDefence(), GroupStart);
	}

	{
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(nullptr);
		FSolidBattlePlan Retreat;
		Retreat.Name = TEXT("Retreat");
		Retreat.AttackOrRetreat = ESolidAggression::Retreat;
		Group.Available.Add(Retreat);
		Group.ActivePlan = 0;
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;

		const float Spear = Battle.SquadWeapon(0).Power;
		const float SquadStart = Battle.GetSquadDefence();
		const float GroupStart = Battle.GetGroupDefence();

		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the captain prepares"), LogSummary(*this, Battle, 0), TEXT("Captain prepares."));
		TestEqual(TEXT("the rags hold"), Battle.GetSquadDefence(), SquadStart);
		TestEqual(TEXT("the hide holds"), Battle.GetGroupDefence(), GroupStart);

		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("the spear"), LogSummary(*this, Battle, 1), TEXT("Captain attacks Raptor with Crude spear."));
		TestEqual(TEXT("two lines while the raptor retreats"), Battle.Log.Num(), 2);
		TestEqual(TEXT("the rags stay whole"), Battle.GetSquadDefence(), SquadStart);
		TestEqual(TEXT("the hide loses a quarter of the spear"), Battle.GetGroupDefence(), GroupStart - Spear * Kept);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleFinishedTest,
	"SolidCore1.Battle.Finished",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleFinishedTest::RunTest(const FString& Parameters)
{
	USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
	FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
	SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
	FSolidBattle Battle(Squad, &Group);
	Battle.Inventory = &Inventory;

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
	TestTrue(TEXT("the battle is finished"), Battle.Finished());
	const int32 Lines = Battle.Log.Num();
	const float When = Battle.Time;
	const float Rags = Battle.GetSquadDefence();

	Battle.AdvanceToNextEvent();
	Battle.Advance(1.f);
	TestEqual(TEXT("a finished battle adds no line"), Battle.Log.Num(), Lines);
	TestEqual(TEXT("a finished battle does not move time"), Battle.Time, When);
	TestEqual(TEXT("the hide stays at 0"), Battle.GetGroupDefence(), 0.f);
	TestEqual(TEXT("the rags stay where the last blow left them"), Battle.GetSquadDefence(), Rags);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleCaptainKilledTest,
	"SolidCore1.Battle.CaptainKilled",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleCaptainKilledTest::RunTest(const FString& Parameters)
{
	const int32 KilledIndex = SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters);
	if (KilledIndex == INDEX_NONE)
	{
		AddError(TEXT("Killed by monsters is not in the event list"));
		return false;
	}

	const int32 Hand = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);
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
		USolidGameState* State = NewObject<USolidGameState>();
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		Battle.State = State;

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
		TestFalse(TEXT("a dead raptor leaves Killed by monsters unfired"), State->bFired[KilledIndex]);
		TestFalse(TEXT("a dead raptor leaves the cave closed"), State->bStrongholdsEnabled);
		TestEqual(
			TEXT("a dead raptor leaves the spear"),
			FString(State->Inventory.Worn[Hand].Id()),
			FString(SolidItemType::IdCrudeSpear));
	}

	{
		USolidGameState* State = NewObject<USolidGameState>();
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		FSolidMonster* Raptor = Group.Get(0);
		if (!Raptor)
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		Raptor->Weapon.Power = FMath::CeilToInt(SolidItemType::RagsStrength);
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		Battle.State = State;

		int32 Guard = 0;
		while (Battle.GetSquadDefence() > 0.f)
		{
			Battle.AdvanceToNextEvent();
			if (++Guard > 40)
			{
				AddError(TEXT("the claws did not kill the captain"));
				return false;
			}
		}
		TestTrue(TEXT("the claws kill the captain"), HasLine(Battle, TEXT("Raptor kills Captain with Claws.")));
		TestTrue(TEXT("the captain's death fires Killed by monsters"), State->bFired[KilledIndex]);
		TestTrue(TEXT("the captain's death opens the cave"), State->bStrongholdsEnabled);
		TestEqual(TEXT("the journal records it once"), State->GetJournal()->Entries.Num(), 1);
		TestEqual(TEXT("that line is Killed by monsters"), State->GetJournal()->Entries[0].EventIndex, KilledIndex);
		TestTrue(TEXT("the play kit drops the spear"), State->Inventory.Worn[Hand].IsEmpty());
		TestEqual(
			TEXT("the play kit keeps rags"),
			FString(State->Inventory.Worn[Body].Id()),
			FString(SolidItemType::IdRags));
		TestEqual(TEXT("the battle rags are gone"), Battle.GetSquadDefence(), 0.f);

		const int32 Lines = Battle.Log.Num();
		Battle.AdvanceToNextEvent();
		TestEqual(TEXT("a later tick adds no line"), Battle.Log.Num(), Lines);
		TestEqual(TEXT("a later tick does not record it again"), State->GetJournal()->Entries.Num(), 1);
	}

	{
		USolidGameState* State = NewObject<USolidGameState>();
		USolidSquad* Squad = SolidBattleTestPrivate::BuildSquad(TEXT("Starter"));
		USolidCompany* Company = Squad->GetCompany();
		USolidCaptain* Captain = Company ? Company->GetCaptain() : nullptr;
		if (!Captain)
		{
			AddError(TEXT("the squad gave the battle no captain"));
			return false;
		}
		SolidEvents::MarkComplete(*State, *Captain, SolidEvents::IdKilledByMonsters);
		const int32 Recorded = State->GetJournal()->Entries.Num();

		FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
		FSolidMonster* Raptor = Group.Get(0);
		if (!Raptor)
		{
			AddError(TEXT("the one raptor gave the battle no monster"));
			return false;
		}
		Raptor->Weapon.Power = FMath::CeilToInt(SolidItemType::RagsStrength);
		SolidInventory::FLoadout Inventory = SolidInventory::StartingLoadout();
		FSolidBattle Battle(Squad, &Group);
		Battle.Inventory = &Inventory;
		Battle.State = State;

		int32 Guard = 0;
		while (Battle.GetSquadDefence() > 0.f)
		{
			Battle.AdvanceToNextEvent();
			if (++Guard > 40)
			{
				AddError(TEXT("the claws did not kill the captain"));
				return false;
			}
		}
		TestTrue(TEXT("the claws still kill the captain"), HasLine(Battle, TEXT("Raptor kills Captain with Claws.")));
		TestEqual(TEXT("the battle rags are gone again"), Battle.GetSquadDefence(), 0.f);
		TestEqual(TEXT("a death after the event adds no journal line"), State->GetJournal()->Entries.Num(), Recorded);
		TestEqual(TEXT("Killed by monsters stays one record"), State->FiredOrder.Num(), 1);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleDeathWakesTheCaptainTest,
	"SolidCore1.Battle.DeathWakesTheCaptain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleDeathWakesTheCaptainTest::RunTest(const FString& Parameters)
{
	const int32 KilledIndex = SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters);
	if (KilledIndex == INDEX_NONE)
	{
		AddError(TEXT("Killed by monsters is not in the event list"));
		return false;
	}

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	USolidGameState* State = GameMode->GetConfiguration();
	USolidSquad* Squad = GameMode->GetSquad();
	TestNotNull(TEXT("play state"), State);
	TestNotNull(TEXT("squad"), Squad);
	if (!State || !Squad)
	{
		return false;
	}
	TestFalse(TEXT("the death has not fired"), State->bFired[KilledIndex]);
	TestFalse(TEXT("the cave starts closed"), State->bStrongholdsEnabled);

	FSolidMonsterGroup Group = SolidBattleTestPrivate::OneRaptor(TEXT("Attack"));
	FSolidMonster* Raptor = Group.Get(0);
	if (!Raptor)
	{
		AddError(TEXT("the encounter has no raptor"));
		return false;
	}
	Raptor->Weapon.Power = FMath::CeilToInt(SolidItemType::RagsStrength);
	FSolidEncounter Encounter;
	Encounter.Groups.Add(MoveTemp(Group));
	GameMode->Encounters.Add(MoveTemp(Encounter));

	const int32 Hand = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);
	TestEqual(
		TEXT("the captain still has the spear"),
		FString(State->Inventory.Worn[Hand].Id()),
		FString(SolidItemType::IdCrudeSpear));

	int32 Guard = 0;
	while (!State->bFired[KilledIndex])
	{
		GameMode->AdvanceBattle(5.f, FVector::ZeroVector);
		if (++Guard > 20)
		{
			AddError(TEXT("the battle did not fire Killed by monsters"));
			return false;
		}
	}

	TestTrue(TEXT("the death opens the cave"), State->bStrongholdsEnabled);
	TestTrue(TEXT("the play kit drops the spear"), State->Inventory.Worn[Hand].IsEmpty());
	TestEqual(
		TEXT("the play kit keeps rags"),
		FString(State->Inventory.Worn[Body].Id()),
		FString(SolidItemType::IdRags));
	TestTrue(TEXT("Attack is on"), SolidEvents::IsBattlePlanEnabled(*State, TEXT("Attack")));
	TestTrue(TEXT("Defend is on"), SolidEvents::IsBattlePlanEnabled(*State, TEXT("Defend")));
	TestFalse(TEXT("Starter is off"), SolidEvents::IsBattlePlanEnabled(*State, TEXT("Starter")));
	const FSolidBattlePlan* Active = Squad->GetActiveBattlePlan();
	TestTrue(TEXT("the squad follows Attack"), Active && Active->Is(TEXT("Attack")));
	TestEqual(TEXT("the journal records it once"), State->GetJournal()->Entries.Num(), 1);

	const int32 Lines = State->GetJournal()->Entries.Num();
	GameMode->AdvanceBattle(5.f, FVector::ZeroVector);
	TestEqual(TEXT("a later tick does not record it again"), State->GetJournal()->Entries.Num(), Lines);
	TestEqual(TEXT("Killed by monsters stays one record"), State->FiredOrder.Num(), 1);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
