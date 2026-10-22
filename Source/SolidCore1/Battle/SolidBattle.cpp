#include "SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Game/SolidGameState.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"

namespace
{
	/** True when that weapon can land a blow. An empty name cannot. */
	bool CanStrike(const FSolidWeapon& Weapon)
	{
		return Weapon.Name && Weapon.Name[0];
	}

	/** Armour Strength of a companion's fixed armour. Anything else adds none. */
	float CompanionDefence(const FSolidCompanion& Companion)
	{
		const FSolidItem Worn = FSolidItem::FromId(*Companion.ArmourId, 1);
		return Worn.IsArmour() ? Worn.Armour.Strength : 0.f;
	}

	const FSolidItem* WornItem(const SolidInventory::FLoadout* Inventory, const SolidInventory::ESlot Slot)
	{
		if (!Inventory)
		{
			return nullptr;
		}
		const int32 Index = SolidInventory::WornIndex(Slot);
		if (Index == INDEX_NONE)
		{
			return nullptr;
		}
		return &Inventory->Worn[Index];
	}

	FSolidItem* WornItem(SolidInventory::FLoadout* Inventory, const SolidInventory::ESlot Slot)
	{
		return const_cast<FSolidItem*>(WornItem(static_cast<const SolidInventory::FLoadout*>(Inventory), Slot));
	}

	void CollectMonsters(FSolidMonsterGroup* Group, TArray<FSolidMonster*>& Out)
	{
		Out.Reset();
		if (!Group)
		{
			return;
		}
		for (int32 Index = 0; Index < Group->Num(); ++Index)
		{
			if (FSolidMonster* Monster = Group->Get(Index))
			{
				Out.Add(Monster);
			}
		}
	}

	/** One living enemy, each of them equally likely. */
	FSolidMonster* PickMonster(const TArray<FSolidMonster*>& Monsters)
	{
		TArray<FSolidMonster*> Living;
		for (FSolidMonster* Monster : Monsters)
		{
			if (Monster && Monster->Armour.Strength > 0.f)
			{
				Living.Add(Monster);
			}
		}
		if (Living.Num() == 0)
		{
			return nullptr;
		}
		return Living[FMath::RandRange(0, Living.Num() - 1)];
	}

	/** Keeps that enemy while they can still be struck. Otherwise takes another living one. */
	void HoldTarget(FSolidMonster*& Target, const TArray<FSolidMonster*>& Monsters)
	{
		if (Target && Monsters.Contains(Target) && Target->Armour.Strength > 0.f)
		{
			return;
		}
		if (FSolidMonster* Next = PickMonster(Monsters))
		{
			Target = Next;
		}
	}

	/** Power that lands. A retreating target loses RetreatReduction of it. */
	float SpentPower(const int32 Power, const bool bRetreating)
	{
		const float Full = static_cast<float>(Power);
		if (!bRetreating)
		{
			return Full;
		}
		return Full * (1.f - FSolidBattle::RetreatReduction);
	}

	/** A blow comes off that armour. Strength stops at zero. True when that leaves it at zero. */
	bool Land(FSolidArmour& Armour, const float Power)
	{
		Armour.Strength = FMath::Max(0.f, Armour.Strength - Power);
		return Armour.Strength <= 0.f;
	}

	FString MonsterName(const FSolidMonster* Monster)
	{
		const TCHAR* Kind = Monster ? Monster->Kind() : nullptr;
		return FString(Kind && Kind[0] ? Kind : TEXT("Monster"));
	}

	/** Selected companions who fight. A cap walks that many boxes from the top. */
	void FightingCompanions(const USolidSquad* Side, const int32 FieldedCount, TArray<int32>& Out)
	{
		Out.Reset();
		if (!Side)
		{
			return;
		}
		const USolidCompany* Company = Side->GetCompany();
		if (!Company)
		{
			return;
		}
		const TArray<int32>& Selected = Side->GetSelectedCompanionIndices();
		const int32 Slots = FieldedCount == INDEX_NONE
			? Selected.Num()
			: FMath::Min(FMath::Max(FieldedCount, 0), Selected.Num());
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			if (Company->GetCompanion(Selected[Slot]))
			{
				Out.Add(Selected[Slot]);
			}
		}
	}
}

