#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SolidCaveLevel.h"
#include "SolidGameInstance.generated.h"

/** Holds the captain's trip across the open world and the cave interior. */
UCLASS()
class SOLIDCORE1_API USolidGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void Capture(const FSolidTravelSession& InSession);
	bool HasSession() const { return bHasSession; }
	const FSolidTravelSession& GetSession() const { return Session; }

	bool IsInCave() const { return bHasSession && Session.bInCave; }
	bool WantsCaveMouthSpawn() const { return bHasSession && Session.bReturnToCaveMouth; }
	void ClearCaveMouthSpawn();

private:
	bool bHasSession = false;
	FSolidTravelSession Session;
};
