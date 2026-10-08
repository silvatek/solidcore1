#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SolidCore1PlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class SOLIDCORE1_API ASolidCore1PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASolidCore1PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Mapping context applied for local players. Character also adds its own IMC when available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;
};
