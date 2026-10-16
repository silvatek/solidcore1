#include "SolidGameState.h"

namespace
{
	bool RequirementsMet(const FGameState& State, const SolidEvents::FEvent& Event)
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

	void ApplyGameStateChange(FGameState& State, const SolidEvents::FChange& Change, SolidEvents::FResult& Result)
	{
		switch (Change.Kind)
		{
		case SolidEvents::EChange::EnableSight:
			if (Change.Sight == ESolidSight::Raven)
			{
				State.bRavenSightEnabled = true;
			}
			else
			{
				State.bTrueSightEnabled = true;
			}
			Result.bSetSight = true;
			Result.Sight = Change.Sight;
			break;
		case SolidEvents::EChange::MaxPartySize:
			State.MaxPartySize = FMath::Max(0, Change.PartySize);
			Result.MaxPartySize = State.MaxPartySize;
			break;
		case SolidEvents::EChange::CaptainTitle:
			if (Change.Title && Change.Title[0])
			{
				State.CaptainTitle = Change.Title;
				Result.bSetTitle = true;
				Result.CaptainTitle = Change.Title;
			}
			break;
		case SolidEvents::EChange::RepairBridge:
			State.bIglinBridgeRepaired = true;
			Result.bRepairBridge = true;
			break;
		default:
			break;
		}
	}
}

namespace SolidEvents
{

int32 PartyCount(const FGameState& State)
{
	return FMath::Clamp(State.MaxPartySize, 0, RosterCount);
}

bool IsSightEnabled(const FGameState& State, const ESolidSight Sight)
{
	switch (Sight)
	{
	case ESolidSight::True:
		return State.bTrueSightEnabled;
	case ESolidSight::Raven:
		return State.bRavenSightEnabled;
	default:
		return false;
	}
}

void Remember(
	FGameState& State,
	const SolidWorldTime::FStamp& When,
	const SolidJournal::EJournalType Type,
	const FString& Text,
	const int32 EventIndex)
{
	if (State.Journal.Num() >= SolidJournal::MaxEntries)
	{
		State.Journal.RemoveAt(0);
	}

	SolidJournal::FJournalEntry Entry;
	Entry.Type = Type;
	Entry.Year = When.Year;
	Entry.Day = When.Day;
	Entry.MinuteOfSession = When.MinuteOfSession;
	Entry.EventIndex = EventIndex;
	Entry.Text = Text;
	State.Journal.Add(Entry);
}

void RememberStronghold(
	FGameState& State,
	const SolidWorldTime::FStamp& When,
	const bool bEnter)
{
	Remember(
		State,
		When,
		SolidJournal::EJournalType::Stronghold,
		bEnter ? FString(TEXT("Enter Stronghold")) : FString(TEXT("Exit Stronghold")));
}

FResult NotifyOccupiedTown(
	FGameState& State,
	const int32 TownId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxPartySize = State.MaxPartySize;
	Result.CaptainTitle = State.CaptainTitle;
	if (State.OccupiedTown == TownId)
	{
		return Result;
	}

	State.OccupiedTown = TownId;
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
		if (!RequirementsMet(State, Event))
		{
			continue;
		}

		State.bFired[Index] = true;
		if (State.FiredCount < MaxEvents)
		{
			State.FiredOrder[State.FiredCount] = Index;
			++State.FiredCount;
		}
		Remember(State, When, SolidJournal::EJournalType::Event, FString(), Index);
		Result.bFired = true;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, MaxChangesPerEvent);
		for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
		{
			ApplyGameStateChange(State, Event.Changes[ChangeIndex], Result);
		}
	}

	Result.MaxPartySize = State.MaxPartySize;
	return Result;
}

}