FSolidBattle::FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup)
	: Squad(InSquad)
	, Group(InGroup)
{
}

const FSolidBattlePlan* FSolidBattle::GetSquadPlan() const
{
	const USolidSquad* Side = Squad.Get();
	return Side ? Side->GetActiveBattlePlan() : nullptr;
}

const FSolidBattlePlan* FSolidBattle::GetGroupPlan() const
{
	return Group ? Group->GetActivePlan() : nullptr;
}

FSolidWeapon FSolidBattle::SquadWeapon(const int32 Index) const
{
	if (Index < 0)
	{
		return FSolidWeapon();
	}
	if (Index == 0)
	{
		const FSolidItem* MainHand = WornItem(Inventory, SolidInventory::ESlot::MainHand);
		return MainHand && MainHand->IsWeapon() ? MainHand->Weapon : FSolidWeapon();
	}

	const USolidSquad* Side = Squad.Get();
	const USolidCompany* Company = Side ? Side->GetCompany() : nullptr;
	if (!Company)
	{
		return FSolidWeapon();
	}
	TArray<int32> Fighting;
	FightingCompanions(Side, FieldedCount, Fighting);
	if (!Fighting.IsValidIndex(Index - 1))
	{
		return FSolidWeapon();
	}
	const FSolidCompanion* Companion = Company->GetCompanion(Fighting[Index - 1]);
	if (!Companion)
	{
		return FSolidWeapon();
	}
	const FSolidItem Weapon = FSolidItem::FromId(*Companion->WeaponId, 1);
	return Weapon.IsWeapon() ? Weapon.Weapon : FSolidWeapon();
}

FSolidWeapon FSolidBattle::MonsterWeapon(const int32 Index) const
{
	if (!Group || Index < 0)
	{
		return FSolidWeapon();
	}
	const FSolidMonster* Monster = Group->Get(Index);
	return Monster ? Monster->Weapon : FSolidWeapon();
}

int32 FSolidBattle::GetSquadPower() const
{
	int32 Total = SquadWeapon(0).Power;
	const USolidSquad* Side = Squad.Get();
	if (!Side)
	{
		return Total;
	}
	TArray<int32> Fighting;
	FightingCompanions(Side, FieldedCount, Fighting);
	for (int32 Member = 0; Member < Fighting.Num(); ++Member)
	{
		Total += SquadWeapon(Member + 1).Power;
	}
	return Total;
}

float FSolidBattle::GetSquadDefence() const
{
	const USolidSquad* Side = Squad.Get();
	if (!Side)
	{
		return 0.f;
	}

	float Total = 0.f;
	if (const FSolidItem* Body = WornItem(Inventory, SolidInventory::ESlot::Body))
	{
		if (Body->IsArmour())
		{
			Total += Body->Armour.Strength;
		}
	}

	const USolidCompany* Company = Side->GetCompany();
	if (!Company)
	{
		return Total;
	}
	TArray<int32> Fighting;
	FightingCompanions(Side, FieldedCount, Fighting);
	for (const int32 Index : Fighting)
	{
		const FSolidCompanion* Companion = Company->GetCompanion(Index);
		if (Companion)
		{
			Total += CompanionDefence(*Companion);
		}
	}
	return Total;
}

int32 FSolidBattle::GetGroupPower() const
{
	if (!Group)
	{
		return 0;
	}

	int32 Total = 0;
	for (int32 Index = 0; Index < Group->Num(); ++Index)
	{
		Total += MonsterWeapon(Index).Power;
	}
	return Total;
}

float FSolidBattle::GetGroupDefence() const
{
	if (!Group)
	{
		return 0.f;
	}

	float Total = 0.f;
	for (int32 Index = 0; Index < Group->Num(); ++Index)
	{
		const FSolidMonster* Monster = Group->Get(Index);
		if (Monster)
		{
			Total += Monster->Armour.Strength;
		}
	}
	return Total;
}

