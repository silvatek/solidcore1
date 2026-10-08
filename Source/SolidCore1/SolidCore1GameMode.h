#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SolidCore1GameMode.generated.h"

UCLASS()
class SOLIDCORE1_API ASolidCore1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASolidCore1GameMode();

	/** When true, spawns a terrain streamer at BeginPlay if the level does not already have one. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	bool bAutoSpawnTerrainStreamer = true;

protected:
	virtual void BeginPlay() override;

	void EnsureTerrainStreamer();
};
