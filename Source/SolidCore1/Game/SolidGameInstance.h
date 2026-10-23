#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Playthrough/SolidAutoPlaythrough.h"
#include "SolidTravelSession.h"
#include "SolidWorldTime.h"
#include "SolidGameInstance.generated.h"

class USolidCompany;
class USolidGameState;

/** Holds the captain's trip across the open world and a closed visit. Owns the company and the play state. */
UCLASS()
class SOLIDCORE1_API USolidGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** A play session. Not called again when the cave reloads this process. */
	virtual void OnStart() override;

	/**
	 * The play session's one company. Created on first ask with the starter catalog
	 * and the Meagre Dungeon. OpenLevel keeps this object.
	 */
	USolidCompany* GetCompany();

	/**
	 * The play session's one play state. Created on first ask. OpenLevel keeps this object.
	 * Inventory, journal, chest, events, previous strongholds, and furnishings live here.
	 */
	USolidGameState* GetConfiguration();

	/** The opening script. OpenLevel keeps this instance, and this object with it. */
	FSolidAutoPlaythrough& GetAutoPlaythrough() { return AutoPlaythrough; }
	const FSolidAutoPlaythrough& GetAutoPlaythrough() const { return AutoPlaythrough; }
	bool IsAutoPlaythroughRunning() const { return AutoPlaythrough.IsRunning(); }

	void Capture(const FSolidTravelSession& InSession);
	bool HasSession() const { return bHasSession; }
	const FSolidTravelSession& GetSession() const { return Session; }

	/**
	 * Point Fog values from the open-world terrain map. OpenLevel rebuilds that
	 * map; these values restore the trail after a stronghold or lair visit.
	 */
	void StoreExplorationFog(const TArray<float>& Fog);
	bool HasExplorationFog() const { return bHasExplorationFog; }
	const TArray<float>& GetExplorationFog() const { return ExplorationFog; }

	bool IsInStronghold() const { return bHasSession && Session.bInStronghold; }
	bool WantsStrongholdExit() const { return bHasSession && Session.bExitStronghold; }
	void ClearStrongholdExit();

	bool IsInLair() const { return bHasSession && Session.bInLair; }
	bool WantsLairMouthSpawn() const { return bHasSession && Session.bReturnToLairMouth; }
	void ClearLairMouthSpawn();

	/** A stronghold interior or the Iglin tunnels. */
	bool IsInClosedVisit() const { return IsInStronghold() || IsInLair(); }

	/**
	 * World time for a journal line. A new process advances the date once.
	 * OpenLevel, including the cave, keeps this clock.
	 */
	SolidWorldTime::FStamp WorldStamp() const;

private:
	void BeginWorldSession();

	UPROPERTY()
	TObjectPtr<USolidCompany> Company;

	UPROPERTY()
	TObjectPtr<USolidGameState> GameState;

	bool bHasSession = false;
	FSolidTravelSession Session;
	bool bHasExplorationFog = false;
	TArray<float> ExplorationFog;
	/** 1 on the first launch. Not reset by the cave trip. */
	int32 SessionNumber = 1;
	/** Platform seconds when this play session started. 0 until OnStart. */
	double SessionStartSeconds = 0.0;

	/** The opening script. A level load keeps this instance, so the script stays. */
	FSolidAutoPlaythrough AutoPlaythrough;
};
