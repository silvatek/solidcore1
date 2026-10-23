#include "SolidMainMenu.h"
#include "Playthrough/SolidAutoPlaythrough.h"

void SolidMainMenu::CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries)
{
	OutEntries.Reset();
	OutEntries.Add({ ESolidMainMenuItem::UsePortal, TEXT("Use portal") });
	OutEntries.Add({ ESolidMainMenuItem::Journal, TEXT("Journal") });
	OutEntries.Add({ ESolidMainMenuItem::BuildSquad, TEXT("Build squad") });
	OutEntries.Add({ ESolidMainMenuItem::CharacterEditor, TEXT("Character editor") });
	OutEntries.Add({ ESolidMainMenuItem::AutoPlaythrough, FSolidAutoPlaythrough::Label() });
	OutEntries.Add({ ESolidMainMenuItem::Credits, TEXT("Credits") });
	OutEntries.Add({ ESolidMainMenuItem::ExitGame, TEXT("Exit Game") });
}

int32 SolidMainMenu::EntryCount()
{
	TArray<FSolidMainMenuEntry> Entries;
	CollectEntries(Entries);
	return Entries.Num();
}

bool SolidMainMenu::FindEntry(const int32 Index, FSolidMainMenuEntry& OutEntry)
{
	TArray<FSolidMainMenuEntry> Entries;
	CollectEntries(Entries);
	if (!Entries.IsValidIndex(Index))
	{
		return false;
	}
	OutEntry = Entries[Index];
	return true;
}

ESolidMainMenuAction SolidMainMenu::ActionForItem(const ESolidMainMenuItem Item)
{
	switch (Item)
	{
	case ESolidMainMenuItem::UsePortal:
		return ESolidMainMenuAction::OpenPortal;
	case ESolidMainMenuItem::Journal:
		return ESolidMainMenuAction::ShowJournal;
	case ESolidMainMenuItem::BuildSquad:
		return ESolidMainMenuAction::OpenSquad;
	case ESolidMainMenuItem::CharacterEditor:
		return ESolidMainMenuAction::OpenCharacterEditor;
	case ESolidMainMenuItem::AutoPlaythrough:
		return ESolidMainMenuAction::StartAutoPlaythrough;
	case ESolidMainMenuItem::Credits:
		return ESolidMainMenuAction::ShowCredits;
	case ESolidMainMenuItem::ExitGame:
		return ESolidMainMenuAction::Quit;
	default:
		return ESolidMainMenuAction::None;
	}
}

int32 SolidMainMenu::WrapIndex(const int32 Index, const int32 Delta)
{
	const int32 Count = EntryCount();
	if (Count <= 0)
	{
		return 0;
	}

	int32 Wrapped = (Index + Delta) % Count;
	if (Wrapped < 0)
	{
		Wrapped += Count;
	}
	return Wrapped;
}
