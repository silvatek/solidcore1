#include "Party/SolidParty.h"
#include "Party/SolidCompany.h"

void USolidParty::InitializeFromCompany(USolidCompany* InCompany)
{
	Company = InCompany;
	if (!Company)
	{
		AssignedCompanyPlanIndices.Reset();
		ActiveAssignedSlot = 0;
		return;
	}

	Company->InitializeDefaultBattlePlans();

	TArray<int32> Starter;
	for (int32 Index = 0; Index < Company->GetBattlePlanCount()
		&& Starter.Num() < SolidBattleFormationSlots::MaxAssignedBattlePlans;
		++Index)
	{
		Starter.Add(Index);
	}
	SetAssignedBattlePlans(Starter);

	// Default active: F1 (first assigned slot — Line in the starter catalog).
	ActiveAssignedSlot = 0;
}

bool USolidParty::SetAssignedBattlePlans(const TArray<int32>& CompanyPlanIndices)
{
	AssignedCompanyPlanIndices.Reset();
	if (!Company)
	{
		ActiveAssignedSlot = 0;
		return false;
	}

	for (const int32 CompanyIndex : CompanyPlanIndices)
	{
		if (!Company->GetBattlePlan(CompanyIndex))
		{
			continue;
		}
		AssignedCompanyPlanIndices.AddUnique(CompanyIndex);
		if (AssignedCompanyPlanIndices.Num() >= SolidBattleFormationSlots::MaxAssignedBattlePlans)
		{
			break;
		}
	}

	if (AssignedCompanyPlanIndices.Num() == 0)
	{
		ActiveAssignedSlot = 0;
		return false;
	}

	ActiveAssignedSlot = FMath::Clamp(ActiveAssignedSlot, 0, AssignedCompanyPlanIndices.Num() - 1);
	return true;
}

bool USolidParty::SelectAssignedSlot(const int32 SlotIndex)
{
	if (!AssignedCompanyPlanIndices.IsValidIndex(SlotIndex))
	{
		return false;
	}
	ActiveAssignedSlot = SlotIndex;
	return true;
}

const FSolidBattlePlan* USolidParty::GetAssignedBattlePlan(const int32 SlotIndex) const
{
	if (!Company || !AssignedCompanyPlanIndices.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	return Company->GetBattlePlan(AssignedCompanyPlanIndices[SlotIndex]);
}

const FSolidBattlePlan* USolidParty::GetActiveBattlePlan() const
{
	return GetAssignedBattlePlan(ActiveAssignedSlot);
}

ESolidBattleFormation USolidParty::GetActiveFormation() const
{
	if (const FSolidBattlePlan* Plan = GetActiveBattlePlan())
	{
		return Plan->Formation;
	}
	return ESolidBattleFormation::Line;
}

ESolidBattleSpacing USolidParty::GetActiveSpacing() const
{
	if (const FSolidBattlePlan* Plan = GetActiveBattlePlan())
	{
		return Plan->Spacing;
	}
	return ESolidBattleSpacing::Standard;
}

FString USolidParty::GetActiveBattlePlanDebugString() const
{
	const FSolidBattlePlan* Plan = GetActiveBattlePlan();
	if (!Plan)
	{
		return TEXT("BattlePlan <none>");
	}
	return FString::Printf(
		TEXT("BattlePlan F%d/%d %s (%s, %s)"),
		ActiveAssignedSlot + 1,
		AssignedCompanyPlanIndices.Num(),
		*Plan->Name,
		SolidBattleFormationSlots::FormationName(Plan->Formation),
		SolidBattleFormationSlots::SpacingName(Plan->Spacing));
}
