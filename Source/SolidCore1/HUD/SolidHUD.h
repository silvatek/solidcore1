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
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ToggleCredits();
	bool IsCreditsVisible() const { return bShowCredits; }
	void SetCreditsVisible(bool bVisible) { bShowCredits = bVisible; }

	bool IsJournalVisible() const { return bShowJournal; }
	void SetJournalVisible(bool bVisible) { bShowJournal = bVisible; }

	/** Town list opened from Use portal. */
	void OpenPortal();
	void ClosePortal();
	bool IsPortalOpen() const { return bShowPortal; }
	int32 GetPortalIndex() const { return PortalIndex; }
	void MovePortalSelection(int32 Delta);
	void SetPortalIndex(int32 Index);

	/** F12. The tech readout in the corner. Battle plans stay up either way. */
	void ToggleDebugPanel();
	bool IsDebugPanelVisible() const { return bShowDebugPanel; }
	void SetDebugPanelVisible(bool bVisible) { bShowDebugPanel = bVisible; }

	/** F10. Closes the portal, journal, or credits if one is up; otherwise toggles the main menu. */
	void HandleMenuKey();
	/** Esc. Closes the menu, the portal, the journal, and the credits page. */
	void CloseMenuOverlay();
	/** Hide the menu list. Leaves the credits page as it is. */
	void CloseMainMenu();
	bool IsMainMenuOpen() const { return bShowMainMenu; }
	int32 GetMainMenuIndex() const { return MainMenuIndex; }
	void MoveMainMenuSelection(int32 Delta);
	void SetMainMenuIndex(int32 Index);

protected:
	void EnsureSightIndicator();
	void RemoveSightIndicator();
	void DrawCreditsPopup() const;
	void DrawJournalPopup() const;
	void DrawMainMenuPopup() const;
	void DrawPortalPopup() const;
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

	/** Town list from Use portal. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowPortal = false;

	/** Highlighted town while the portal list is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 PortalIndex = 0;

	/** Tech readout (build, FPS, pawn, terrain). On until F12. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowDebugPanel = true;

	/** Highlighted row while the main menu is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 MainMenuIndex = 0;

	/** Top-right eye / eagle widget has been added to the viewport. */
	bool bSightIndicatorAdded = false;
};
