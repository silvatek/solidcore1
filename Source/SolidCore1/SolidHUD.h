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

	void ToggleCredits();
	bool IsCreditsVisible() const { return bShowCredits; }
	void SetCreditsVisible(bool bVisible) { bShowCredits = bVisible; }

protected:
	void DrawCreditsPopup() const;
	/** Wall-clock timestamps (seconds) for frames in the rolling FPS window. */
	TArray<double> RecentFrameTimes;

	/** Rolling average window for the FPS gauge. */
	UPROPERTY(EditAnywhere, Category = "HUD")
	float FpsAverageWindowSeconds = 2.f;

	/** F10 credits overlay. Off until toggled. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowCredits = false;
};
