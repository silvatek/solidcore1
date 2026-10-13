#include "SolidMainMenu.h"
#include "SolidDungeonLevel.h"

void SolidMainMenu::CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries, const bool bInDungeon)
{
	OutEntries.Reset();
	OutEntries.Add({ ESolidMainMenuItem::UsePortal, TEXT("Use portal") });
	OutEntries.Add({ ESolidMainMenuItem::TestDrill, TEXT("Test Drill") });
	OutEntries.Add({ ESolidMainMenuItem::Journal, TEXT("Journal") });
	OutEntries.Add({ ESolidMainMenuItem::Credits, TEXT("Credits") });
	OutEntries.Add({ ESolidMainMenuItem::BuildParty, TEXT("Build party") });
	OutEntries.Add({ ESolidMainMenuItem::Dungeon, SolidDungeonLevel::MenuLabel(bInDungeon) });
	OutEntries.Add({ ESolidMainMenuItem::CharacterEditor, TEXT("Character editor") });
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
	case ESolidMainMenuItem::TestDrill:
		return ESolidMainMenuAction::StartTestDrill;
	case ESolidMainMenuItem::Journal:
		return ESolidMainMenuAction::ShowJournal;
	case ESolidMainMenuItem::Credits:
		return ESolidMainMenuAction::ShowCredits;
	case ESolidMainMenuItem::BuildParty:
		return ESolidMainMenuAction::OpenParty;
	case ESolidMainMenuItem::Dungeon:
		return ESolidMainMenuAction::TravelDungeon;
	case ESolidMainMenuItem::CharacterEditor:
		return ESolidMainMenuAction::OpenCharacterEditor;
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