bool FSolidBattle::Finished() const
{
	return GetSquadDefence() <= 0.f || GetGroupDefence() <= 0.f;
}

FString FSolidBattleLogEntry::Summary() const
{
	switch (Kind)
	{
	case ESolidBattleLogKind::Attack:
		return FString::Printf(
			TEXT("%s %s %s with %s."),
			*Actor,
			bKilled ? TEXT("kills") : TEXT("attacks"),
			*Target,
			*Weapon);
	case ESolidBattleLogKind::Abandon:
		return FString::Printf(TEXT("%s abandons the attack."), *Actor);
	case ESolidBattleLogKind::Abort:
		return FString::Printf(TEXT("%s aborts the attack on %s."), *Actor, *Target);
	case ESolidBattleLogKind::Prepare:
	default:
		return FString::Printf(TEXT("%s prepares."), *Actor);
	}
}

void FSolidBattle::NoteCaptainKilled() const
{
	USolidGameState* Play = State;
	const USolidSquad* Side = Squad.Get();
	USolidCompany* Company = Side ? Side->GetCompany() : nullptr;
	USolidCaptain* Captain = Company ? Company->GetCaptain() : nullptr;
	if (!Play || !Captain)
	{
		return;
	}
	SolidEvents::NotifyCaptainKilled(*Play, *Captain);
}

void FSolidBattle::Record(
	const float When,
	const ESolidBattleLogKind Kind,
	const FString& Actor,
	const FString& Target,
	const FString& Weapon,
	const bool bKilled)
{
	FSolidBattleLogEntry Entry;
	Entry.Time = When;
	Entry.Kind = Kind;
	Entry.Actor = Actor;
	Entry.Target = Target;
	Entry.Weapon = Weapon;
	Entry.bKilled = bKilled;
	Log.Add(Entry);
}

void FSolidBattle::AdvanceCycle(
	FCycle& Cycle,
	const float Speed,
	const float Delta,
	const bool bAttacking,
	const float Now,
	const float Arrived,
	const TFunctionRef<void(float When)> BeginPrepare,
	const TFunctionRef<void(float When)> LandBlow,
	const TFunctionRef<void(float When)> Abandon)
{
	if (!FMath::IsFinite(Speed) || Speed <= KINDA_SMALL_NUMBER)
	{
		Cycle.bEngaged = false;
		return;
	}
	if (!Cycle.bEngaged)
	{
		if (!bAttacking)
		{
			return;
		}
		Cycle.bEngaged = true;
		Cycle.Step = FCycle::EStep::Prepare;
		Cycle.TimeLeft = Speed;
		BeginPrepare(Now);
	}

	Cycle.TimeLeft -= Delta;
	while (Cycle.TimeLeft <= 0.f)
	{
		const float Overshoot = -Cycle.TimeLeft;
		if (Cycle.Step == FCycle::EStep::Prepare)
		{
			if (!bAttacking)
			{
				Cycle.bEngaged = false;
				Abandon(Arrived);
				return;
			}
			LandBlow(Arrived);
			Cycle.Step = FCycle::EStep::Recover;
		}
		else if (!bAttacking)
		{
			Cycle.bEngaged = false;
			return;
		}
		else
		{
			Cycle.Step = FCycle::EStep::Prepare;
			BeginPrepare(Arrived);
		}
		Cycle.TimeLeft = Speed - Overshoot;
	}
}

