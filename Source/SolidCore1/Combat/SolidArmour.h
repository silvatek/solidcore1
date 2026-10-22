#pragma once

#include "CoreMinimal.h"

/**
 * Armour, the piece worn on the body.
 * Strength is how much that piece is. Play-testing chooses the amount. It stays off the screen.
 * A helmet, boots, and a belt are not armour, and they add no Strength.
 */
class FSolidArmour
{
public:
	const TCHAR* Name = TEXT("");
	/** How much this armour is. Play-testing chooses it. */
	float Strength = 1.f;
};

/**
 * Body armour a catalog item can point at.
 * The amount is a starting figure. Play-testing chooses it.
 */
namespace SolidArmour
{
	inline const FSolidArmour Rags { TEXT("Rags"), 1.f };
}
