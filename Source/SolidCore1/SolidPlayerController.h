#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SolidPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;

UCLASS()
class SOLIDCORE1_API ASolidPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASolidPlayerController();

	/** True when player input or Slate still has the right button down. */
	bool IsRightMouseHeld() const;

	/** Current Axis2D value of a mapped Enhanced Input action, or zero. */
	FVector2D GetMappedAxis2D(const UInputAction* Action) const;

protected:
	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	/** Free the mouse until right-button look. Hardware cursor stays hidden. */
	void ApplyPointerMode();
	void EnsurePointerCaptureMode();
	void HideHardwareCursor();

	/** Mapping context applied for local players. Character also adds its own IMC when available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	int32 DefaultMappingPriority = 0;
};
