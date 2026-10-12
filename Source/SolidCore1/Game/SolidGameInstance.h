#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SolidDungeonLevel.h"
#include "SolidGameInstance.generated.h"

/** Holds the captain's trip across the open world and a dungeon interior. */
UCLASS()
class SOLIDCORE1_API USolidGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void Capture(const FSolidTravelSession& InSession);
	bool HasSession() const { return bHasSession; }
	const FSolidTravelSession& GetSession() const { return Session; }

	bool IsInDungeon() const { return bHasSession && Session.bInDungeon; }
	bool WantsDungeonMouthSpawn() const { return bHasSession && Session.bReturnToDungeonMouth; }
	void ClearDungeonMouthSpawn();

private:
	bool bHasSession = false;
	FSolidTravelSession Session;
};
