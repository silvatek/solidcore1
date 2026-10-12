#include "SolidGameInstance.h"

void USolidGameInstance::Capture(const FSolidTravelSession& InSession)
{
	Session = InSession;
	bHasSession = true;
}

void USolidGameInstance::ClearCaveMouthSpawn()
{
	Session.bReturnToCaveMouth = false;
}
