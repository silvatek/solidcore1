#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/** Entries shown by the F10 main menu. Order is the on-screen order. */
enum class ESolidMainMenuItem : uint8
{
	UsePortal,
	Journal,
	BuildSquad,
	CharacterEditor,
	Credits,
	ExitGame,
};

/** What confirming a menu entry does. The pawn performs the action. */
enum class ESolidMainMenuAction : uint8
{
	None,
	OpenPortal,
	ShowJournal,
	ShowCredits,
	OpenSquad,
	OpenCharacterEditor,
	Quit,
};

struct FSolidMainMenuEntry
{
	ESolidMainMenuItem Item = ESolidMainMenuItem::UsePortal;
	FString Label;
};

namespace SolidMainMenu
{
	/** Menu on Xbox, Options on PlayStation. The same key as F10. */
	inline FKey ControllerKey()
	{
		return EKeys::Gamepad_Special_Right;
	}

	void CollectEntries(TArray<FSolidMainMenuEntry>& OutEntries);
	int32 EntryCount();
	bool FindEntry(int32 Index, FSolidMainMenuEntry& OutEntry);
	ESolidMainMenuAction ActionForItem(ESolidMainMenuItem Item);

	/** Move Index by Delta and wrap inside the entry list. */
	int32 WrapIndex(int32 Index, int32 Delta);
}
