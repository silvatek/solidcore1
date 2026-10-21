#include "SolidGameState.h"
#include "Company/SolidBattlePlan.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"

namespace
{
	bool RequirementsMet(const USolidGameState& State, const SolidEvents::FEvent& Event)
	{
		const int32 Count = FMath::Min(Event.RequireCount, SolidEvents::MaxRequiresPerEvent);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Required = SolidEvents::FindIndexById(Event.Requires[Index]);
			if (Required == INDEX_NONE || Required >= SolidEvents::MaxEvents || !State.bFired[Required])
			{
				return false;
			}
		}
		return true;
	}

	bool TryFireEvent(
		USolidGameState& State,
		USolidCaptain& Captain,
		const int32 Index,
		const SolidWorldTime::FStamp& When,
		SolidEvents::FResult& Result,
		const bool bForce = false)
	{
		if (Index < 0 || Index >= SolidEvents::DefinitionCount || Index >= SolidEvents::MaxEvents)
		{
			return false;
		}
		if (State.bFired[Index] || (!bForce && !RequirementsMet(State, SolidEvents::Definitions[Index])))
		{
			return false;
		}

		const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
		State.bFired[Index] = true;
		if (State.FiredOrder.Num() < SolidEvents::MaxEvents)
		{
			State.FiredOrder.Add(Index);
		}
		State.GetJournal()->Remember(When, EJournalType::Event, FString(), Index);
		Result.bFired = true;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
		{
			SolidEvents::ApplyChange(State, Captain, Event.Changes[ChangeIndex], Result);
		}
		return true;
	}
}

USolidJournal* USolidGameState::GetJournal()
{
	if (!Journal)
	{
		Journal = NewObject<USolidJournal>(this, TEXT("Journal"));
	}
	return Journal;
}

namespace SolidEvents
{

void ApplyChange(
	USolidGameState& State,
	USolidCaptain& Captain,
	const FChange& Change,
	FResult& Result)
{
	switch (Change.Kind)
	{
	case EChange::EnableSight:
		if (Change.Sight == ESolidSight::Raven)
		{
			Captain.bRavenSightEnabled = true;
		}
		else
		{
			Captain.bTrueSightEnabled = true;
		}
		Result.bSetSight = true;
		Result.Sight = Change.Sight;
		break;
	case EChange::MaxSquadSize:
		State.MaxSquadSize = FMath::Max(0, Change.SquadSize);
		Result.MaxSquadSize = State.MaxSquadSize;
		break;
	case EChange::CaptainTitle:
		Captain.Title = Change.Title;
		Result.bSetTitle = true;
		Result.CaptainTitle = Change.Title;
		break;
	case EChange::RepairBridge:
		State.bIglinBridgeRepaired = true;
		Result.bRepairBridge = true;
		break;
	case EChange::EnableCave:
		State.bStrongholdsEnabled = true;
		Result.bEnterStronghold = true;
		break;
	case EChange::ShowMessage:
		Result.bShowMessage = true;
		Result.MessageId = Change.MessageId;
		break;
	case EChange::ClearInventory:
		SolidInventory::Clear(State.Inventory);
		break;
	case EChange::EnableStrongholdSize:
	{
		State.bStrongholdsEnabled = true;
		const int32 Index = static_cast<int32>(Change.StrongholdSize);
		if (Index > static_cast<int32>(State.MaxStrongholdSize) && Index < SolidStronghold::SizeCount)
		{
			State.MaxStrongholdSize = Change.StrongholdSize;
		}
		break;
	}
	case EChange::EnableLair:
		if (Change.LairId && Change.LairId[0] && !IsLairEnabled(State, Change.LairId))
		{
			if (State.EnabledLairs.Num() < MaxEnabledLairs)
			{
				State.EnabledLairs.Add(Change.LairId);
			}
		}
		break;
	case EChange::EnableBattlePlan:
	case EChange::DisableBattlePlan:
	{
		USolidCompany* Company = Cast<USolidCompany>(Captain.GetOuter());
		if (!Company || !Change.BattlePlanName || !Change.BattlePlanName[0])
		{
			break;
		}
		SeedBattlePlans(State, *Company);
		if (Company->FindBattlePlanIndexByName(Change.BattlePlanName) == INDEX_NONE)
		{
			break;
		}
		const int32 Found = State.EnabledBattlePlans.IndexOfByKey(FString(Change.BattlePlanName));
		if (Change.Kind == EChange::EnableBattlePlan)
		{
			if (Found == INDEX_NONE)
			{
				State.EnabledBattlePlans.Add(Change.BattlePlanName);
			}
		}
		else if (Found != INDEX_NONE)
		{
			State.EnabledBattlePlans.RemoveAt(Found);
		}
		break;
	}
	default:
		break;
	}
}

void SeedBattlePlans(USolidGameState& State, const USolidCompany& Company)
{
	if (State.bBattlePlansSeeded || Company.GetBattlePlanCount() <= 0)
	{
		return;
	}
	State.bBattlePlansSeeded = true;
	State.EnabledBattlePlans.Reset();
	if (Company.FindBattlePlanIndexByName(TEXT("Starter")) != INDEX_NONE)
	{
		State.EnabledBattlePlans.Add(TEXT("Starter"));
	}
}

bool IsBattlePlanEnabled(const USolidGameState& State, const TCHAR* Name)
{
	return Name && Name[0] && State.EnabledBattlePlans.Contains(FString(Name));
}

namespace
{
	int32 EnabledInCatalog(const USolidGameState& State, const USolidCompany& Company)
	{
		int32 Count = 0;
		for (int32 Index = 0; Index < Company.GetBattlePlanCount(); ++Index)
		{
			const FSolidBattlePlan* Plan = Company.GetBattlePlan(Index);
			if (Plan && IsBattlePlanEnabled(State, *Plan->Name))
			{
				++Count;
			}
		}
		return Count;
	}

