#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SolidPlayerController.generated.h"

class UInputMappingContext;

UCLASS()
class SOLIDCORE1_API ASolidPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASolidPlayerController();

	/**
	 * Right mouse button. While held, the mouse is captured and looks.
	 * Releasing it returns the bronze pointer to where the button went down.
	 */
	void SetMouseLookHeld(bool bHeld);

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	/** Free the mouse for the in-game pointer. Hardware cursor stays hidden. */
	void ApplyPointerMode();

	/** Mapping context applied for local players. Character also adds its own IMC when available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;
};
