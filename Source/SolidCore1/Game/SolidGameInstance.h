#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SolidDungeonLevel.h"
#include "SolidWorldTime.h"
#include "SolidGameInstance.generated.h"

/** Holds the captain's trip across the open world and a closed visit. */
UCLASS()
class SOLIDCORE1_API USolidGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** A play session. Not called again when the cave reloads this process. */
	virtual void OnStart() override;

	void Capture(const FSolidTravelSession& InSession);
	bool HasSession() const { return bHasSession; }
	const FSolidTravelSession& GetSession() const { return Session; }

	bool IsInDungeon() const { return bHasSession && Session.bInDungeon; }
	bool WantsDungeonMouthSpawn() const { return bHasSession && Session.bReturnToDungeonMouth; }
	void ClearDungeonMouthSpawn();

	bool IsInLair() const { return bHasSession && Session.bInLair; }
	bool WantsLairMouthSpawn() const { return bHasSession && Session.bReturnToLairMouth; }
	void ClearLairMouthSpawn();

	/**
	 * World time for a journal line. A new process advances the date once.
	 * OpenLevel, including the cave, keeps this clock.
	 */
	SolidWorldTime::FStamp WorldStamp() const;

private:
	void BeginWorldSession();

	bool bHasSession = false;
	FSolidTravelSession Session;
	/** 1 on the first launch. Not reset by the cave trip. */
	int32 SessionNumber = 1;
	/** Platform seconds when this play session started. 0 until OnStart. */
	double SessionStartSeconds = 0.0;
};
