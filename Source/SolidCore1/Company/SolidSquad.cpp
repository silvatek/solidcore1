#include "Company/SolidSquad.h"
#include "Company/SolidCompany.h"
#include "SolidGameState.h"

void USolidSquad::InitializeFromCompany(USolidCompany* InCompany)
{
	Company = InCompany;
	if (!Company)
	{
		AssignedCompanyPlanIndices.Reset();
		ActiveAssignedSlot = 0;
		HeldFormation = ESolidBattleFormation::Line;
		HeldSpacing = ESolidBattleSpacing::Standard;
		SelectedCompanionIndices.Reset();
		return;
	}

	Company->InitializeDefaultBattlePlans();
	Company->InitializeDefaultCompanions();

	TArray<int32> Starter;
	for (int32 Index = 0; Index < Company->GetBattlePlanCount()
		&& Starter.Num() < SolidBattleFormationSlots::MaxAssignedBattlePlans;
		++Index)
	{
		Starter.Add(Index);
	}
	SetAssignedBattlePlans(Starter);

	// Default active: F1. The play state then keeps the enabled plans.
	ActiveAssignedSlot = 0;
	RememberHeldFormation();

	TArray<int32> StarterCompanions;
	const int32 Sam = Company->FindCompanionIndexByName(TEXT("Sam"));
	const int32 Alex = Company->FindCompanionIndexByName(TEXT("Alex"));
	if (Sam != INDEX_NONE)
	{
		StarterCompanions.Add(Sam);
	}
	if (Alex != INDEX_NONE)
	{
		StarterCompanions.Add(Alex);
	}
	SetSelectedCompanions(StarterCompanions);
}

void USolidSquad::SyncEnabledBattlePlans(USolidGameState& State)
{
	if (!Company)
	{
		return;
	}
	SolidEvents::SeedBattlePlans(State, *Company);

	TArray<int32> Enabled;
	for (int32 Index = 0; Index < Company->GetBattlePlanCount(); ++Index)
	{
		const FSolidBattlePlan* Plan = Company->GetBattlePlan(Index);
		if (Plan && SolidEvents::IsBattlePlanEnabled(State, *Plan->Name))
		{
			Enabled.Add(Index);
		}
	}
	if (Enabled.Num() >= SolidBattleFormationSlots::MaxAssignedBattlePlans)
	{
		return;
	}

	const FSolidBattlePlan* Active = GetActiveBattlePlan();
	const FString ActiveName = Active ? Active->Name : FString();
	const bool bHadActive = Active != nullptr;
	SetAssignedBattlePlans(Enabled);
	if (!bHadActive)
	{
		return;
	}

	int32 Keep = INDEX_NONE;
	for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
	{
		const FSolidBattlePlan* Plan = GetAssignedBattlePlan(Slot);
		if (Plan && Plan->Name == ActiveName)
		{
			Keep = Slot;
			break;
		}
	}
	if (Keep == INDEX_NONE)
	{
		for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
		{
			if (GetAssignedBattlePlan(Slot))
			{
				Keep = Slot;
				break;
			}
		}
	}
	if (Keep != INDEX_NONE)
	{
		SelectAssignedSlot(Keep);
	}
}

int32 USolidSquad::GetAssignedCount() const
{
	int32 Count = 0;
	for (const int32 CompanyIndex : AssignedCompanyPlanIndices)
	{
		if (CompanyIndex != INDEX_NONE)
		{
			++Count;
		}
	}
	return Count;
}

bool USolidSquad::SetAssignedBattlePlans(const TArray<int32>& CompanyPlanIndices)
{
	if (!Company)
	{
		AssignedCompanyPlanIndices.Reset();
		ActiveAssignedSlot = 0;
		return false;
	}

	TArray<int32> Next;
	Next.Init(INDEX_NONE, SolidBattleFormationSlots::MaxAssignedBattlePlans);
	int32 Write = 0;
	for (const int32 CompanyIndex : CompanyPlanIndices)
	{
		if (Write >= SolidBattleFormationSlots::MaxAssignedBattlePlans)
		{
			break;
		}
		if (CompanyIndex == INDEX_NONE)
		{
			Next[Write++] = INDEX_NONE;
			continue;
		}
		if (!Company->GetBattlePlan(CompanyIndex))
		{
			continue;
		}
		bool bAlready = false;
		for (int32 Slot = 0; Slot < Write; ++Slot)
		{
			if (Next[Slot] == CompanyIndex)
			{
				bAlready = true;
				break;
			}
		}
		if (bAlready)
		{
			continue;
		}
		Next[Write++] = CompanyIndex;
	}

	AssignedCompanyPlanIndices = Next;

	int32 Filled = 0;
	int32 LastFilled = INDEX_NONE;
	for (int32 Slot = 0; Slot < Next.Num(); ++Slot)
	{
		if (Next[Slot] != INDEX_NONE)
		{
			++Filled;
			LastFilled = Slot;
		}
	}
	if (Filled == 0)
	{
		ActiveAssignedSlot = 0;
		return false;
	}
	if (ActiveAssignedSlot != INDEX_NONE && !GetAssignedBattlePlan(ActiveAssignedSlot))
	{
		ActiveAssignedSlot = LastFilled;
	}
	RememberHeldFormation();
	return true;
}

