#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "SolidHUD.generated.h"

UCLASS()
class SOLIDCORE1_API ASolidHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
};
