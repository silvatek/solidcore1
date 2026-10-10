#include "SolidJournal.h"
#include "SolidSight.h"

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

void SolidJournal::CollectLines(const SolidEvents::FState& State, TArray<FString>& OutLines)
{
	OutLines.Reset();
	OutLines.Add(TEXT("Journal"));
	OutLines.Add(TEXT(""));

	if (State.FiredCount <= 0)
	{
		OutLines.Add(TEXT("No events yet."));
	}
	else
	{
		const int32 Count = FMath::Min(State.FiredCount, SolidEvents::MaxEvents);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 EventIndex = State.FiredOrder[Index];
			if (EventIndex < 0 || EventIndex >= SolidEvents::DefinitionCount)
			{
				continue;
			}
			const SolidEvents::FEvent& Event = SolidEvents::Definitions[EventIndex];
			OutLines.Add(FString::Printf(TEXT("%d  %s"), Index + 1, Event.Name));
			const FString Detail = Describe(Event);
			if (!Detail.IsEmpty())
			{
				OutLines.Add(FString::Printf(TEXT("    %s"), *Detail));
			}
		}
	}

	OutLines.Add(TEXT(""));
	OutLines.Add(CloseHint);
}
