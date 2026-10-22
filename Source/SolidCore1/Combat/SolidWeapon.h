#pragma once

#include "CoreMinimal.h"

/**
 * A weapon. Speed is how long prepare takes and how long recover takes.
 * Power is what an attack takes off the target's armour.
 * On an item, Power is PowerFor of that item's quality. Play-testing chooses Speed.
 * Both stay off the screen.
 */
class FSolidWeapon
{
public:
	const TCHAR* Name = TEXT("");
	/** How long prepare takes, and how long recover takes. Play-testing chooses it. */
	float Speed = 0.f;
	/** Taken off the target's armour. On an item, this is PowerFor of the item's quality. */
	int32 Power = 0;
};
