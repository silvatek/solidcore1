#pragma once

#include "CoreMinimal.h"

/**
 * Stable lair ids. Eight hex digits.
 * An EnableLair change and the company state use these, not the display name.
 * The world map still writes the display name.
 */
namespace SolidLair
{
	inline constexpr int32 IdLength = 8;

	inline constexpr const TCHAR* IdIglinTunnels = TEXT("d18f4a73");
}
