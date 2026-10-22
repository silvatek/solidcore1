#pragma once

#include "CoreMinimal.h"

/**
 * A weapon. Speed is how long prepare takes and how long recover takes.
 * Power is what an attack takes off the target's armour.
 * Play-testing chooses both amounts. They stay off the screen.
 */
class FSolidWeapon
{
public:
	const TCHAR* Name = TEXT("");
	/** How long prepare takes, and how long recover takes. Play-testing chooses it. */
	float Speed = 1.f;
	/** Taken off the target's armour. Play-testing chooses it. */
	float Power = 1.f;
};

/**
 * Catalog weapons. Each one has its own Speed and Power.
 * The amounts are a starting figure. Play-testing chooses them.
 */
namespace SolidWeapon
{
	inline const FSolidWeapon CrudeSpear { TEXT("Crude spear"), 1.f, 1.f };
	inline const FSolidWeapon RustySword { TEXT("Rusty sword"), 1.f, 1.f };
}
