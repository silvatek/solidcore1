#include "SolidArrange.h"

bool FSolidArrange::Toggle(const FContext& Context)
{
	if (Context.bOverlay || !Context.bInStronghold)
	{
		return false;
	}
	bOn = !bOn;
	return true;
}

void FSolidArrange::Leave()
{
	bOn = false;
}
