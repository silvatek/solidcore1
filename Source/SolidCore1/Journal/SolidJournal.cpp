#include "SolidJournal.h"
#include "SolidGameState.h"
#include "SolidSight.h"
#include "SolidWorldTime.h"

FString SolidJournal::Describe(const SolidEvents::FEvent& Event)
{
	FString Text;
	const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
	for (int32 Index = 0; Index < ChangeCount; ++Index)
	{
		const SolidEvents::FChange& Change = Event.Changes[Index];
		FString Part;
		switch (Change.Kind)
		{
		case SolidEvents::EChange::EnableSight:
			Part = FString::Printf(TEXT("%s enabled"), SolidSight::Label(Change.Sight));
			break;
		case SolidEvents::EChange::CaptainTitle:
			if (Change.Title && Change.Title[0])
			{
				Part = FString::Printf(TEXT("Title %s"), Change.Title);
			}
			break;
		case SolidEvents::EChange::RepairBridge:
			Part = TEXT("Iglin bridge repaired");
			break;
		case SolidEvents::EChange::EnableCave:
			Part = TEXT("Cave enabled");
			break;
		case SolidEvents::EChange::MaxPartySize:
		{
			const int32 Count = FMath::Clamp(Change.PartySize, 0, SolidEvents::RosterCount);
			FString Names;
			for (int32 Slot = 0; Slot < Count; ++Slot)
			{
				if (Slot > 0)
				{
					Names += TEXT(", ");
				}
				Names += SolidEvents::RosterName(Slot);
			}
			if (Names.IsEmpty())
			{
				Part = FString::Printf(TEXT("Party %d"), Count);
			}
			else
			{
				Part = FString::Printf(TEXT("Party %d: %s"), Count, *Names);
			}
			break;
		}
		default:
			break;
		}

		if (Part.IsEmpty())
		{
			continue;
		}
		if (!Text.IsEmpty())
		{
			Text += TEXT(". ");
		}
		Text += Part;
	}
	return Text;
}

void SolidJournal::CollectRows(const FGameState& State, TArray<FRow>& OutRows)
{
	OutRows.Reset();
	OutRows.Add({ ERowKind::Title, FString(), FString(Title) });
	OutRows.Add({ ERowKind::Heading, FString(TimeHeading), FString(SummaryHeading) });

	if (State.Journal.Num() <= 0)
	{
		OutRows.Add({ ERowKind::Empty, FString(), FString(EmptyText) });
	}
	else
	{
		for (const FJournalEntry& Entry : State.Journal)
		{
			FString Text = Entry.Text;
			FString Detail;
			if (Entry.Type == EJournalType::Event
				&& Entry.EventIndex >= 0
				&& Entry.EventIndex < SolidEvents::DefinitionCount)
			{
				const SolidEvents::FEvent& Event = SolidEvents::Definitions[Entry.EventIndex];
				Text = Event.Name;
				Detail = Describe(Event);
			}
			if (Text.IsEmpty())
			{
				continue;
			}

			SolidWorldTime::FStamp Stamp;
			Stamp.Year = Entry.Year;
			Stamp.Day = Entry.Day;
			Stamp.MinuteOfSession = Entry.MinuteOfSession;
			OutRows.Add({ ERowKind::Entry, SolidWorldTime::Format(Stamp), Text });
			if (!Detail.IsEmpty())
			{
				OutRows.Add({ ERowKind::Detail, FString(), Detail });
			}
		}
	}

	OutRows.Add({ ERowKind::Hint, FString(), FString(CloseHint) });
}