float FSolidBattle::TimeUntilNextEvent()
{
	if (Finished())
	{
		return -1.f;
	}

	float Best = -1.f;
	auto Note = [&Best](const float Wait)
	{
		if (Wait < 0.f)
		{
			return;
		}
		if (Best < 0.f || Wait < Best)
		{
			Best = Wait;
		}
	};
	auto NoteCycle = [&Note](const bool bCanFight, const FCycle* Cycle, const bool bAttacking)
	{
		if (!bCanFight)
		{
			return;
		}
		if (!Cycle || !Cycle->bEngaged)
		{
			if (bAttacking)
			{
				Note(0.f);
			}
			return;
		}
		Note(FMath::Max(0.f, Cycle->TimeLeft));
	};

	const bool bSquadAttacks = GetSquadPlan() && GetSquadPlan()->Attacks();
	const bool bGroupAttacks = GetGroupPlan() && GetGroupPlan()->Attacks();
	NoteCycle(CanStrike(SquadWeapon(0)), &CaptainCycle, bSquadAttacks);

	if (const USolidSquad* Side = Squad.Get())
	{
		if (Side->GetCompany())
		{
			TArray<int32> Fighting;
			FightingCompanions(Side, FieldedCount, Fighting);
			for (int32 Member = 0; Member < Fighting.Num(); ++Member)
			{
				NoteCycle(CanStrike(SquadWeapon(Member + 1)), CompanionCycles.Find(Fighting[Member]), bSquadAttacks);
			}
		}
	}

	const FSolidItem* Body = WornItem(Inventory, SolidInventory::ESlot::Body);
	const bool bBody = Body && Body->IsArmour();
	if (Group && bBody)
	{
		for (int32 Index = 0; Index < Group->Num(); ++Index)
		{
			FSolidMonster* Monster = Group->Get(Index);
			if (!Monster)
			{
				continue;
			}
			NoteCycle(true, MonsterCycles.Find(Monster), bGroupAttacks);
		}
	}
	return Best;
}

bool FSolidBattle::AdvanceToNextEvent()
{
	const float Wait = TimeUntilNextEvent();
	if (Wait < 0.f)
	{
		return false;
	}
	Advance(Wait);
	return true;
}

