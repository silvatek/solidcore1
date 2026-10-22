#include "SolidBattle.h"
#include "Company/SolidCompany.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"

namespace
{
	/** Power of a companion's fixed weapon. An unknown id adds none. */
	int32 CompanionPower(const FSolidCompanion& Companion)
	{
		return FSolidItem::FromId(*Companion.WeaponId, 1).Power();
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

	/** One of these monsters, each of them equally likely. */
	FSolidMonster* PickMonster(const TArray<FSolidMonster*>& Monsters)
	{
		if (Monsters.Num() == 0)
		{
			return nullptr;
		}
		return Monsters[FMath::RandRange(0, Monsters.Num() - 1)];
	}

	void Land(FSolidArmour& Armour, const int32 Power)
	{
		Armour.Strength -= static_cast<float>(Power);
	}

	const TCHAR* ItemWeaponName(const FSolidItem& Item)
	{
		if (Item.Type && Item.Type->Name && Item.Type->Name[0])
		{
			return Item.Type->Name;
		}
		return Item.Weapon.Name ? Item.Weapon.Name : TEXT("");
	}

	FString MonsterName(const FSolidMonster* Monster)
	{
		const TCHAR* Kind = Monster ? Monster->Kind() : nullptr;
		return FString(Kind && Kind[0] ? Kind : TEXT("Monster"));
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

int32 FSolidBattle::GetSquadPower() const
{
	const USolidSquad* Side = Squad.Get();
	if (!Side)
	{
		return 0;
	}

	int32 Total = 0;
	if (const FSolidItem* MainHand = WornItem(Inventory, SolidInventory::ESlot::MainHand))
	{
		Total += MainHand->Power();
	}

	const USolidCompany* Company = Side->GetCompany();
	if (!Company)
	{
		return Total;
	}
	for (const int32 Index : Side->GetSelectedCompanionIndices())
	{
		const FSolidCompanion* Companion = Company->GetCompanion(Index);
		if (Companion)
		{
			Total += CompanionPower(*Companion);
		}
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
	for (const int32 Index : Side->GetSelectedCompanionIndices())
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
		const FSolidMonster* Monster = Group->Get(Index);
		if (Monster)
		{
			Total += Monster->Weapon.Power;
		}
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

FString FSolidBattleLogEntry::Summary() const
{
	switch (Kind)
	{
	case ESolidBattleLogKind::Attack:
		return FString::Printf(TEXT("%s attacks %s with %s."), *Actor, *Target, *Weapon);
	case ESolidBattleLogKind::Abandon:
		return FString::Printf(TEXT("%s abandons the attack."), *Actor);
	case ESolidBattleLogKind::Prepare:
	default:
		return FString::Printf(TEXT("%s prepares."), *Actor);
	}
}

void FSolidBattle::Record(
	const float When,
	const ESolidBattleLogKind Kind,
	const FString& Actor,
	const FString& Target,
	const FString& Weapon)
{
	FSolidBattleLogEntry Entry;
	Entry.Time = When;
	Entry.Kind = Kind;
	Entry.Actor = Actor;
	Entry.Target = Target;
	Entry.Weapon = Weapon;
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
	const FSolidItem* MainHand = WornItem(Inventory, SolidInventory::ESlot::MainHand);
	NoteCycle(MainHand && MainHand->IsWeapon(), &CaptainCycle, bSquadAttacks);

	if (const USolidSquad* Side = Squad.Get())
	{
		if (const USolidCompany* Company = Side->GetCompany())
		{
			for (const int32 Index : Side->GetSelectedCompanionIndices())
			{
				const FSolidCompanion* Companion = Company->GetCompanion(Index);
				if (!Companion)
				{
					continue;
				}
				const FSolidItem Weapon = FSolidItem::FromId(*Companion->WeaponId, 1);
				NoteCycle(Weapon.IsWeapon(), CompanionCycles.Find(Index), bSquadAttacks);
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

	if (const FSolidItem* MainHand = WornItem(Inventory, SolidInventory::ESlot::MainHand))
	{
		if (MainHand->IsWeapon())
		{
			const FString WeaponName(ItemWeaponName(*MainHand));
			const int32 Power = MainHand->Power();
			AdvanceCycle(
				CaptainCycle,
				MainHand->Weapon.Speed,
				Delta,
				bSquadAttacks,
				Now,
				Arrived,
				[&](const float When)
				{
					CaptainCycle.Target = PickMonster(Monsters);
					Record(When, ESolidBattleLogKind::Prepare, CaptainName, FString(), FString());
				},
				[&](const float When)
				{
					if (CaptainCycle.Target && Monsters.Contains(CaptainCycle.Target))
					{
						Land(CaptainCycle.Target->Armour, Power);
						Record(When, ESolidBattleLogKind::Attack, CaptainName, MonsterName(CaptainCycle.Target), WeaponName);
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
			for (const int32 Index : Side->GetSelectedCompanionIndices())
			{
				const FSolidCompanion* Companion = Company->GetCompanion(Index);
				if (!Companion)
				{
					continue;
				}
				const FSolidItem Weapon = FSolidItem::FromId(*Companion->WeaponId, 1);
				if (!Weapon.IsWeapon())
				{
					continue;
				}
				StillOut.Add(Index);
				FCycle& Cycle = CompanionCycles.FindOrAdd(Index);
				const FString Actor = Companion->Name;
				const FString WeaponName(ItemWeaponName(Weapon));
				const int32 Power = Weapon.Power();
				AdvanceCycle(
					Cycle,
					Weapon.Weapon.Speed,
					Delta,
					bSquadAttacks,
					Now,
					Arrived,
					[&](const float When)
					{
						Cycle.Target = PickMonster(Monsters);
						Record(When, ESolidBattleLogKind::Prepare, Actor, FString(), FString());
					},
					[&](const float When)
					{
						if (Cycle.Target && Monsters.Contains(Cycle.Target))
						{
							Land(Cycle.Target->Armour, Power);
							Record(When, ESolidBattleLogKind::Attack, Actor, MonsterName(Cycle.Target), WeaponName);
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

	for (FSolidMonster* Monster : Monsters)
	{
		FCycle& Cycle = MonsterCycles.FindOrAdd(Monster);
		const int32 Power = Monster->Weapon.Power;
		const FString Actor = MonsterName(Monster);
		const FString WeaponName(Monster->Weapon.Name ? Monster->Weapon.Name : TEXT(""));
		AdvanceCycle(
			Cycle,
			Monster->Weapon.Speed,
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
				Land(Body->Armour, Power);
				Record(When, ESolidBattleLogKind::Attack, Actor, CaptainName, WeaponName);
			},
			[&](const float When)
			{
				Record(When, ESolidBattleLogKind::Abandon, Actor, FString(), FString());
			});
	}
}
