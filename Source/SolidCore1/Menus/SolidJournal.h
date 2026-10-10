#pragma once

#include "CoreMinimal.h"
#include "Events/SolidEvents.h"

/**
 * Journal page opened from the F10 menu.
 * Lists events in the order they fired, plus what each one changed.
 */
namespace SolidJournal
{
	inline constexpr const TCHAR* CloseHint = TEXT("F10 to close");

	/** One line describing an event's configuration changes. */
	FString Describe(const SolidEvents::FEvent& Event);

	void CollectLines(const SolidEvents::FState& State, TArray<FString>& OutLines);
}
