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

	bool IsJournalVisible() const { return bShowJournal; }
	void SetJournalVisible(bool bVisible) { bShowJournal = bVisible; }

	/** F10. Closes the journal or credits if they are up; otherwise toggles the main menu. */
	void HandleMenuKey();
	/** Esc. Closes the menu, the journal, and the credits page. */
	void CloseMenuOverlay();
	/** Hide the menu list. Leaves the credits page as it is. */
	void CloseMainMenu();
	bool IsMainMenuOpen() const { return bShowMainMenu; }
	int32 GetMainMenuIndex() const { return MainMenuIndex; }
	void MoveMainMenuSelection(int32 Delta);
	void SetMainMenuIndex(int32 Index);

protected:
	void DrawCreditsPopup() const;
	void DrawJournalPopup() const;
	void DrawMainMenuPopup() const;
	/** Wall-clock timestamps (seconds) for frames in the rolling FPS window. */
	TArray<double> RecentFrameTimes;

	/** Rolling average window for the FPS gauge. */
	UPROPERTY(EditAnywhere, Category = "HUD")
	float FpsAverageWindowSeconds = 2.f;

	/** Credits page, opened from the main menu. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowCredits = false;

	/** Journal page, opened from the main menu. Lists events that have fired. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowJournal = false;

	/** F10 main menu. Off until toggled. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowMainMenu = false;

	/** Highlighted row while the main menu is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 MainMenuIndex = 0;
};
