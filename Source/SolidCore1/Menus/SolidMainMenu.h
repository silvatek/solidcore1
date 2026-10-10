#pragma once

#include "CoreMinimal.h"

/** Entries shown by the F10 main menu. Order is the on-screen order. */
enum class ESolidMainMenuItem : uint8
{
	TestDrill,
	Journal,
	Credits,
};

/** What confirming a menu entry does. The pawn performs the action. */
enum class ESolidMainMenuAction : uint8
{
	None,
	StartTestDrill,
	ShowJournal,
	ShowCredits,
};

struct FSolidMainMenuEntry
{
	ESolidMainMenuItem Item = ESolidMainMenuItem::TestDrill;
	FString Label;
};

namespace SolidMainMenu
{
	void CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries);
	int32 EntryCount();
	bool FindEntry(int32 Index, FSolidMainMenuEntry& OutEntry);
	ESolidMainMenuAction ActionForItem(ESolidMainMenuItem Item);

	/** Move Index by Delta and wrap inside the entry list. */
	int32 WrapIndex(int32 Index, int32 Delta);
}
