#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SolidStrongholdExterior.generated.h"

/**
 * An actor drawn for one stronghold. Dungeon and tower exteriors are the kinds.
 * The stronghold names which kind, and fills it.
 */
UCLASS()
class SOLIDCORE1_API ASolidStrongholdExterior : public AActor
{
	GENERATED_BODY()

public:
	ASolidStrongholdExterior();
};