	int32 SlotForPlanName(const USolidSquad& Squad, const FString& Name)
	{
		if (Name.IsEmpty())
		{
			return INDEX_NONE;
		}
		for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
		{
			const FSolidBattlePlan* Plan = Squad.GetAssignedBattlePlan(Slot);
			if (Plan && Plan->Name == Name)
			{
				return Slot;
			}
		}
		return INDEX_NONE;
	}
}

void RestoreAssignedBattlePlans(
	USolidSquad& Squad,
	USolidGameState& State,
	const TArray<int32>& AssignedIndices,
	const int32 ActiveSlot,
	const bool bActive,
	const uint8 HeldFormation,
	const uint8 HeldSpacing)
{
	USolidCompany* Company = Squad.GetCompany();
	if (!Company)
	{
		return;
	}
	SeedBattlePlans(State, *Company);

	FString ActiveName;
	if (bActive && AssignedIndices.IsValidIndex(ActiveSlot))
	{
		if (const FSolidBattlePlan* Plan = Company->GetBattlePlan(AssignedIndices[ActiveSlot]))
		{
			ActiveName = Plan->Name;
		}
	}

	if (EnabledInCatalog(State, *Company) < SolidBattleFormationSlots::MaxAssignedBattlePlans)
	{
		Squad.SyncEnabledBattlePlans(State);
	}
	else if (AssignedIndices.Num() > 0)
	{
		Squad.SetAssignedBattlePlans(AssignedIndices);
	}

	if (!bActive)
	{
		Squad.SetHeldFormation(
			static_cast<ESolidBattleFormation>(HeldFormation),
			static_cast<ESolidBattleSpacing>(HeldSpacing));
		Squad.ClearActiveBattlePlan();
		return;
	}

	const int32 ByName = SlotForPlanName(Squad, ActiveName);
	if (ByName != INDEX_NONE)
	{
		Squad.SelectAssignedSlot(ByName);
		return;
	}
	if (Squad.GetAssignedBattlePlan(ActiveSlot))
	{
		Squad.SelectAssignedSlot(ActiveSlot);
	}
}

int32 SquadCount(const USolidGameState& State)
{
	return FMath::Clamp(State.MaxSquadSize, 0, RosterCount);
}

bool IsSightEnabled(const USolidCaptain& Captain, const ESolidSight Sight)
{
	switch (Sight)
	{
	case ESolidSight::True:
		return Captain.bTrueSightEnabled;
	case ESolidSight::Raven:
		return Captain.bRavenSightEnabled;
	default:
		return false;
	}
}

FResult NotifyOccupiedTown(
	USolidGameState& State,
	USolidCaptain& Captain,
	const int32 TownId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = Captain.Title;
	if (Captain.OccupiedTown == TownId)
	{
		return Result;
	}

	Captain.OccupiedTown = TownId;
	if (TownId == INDEX_NONE)
	{
		return Result;
	}

	for (int32 Index = 0; Index < DefinitionCount; ++Index)
	{
		if (State.bFired[Index])
		{
			continue;
		}

		const FEvent& Event = Definitions[Index];
		if (Event.Trigger != ETrigger::EnterTown || Event.Parameter != TownId)
		{
			continue;
		}
		TryFireEvent(State, Captain, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult NotifyReachBrokenBridge(
	USolidGameState& State,
	USolidCaptain& Captain,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = Captain.Title;

	for (int32 Index = 0; Index < DefinitionCount; ++Index)
	{
		if (Definitions[Index].Trigger != ETrigger::ReachBrokenBridge)
		{
			continue;
		}
		TryFireEvent(State, Captain, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult NotifyMessageClosed(
	USolidGameState& State,
	USolidCaptain& Captain,
	const TCHAR* MessageId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = Captain.Title;
	if (!MessageId || !MessageId[0])
	{
		return Result;
	}

	for (int32 Index = 0; Index < DefinitionCount; ++Index)
	{
		const FEvent& Event = Definitions[Index];
		if (Event.Trigger != ETrigger::MessageClosed || !Event.MessageId)
		{
			continue;
		}
		if (FCString::Strcmp(Event.MessageId, MessageId) != 0)
		{
			continue;
		}
		TryFireEvent(State, Captain, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult MarkComplete(
	USolidGameState& State,
	USolidCaptain& Captain,
	const TCHAR* EventId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = Captain.Title;
	if (!EventId || !EventId[0])
	{
		return Result;
	}

	TryFireEvent(State, Captain, FindIndexById(EventId), When, Result, /*bForce=*/true);
	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

}
