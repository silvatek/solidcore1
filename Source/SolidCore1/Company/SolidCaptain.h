#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SolidCaptain.generated.h"

class ASolidCharacter;

/** Titles the captain can hold. The nameplate and journal use the same words. */
UENUM()
enum class ESolidCaptainTitle : uint8
{
	Outcast,
	Orphan,
	Vagrant,
	Ranger,
	Captain,
};

/** Nameplate and journal word for a title. */
inline const TCHAR* CaptainTitleName(const ESolidCaptainTitle Title)
{
	switch (Title)
	{
	case ESolidCaptainTitle::Outcast: return TEXT("Outcast");
	case ESolidCaptainTitle::Orphan: return TEXT("Orphan");
	case ESolidCaptainTitle::Vagrant: return TEXT("Vagrant");
	case ESolidCaptainTitle::Ranger: return TEXT("Ranger");
	case ESolidCaptainTitle::Captain: return TEXT("Captain");
	default: return TEXT("Outcast");
	}
}

/**
 * The company's one captain for the play session.
 * Holds the title, the occupied town, and which sights are unlocked.
 * The live body is ASolidCharacter in the current world.
 */
UCLASS()
class SOLIDCORE1_API USolidCaptain : public UObject
{
	GENERATED_BODY()

public:
	ESolidCaptainTitle Title = ESolidCaptainTitle::Outcast;
	/** Numbered cell the captain is standing on, or INDEX_NONE. */
	int32 OccupiedTown = INDEX_NONE;
	bool bTrueSightEnabled = true;
	bool bRavenSightEnabled = false;
	bool bFalconSightEnabled = false;
	bool bEagleSightEnabled = false;

	/** Live body in this world. Cleared when that pawn ends. Not a UPROPERTY. */
	ASolidCharacter* GetActor() const { return Actor.Get(); }
	void SetActor(ASolidCharacter* InActor);

private:
	TWeakObjectPtr<ASolidCharacter> Actor;
};
