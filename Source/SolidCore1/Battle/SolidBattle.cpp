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
