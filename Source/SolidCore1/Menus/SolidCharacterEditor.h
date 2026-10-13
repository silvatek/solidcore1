#pragma once

#include "CoreMinimal.h"
#include "Party/SolidCompany.h"

/** Text for the F10 character editor. The stage itself is a world actor. */
namespace SolidCharacterEditor
{
	/** Move Index by Delta and wrap inside Count. Count of 0 stays at 0. */
	int32 WrapIndex(int32 Index, int32 Delta, int32 Count);

	/**
	 * Name, vocation, the nine body channels, and shirt, trouser, and skin numbers.
	 * The last two lines are the left/right and F10 hints.
	 */
	void Describe(const FSolidCompanion& Companion, TArray<FString>& OutLines);
}