void FSolidBattle::Advance(const float Delta)
{
	if (Finished())
	{
		return;
	}
	if (!FMath::IsFinite(Delta) || Delta < 0.f)
	{
		return;
	}

	const float Now = Time;
	Time += Delta;
	const float Arrived = Time;

	TArray<FSolidMonster*> Monsters;
	CollectMonsters(Group, Monsters);
	for (auto It = MonsterCycles.CreateIterator(); It; ++It)
	{
		if (!Monsters.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	const bool bSquadAttacks = GetSquadPlan() && GetSquadPlan()->Attacks();
	const bool bGroupAttacks = GetGroupPlan() && GetGroupPlan()->Attacks();
	const FString CaptainName(TEXT("Captain"));

	/** A blow on armour already at 0 does not land. True when this blow leaves the armour at 0. */
	auto Strike = [&](
		FSolidArmour& Armour,
		const int32 Power,
		const bool bRetreating,
		const float When,
		const FString& Actor,
		const FString& TargetName,
		const FString& Weapon)
	{
		if (Armour.Strength <= 0.f)
		{
			Record(When, ESolidBattleLogKind::Abort, Actor, TargetName, FString());
			return false;
		}
		const bool bKilled = Land(Armour, SpentPower(Power, bRetreating));
		Record(When, ESolidBattleLogKind::Attack, Actor, TargetName, Weapon, bKilled);
		return bKilled;
	};

	const FSolidWeapon CaptainWeapon = SquadWeapon(0);
	if (CanStrike(CaptainWeapon))
	{
		const FString WeaponName(CaptainWeapon.Name);
		const int32 Power = CaptainWeapon.Power;
		AdvanceCycle(
			CaptainCycle,
			CaptainWeapon.Speed,
			Delta,
			bSquadAttacks,
			Now,
			Arrived,
			[&](const float When)
			{
				HoldTarget(CaptainCycle.Target, Monsters);
				Record(When, ESolidBattleLogKind::Prepare, CaptainName, FString(), FString());
			},
			[&](const float When)
			{
					if (CaptainCycle.Target && Monsters.Contains(CaptainCycle.Target))
					{
						const bool bRetreats = GetGroupPlan() && GetGroupPlan()->Retreats();
						Strike(CaptainCycle.Target->Armour, Power, bRetreats, When, CaptainName, MonsterName(CaptainCycle.Target), WeaponName);
					}
			},
			[&](const float When)
			{
				Record(When, ESolidBattleLogKind::Abandon, CaptainName, FString(), FString());
			});
	}
	else
	{
		CaptainCycle.bEngaged = false;
	}

	TSet<int32> StillOut;
	if (const USolidSquad* Side = Squad.Get())
	{
		if (const USolidCompany* Company = Side->GetCompany())
		{
			TArray<int32> Fighting;
			FightingCompanions(Side, FieldedCount, Fighting);
			for (int32 Member = 0; Member < Fighting.Num(); ++Member)
			{
				const int32 Index = Fighting[Member];
				const FSolidCompanion* Companion = Company->GetCompanion(Index);
				if (!Companion)
				{
					continue;
				}
				const FSolidWeapon Weapon = SquadWeapon(Member + 1);
				if (!CanStrike(Weapon))
				{
					continue;
				}
				StillOut.Add(Index);
				FCycle& Cycle = CompanionCycles.FindOrAdd(Index);
				const FString Actor = Companion->Name;
				const FString WeaponName(Weapon.Name);
				const int32 Power = Weapon.Power;
				AdvanceCycle(
					Cycle,
					Weapon.Speed,
					Delta,
					bSquadAttacks,
					Now,
					Arrived,
					[&](const float When)
					{
						HoldTarget(Cycle.Target, Monsters);
						Record(When, ESolidBattleLogKind::Prepare, Actor, FString(), FString());
					},
					[&](const float When)
					{
						if (Cycle.Target && Monsters.Contains(Cycle.Target))
						{
							const bool bRetreats = GetGroupPlan() && GetGroupPlan()->Retreats();
							Strike(Cycle.Target->Armour, Power, bRetreats, When, Actor, MonsterName(Cycle.Target), WeaponName);
						}
					},
					[&](const float When)
					{
						Record(When, ESolidBattleLogKind::Abandon, Actor, FString(), FString());
					});
			}
		}
	}
	for (auto It = CompanionCycles.CreateIterator(); It; ++It)
	{
		if (!StillOut.Contains(It.Key()))
		{
			It.RemoveCurrent();
		}
	}

	FSolidItem* Body = WornItem(Inventory, SolidInventory::ESlot::Body);
	if (!Body || !Body->IsArmour())
	{
		for (FSolidMonster* Monster : Monsters)
		{
			if (FCycle* Cycle = MonsterCycles.Find(Monster))
			{
				Cycle->bEngaged = false;
			}
		}
		return;
	}

	bool bCaptainKilled = false;
	for (int32 Index = 0; Group && Index < Group->Num(); ++Index)
	{
		FSolidMonster* Monster = Group->Get(Index);
		if (!Monster)
		{
			continue;
		}
		FCycle& Cycle = MonsterCycles.FindOrAdd(Monster);
		const FSolidWeapon Weapon = MonsterWeapon(Index);
		const int32 Power = Weapon.Power;
		const FString Actor = MonsterName(Monster);
		const FString WeaponName(Weapon.Name ? Weapon.Name : TEXT(""));
		AdvanceCycle(
			Cycle,
			Weapon.Speed,
			Delta,
			bGroupAttacks,
			Now,
			Arrived,
			[&](const float When)
			{
				Record(When, ESolidBattleLogKind::Prepare, Actor, FString(), FString());
			},
			[&](const float When)
			{
				const bool bRetreats = GetSquadPlan() && GetSquadPlan()->Retreats();
				if (Strike(Body->Armour, Power, bRetreats, When, Actor, CaptainName, WeaponName))
				{
					bCaptainKilled = true;
				}
			},
			[&](const float When)
			{
				Record(When, ESolidBattleLogKind::Abandon, Actor, FString(), FString());
			});
	}
	if (bCaptainKilled)
	{
		NoteCaptainKilled();
	}
}
