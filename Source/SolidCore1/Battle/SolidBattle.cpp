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

void FSolidBattle::Attack()
{
	TArray<FSolidMonster*> Monsters;
	CollectMonsters(Group, Monsters);

	if (const FSolidBattlePlan* Plan = GetSquadPlan())
	{
		if (Plan->Attacks())
		{
			if (const FSolidItem* MainHand = WornItem(Inventory, SolidInventory::ESlot::MainHand))
			{
				if (MainHand->IsWeapon())
				{
					if (FSolidMonster* Target = PickMonster(Monsters))
					{
						Land(Target->Armour, MainHand->Power());
					}
				}
			}

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
						if (FSolidMonster* Target = PickMonster(Monsters))
						{
							Land(Target->Armour, Weapon.Power());
						}
					}
				}
			}
		}
	}

	if (const FSolidBattlePlan* Plan = GetGroupPlan())
	{
		if (!Plan->Attacks())
		{
			return;
		}
		FSolidItem* Body = WornItem(Inventory, SolidInventory::ESlot::Body);
		if (!Body || !Body->IsArmour())
		{
			return;
		}
		for (FSolidMonster* Monster : Monsters)
		{
			Land(Body->Armour, Monster->Weapon.Power);
		}
	}
}
