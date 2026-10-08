#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SolidCore1HUD.generated.h"

UCLASS()
class SOLIDCORE1_API ASolidCore1HUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
