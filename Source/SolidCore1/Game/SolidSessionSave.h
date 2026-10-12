#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SolidSessionSave.generated.h"

/** How many play sessions have started. The cave trip does not write this. */
UCLASS()
class SOLIDCORE1_API USolidSessionSave : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY()
	int32 SessionNumber = 0;
};
