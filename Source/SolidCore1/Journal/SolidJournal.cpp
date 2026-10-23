#include "SolidJournal.h"
#include "Events/SolidEvents.h"
#include "Events/SolidMessages.h"
#include "Lairs/SolidLair.h"
#include "SolidSight.h"
#include "SolidStronghold.h"
#include "SolidWorldTime.h"

void USolidJournal::Remember(
	const SolidWorldTime::FStamp& When,
	const EJournalType Type,
	const FString& Text,
	const int32 EventIndex)
{
	if (Entries.Num() >= MaxEntries)
	{
		Entries.RemoveAt(0);
	}

	FJournalEntry Entry;
	Entry.Type = Type;
	Entry.Year = When.Year;
	Entry.Day = When.Day;
	Entry.MinuteOfSession = When.MinuteOfSession;
	Entry.EventIndex = EventIndex;
	Entry.Text = Text;
	Entries.Add(Entry);
}

void USolidJournal::RememberStronghold(const SolidWorldTime::FStamp& When, const bool bEnter)
{
	Remember(
		When,
		EJournalType::Stronghold,
		bEnter ? FString(SolidStronghold::EnterLabel) : FString(SolidStronghold::ExitLabel));
}

void USolidJournal::RememberLair(const SolidWorldTime::FStamp& When, const bool bEnter)
{
	Remember(When, EJournalType::Lair, SolidLair::VisitJournalText(bEnter));
}

FString USolidJournal::Describe(const SolidEvents::FEvent& Event)
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
			Part = FString::Printf(TEXT("Title %s"), CaptainTitleName(Change.Title));
			break;
		case SolidEvents::EChange::RepairBridge:
			Part = TEXT("Iglin bridge repaired");
			break;
		case SolidEvents::EChange::EnableCave:
			Part = TEXT("Cave enabled");
			break;
		case SolidEvents::EChange::ClearInventory:
			Part = TEXT("Inventory cleared");
			break;
		case SolidEvents::EChange::EnableStrongholdSize:
			Part = FString::Printf(TEXT("%s size unlocked"), SolidStronghold::SizeName(Change.StrongholdSize));
			break;
		case SolidEvents::EChange::EnableLair:
			if (const TCHAR* LairName = SolidLair::NameFor(Change.LairId))
			{
				Part = FString::Printf(TEXT("%s enabled"), LairName);
			}
			break;
		case SolidEvents::EChange::EnableBattlePlan:
			if (Change.BattlePlanName && Change.BattlePlanName[0])
			{
				Part = FString::Printf(TEXT("%s enabled"), Change.BattlePlanName);
			}
			break;
		case SolidEvents::EChange::DisableBattlePlan:
			if (Change.BattlePlanName && Change.BattlePlanName[0])
			{
				Part = FString::Printf(TEXT("%s disabled"), Change.BattlePlanName);
			}
			break;
		case SolidEvents::EChange::ShowMessage:
			if (const TCHAR* MessageText = SolidMessages::TextFor(Change.MessageId))
			{
				Part = MessageText;
			}
			break;
		case SolidEvents::EChange::AddCompanion:
			if (Change.CompanionName && Change.CompanionName[0])
			{
				Part = FString::Printf(TEXT("%s joins"), Change.CompanionName);
			}
			break;
		case SolidEvents::EChange::MaxSquadSize:
		{
			const int32 Count = FMath::Clamp(Change.SquadSize, 0, SolidEvents::RosterCount);
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
				Part = FString::Printf(TEXT("Squad %d"), Count);
			}
			else
			{
				Part = FString::Printf(TEXT("Squad %d: %s"), Count, *Names);
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

void USolidJournal::CollectRows(TArray<FRow>& OutRows) const
{
	OutRows.Reset();
	OutRows.Add({ ERowKind::Title, FString(), FString(Title) });
	OutRows.Add({ ERowKind::Heading, FString(TimeHeading), FString(SummaryHeading) });

	if (Entries.Num() <= 0)
	{
		OutRows.Add({ ERowKind::Empty, FString(), FString(EmptyText) });
	}
	else
	{
		for (const FJournalEntry& Entry : Entries)
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
