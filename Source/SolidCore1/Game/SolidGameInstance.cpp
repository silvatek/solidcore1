#include "SolidGameInstance.h"

void USolidGameInstance::Capture(const FSolidTravelSession& InSession)
{
	Session = InSession;
	bHasSession = true;
}

void USolidGameInstance::ClearDungeonMouthSpawn()
{
	Session.bReturnToDungeonMouth = false;
}
