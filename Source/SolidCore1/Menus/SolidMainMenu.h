#pragma once

#include "CoreMinimal.h"

/** Entries shown by the F10 main menu. Order is the on-screen order. */
enum class ESolidMainMenuItem : uint8
{
	UsePortal,
	TestDrill,
	Journal,
	Credits,
	BuildParty,
	Dungeon,
	CharacterEditor,
	Inventory,
};

/** What confirming a menu entry does. The pawn performs the action. */
enum class ESolidMainMenuAction : uint8
{
	None,
	OpenPortal,
	StartTestDrill,
	ShowJournal,
	ShowCredits,
	OpenParty,
	TravelDungeon,
	OpenCharacterEditor,
	ShowInventory,
};

struct FSolidMainMenuEntry
{
	ESolidMainMenuItem Item = ESolidMainMenuItem::TestDrill;
	FString Label;
};

namespace SolidMainMenu
{
	/** bInDungeon swaps the cave row between Enter cave and Leave cave. */
	void CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries, bool bInDungeon = false);

	/** Enter cave stays closed until an event opens it. Leave cave stays available. */
	inline bool CanUseCaveRow(const bool bCaveEnabled, const bool bInDungeon)
	{
		return bInDungeon || bCaveEnabled;
	}
	int32 EntryCount();
	bool FindEntry(int32 Index, FSolidMainMenuEntry& OutEntry);
	ESolidMainMenuAction ActionForItem(ESolidMainMenuItem Item);

	/** Move Index by Delta and wrap inside the entry list. */
	int32 WrapIndex(int32 Index, int32 Delta);
}