bool USolidSquad::SelectAssignedSlot(const int32 SlotIndex)
{
	if (!GetAssignedBattlePlan(SlotIndex))
	{
		return false;
	}
	ActiveAssignedSlot = SlotIndex;
	RememberHeldFormation();
	return true;
}

void USolidSquad::ClearActiveBattlePlan()
{
	ActiveAssignedSlot = INDEX_NONE;
}

void USolidSquad::SetHeldFormation(
	const ESolidBattleFormation Formation,
	const ESolidBattleSpacing Spacing)
{
	HeldFormation = Formation;
	HeldSpacing = Spacing;
}

void USolidSquad::RememberHeldFormation()
{
	if (const FSolidBattlePlan* Plan = GetActiveBattlePlan())
	{
		HeldFormation = Plan->Formation;
		HeldSpacing = Plan->Spacing;
	}
}

const FSolidBattlePlan* USolidSquad::GetAssignedBattlePlan(const int32 SlotIndex) const
{
	if (!Company || !AssignedCompanyPlanIndices.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	return Company->GetBattlePlan(AssignedCompanyPlanIndices[SlotIndex]);
}

const FSolidBattlePlan* USolidSquad::GetActiveBattlePlan() const
{
	return GetAssignedBattlePlan(ActiveAssignedSlot);
}

ESolidBattleFormation USolidSquad::GetActiveFormation() const
{
	if (const FSolidBattlePlan* Plan = GetActiveBattlePlan())
	{
		return Plan->Formation;
	}
	return HeldFormation;
}

ESolidBattleSpacing USolidSquad::GetActiveSpacing() const
{
	if (const FSolidBattlePlan* Plan = GetActiveBattlePlan())
	{
		return Plan->Spacing;
	}
	return HeldSpacing;
}

FString USolidSquad::GetActiveBattlePlanDebugString() const
{
	const FSolidBattlePlan* Plan = GetActiveBattlePlan();
	if (!Plan)
	{
		return TEXT("BattlePlan <none>");
	}
	return FString::Printf(
		TEXT("BattlePlan F%d/%d %s (%s, %s)"),
		ActiveAssignedSlot + 1,
		GetAssignedCount(),
		*Plan->Name,
		SolidBattleFormationSlots::FormationName(Plan->Formation),
		SolidBattleFormationSlots::SpacingName(Plan->Spacing));
}

int32 USolidSquad::GetSelectedCompanionCount() const
{
	int32 Count = 0;
	for (const int32 CompanyIndex : SelectedCompanionIndices)
	{
		if (CompanyIndex != INDEX_NONE)
		{
			++Count;
		}
	}
	return Count;
}

bool USolidSquad::SetSelectedCompanions(const TArray<int32>& CompanyCompanionIndices)
{
	SelectedCompanionIndices.Reset();
	if (!Company)
	{
		return false;
	}

	for (const int32 CompanyIndex : CompanyCompanionIndices)
	{
		if (CompanyIndex == INDEX_NONE)
		{
			SelectedCompanionIndices.Add(INDEX_NONE);
			continue;
		}
		if (!Company->GetCompanion(CompanyIndex))
		{
			continue;
		}
		if (SelectedCompanionIndices.Contains(CompanyIndex))
		{
			continue;
		}
		SelectedCompanionIndices.Add(CompanyIndex);
	}

	while (SelectedCompanionIndices.Num() > 0 && SelectedCompanionIndices.Last() == INDEX_NONE)
	{
		SelectedCompanionIndices.Pop();
	}
	return true;
}

void USolidSquad::CollectFieldedCompanions(const int32 MaxCount, TArray<FSolidCompanion>& Out) const
{
	Out.Reset();
	if (!Company || MaxCount <= 0)
	{
		return;
	}

	const int32 Slots = FMath::Min(MaxCount, SelectedCompanionIndices.Num());
	for (int32 Slot = 0; Slot < Slots; ++Slot)
	{
		const FSolidCompanion* Companion = Company->GetCompanion(SelectedCompanionIndices[Slot]);
		if (!Companion)
		{
			continue;
		}
		Out.Add(*Companion);
	}
}
