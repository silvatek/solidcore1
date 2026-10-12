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
	Cave,
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
	TravelCave,
};

struct FSolidMainMenuEntry
{
	ESolidMainMenuItem Item = ESolidMainMenuItem::TestDrill;
	FString Label;
};

namespace SolidMainMenu
{
	/** bInCave swaps the last row between Enter cave and Leave cave. */
	void CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries, bool bInCave = false);
	int32 EntryCount();
	bool FindEntry(int32 Index, FSolidMainMenuEntry& OutEntry);
	ESolidMainMenuAction ActionForItem(ESolidMainMenuItem Item);

	/** Move Index by Delta and wrap inside the entry list. */
	int32 WrapIndex(int32 Index, int32 Delta);
}
