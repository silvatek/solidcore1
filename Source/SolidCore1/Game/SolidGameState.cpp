#include "SolidGameState.h"
#include "Lairs/SolidLair.h"

FGameState::FGameState()
	: ActiveStronghold(SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		SolidStronghold::StartingCell(),
		SolidStronghold::WorldXY(SolidStronghold::StartingCell())))
{
	if (ActiveStronghold)
	{
		ActiveStronghold->Orientation = ESolidStrongholdOrientation::North;
		ActiveStronghold->Name = SolidStronghold::StartingStrongholdName;
	}
}

FGameState::FGameState(const FGameState& Other)
{
	*this = Other;
}

FGameState& FGameState::operator=(const FGameState& Other)
{
	if (this == &Other)
	{
		return *this;
	}

	MaxSquadSize = Other.MaxSquadSize;
	CaptainTitle = Other.CaptainTitle;
	bTrueSightEnabled = Other.bTrueSightEnabled;
	bRavenSightEnabled = Other.bRavenSightEnabled;
	bIglinBridgeRepaired = Other.bIglinBridgeRepaired;
	bCaveEnabled = Other.bCaveEnabled;
	for (int32 Index = 0; Index < SolidEvents::MaxEnabledLairs; ++Index)
	{
		EnabledLairs[Index] = Other.EnabledLairs[Index];
	}
	EnabledLairCount = Other.EnabledLairCount;
	OccupiedTown = Other.OccupiedTown;
	for (int32 Index = 0; Index < SolidEvents::MaxEvents; ++Index)
	{
		bFired[Index] = Other.bFired[Index];
		FiredOrder[Index] = Other.FiredOrder[Index];
	}
	FiredCount = Other.FiredCount;
	Journal = Other.Journal;
	Inventory = Other.Inventory;
	TakenForagePiles = Other.TakenForagePiles;
	CaveChest = Other.CaveChest;
	ActiveStronghold = Other.ActiveStronghold
		? Other.ActiveStronghold->Clone()
		: SolidStronghold::Make(
			ESolidStrongholdType::Dungeon,
			ESolidStrongholdSize::Meagre,
			SolidStronghold::StartingCell(),
			SolidStronghold::WorldXY(SolidStronghold::StartingCell()));
	if (!Other.ActiveStronghold && ActiveStronghold)
	{
		ActiveStronghold->Orientation = ESolidStrongholdOrientation::North;
		ActiveStronghold->Name = SolidStronghold::StartingStrongholdName;
	}
	MaxStrongholdSize = Other.MaxStrongholdSize;
	PreviousStrongholds.Reset();
	PreviousStrongholds.Reserve(Other.PreviousStrongholds.Num());
	for (const TUniquePtr<FSolidStronghold>& Previous : Other.PreviousStrongholds)
	{
		PreviousStrongholds.Add(Previous ? Previous->Clone() : nullptr);
	}
	Furnishings = Other.Furnishings;
	return *this;
}

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

	bool TryFireEvent(
		FGameState& State,
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
		if (State.FiredCount < SolidEvents::MaxEvents)
		{
			State.FiredOrder[State.FiredCount] = Index;
			++State.FiredCount;
		}
		SolidEvents::Remember(State, When, SolidJournal::EJournalType::Event, FString(), Index);
		Result.bFired = true;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
		for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
		{
			SolidEvents::ApplyChange(State, Event.Changes[ChangeIndex], Result);
		}
		return true;
	}
}

namespace SolidEvents
{

void ApplyChange(FGameState& State, const FChange& Change, FResult& Result)
{
	switch (Change.Kind)
	{
	case EChange::EnableSight:
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
	case EChange::MaxSquadSize:
		State.MaxSquadSize = FMath::Max(0, Change.SquadSize);
		Result.MaxSquadSize = State.MaxSquadSize;
		break;
	case EChange::CaptainTitle:
		if (Change.Title && Change.Title[0])
		{
			State.CaptainTitle = Change.Title;
			Result.bSetTitle = true;
			Result.CaptainTitle = Change.Title;
		}
		break;
	case EChange::RepairBridge:
		State.bIglinBridgeRepaired = true;
		Result.bRepairBridge = true;
		break;
	case EChange::EnableCave:
		State.bCaveEnabled = true;
		Result.bEnterCave = true;
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
			if (State.EnabledLairCount < MaxEnabledLairs)
			{
				State.EnabledLairs[State.EnabledLairCount] = Change.LairId;
				++State.EnabledLairCount;
			}
		}
		break;
	default:
		break;
	}
}

int32 SquadCount(const FGameState& State)
{
	return FMath::Clamp(State.MaxSquadSize, 0, RosterCount);
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

void RememberLair(
	FGameState& State,
	const SolidWorldTime::FStamp& When,
	const bool bEnter)
{
	Remember(
		State,
		When,
		SolidJournal::EJournalType::Lair,
		SolidLair::VisitJournalText(bEnter));
}

FResult NotifyOccupiedTown(
	FGameState& State,
	const int32 TownId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
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
		TryFireEvent(State, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult NotifyReachBrokenBridge(
	FGameState& State,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = State.CaptainTitle;

	for (int32 Index = 0; Index < DefinitionCount; ++Index)
	{
		if (Definitions[Index].Trigger != ETrigger::ReachBrokenBridge)
		{
			continue;
		}
		TryFireEvent(State, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult NotifyMessageClosed(
	FGameState& State,
	const TCHAR* MessageId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = State.CaptainTitle;
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
		TryFireEvent(State, Index, When, Result);
	}

	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

FResult MarkComplete(
	FGameState& State,
	const TCHAR* EventId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxSquadSize = State.MaxSquadSize;
	Result.CaptainTitle = State.CaptainTitle;
	if (!EventId || !EventId[0])
	{
		return Result;
	}

	TryFireEvent(State, FindIndexById(EventId), When, Result, /*bForce=*/true);
	Result.MaxSquadSize = State.MaxSquadSize;
	return Result;
}

}
