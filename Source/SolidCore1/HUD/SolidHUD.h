#pragma once

#include "CoreMinimal.h"
#include "Menus/SolidMenuPointer.h"
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

	/** Company list opened from Build party. */
	void OpenPartyBuilder();
	void ClosePartyBuilder();
	bool IsPartyBuilderOpen() const { return bShowPartyBuilder; }
	int32 GetPartyBuilderIndex() const { return PartyBuilderIndex; }
	void MovePartyBuilderSelection(int32 Delta);
	void SetPartyBuilderIndex(int32 Index);

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

	/** One companion on a plain stage. Left and right cycle the company. F10 returns to the world. */
	void OpenCharacterEditor();
	void CloseCharacterEditor();
	bool IsCharacterEditorOpen() const { return bShowCharacterEditor; }
	int32 GetCharacterEditorIndex() const { return CharacterEditorIndex; }
	void MoveCharacterEditor(int32 Delta);

	/** F10. Closes an open page, including the character editor, or toggles the main menu. */
	void HandleMenuKey();
	/** Esc. Closes the menu, the portal, the party builder, the journal, the credits page, and the character editor. */
	void CloseMenuOverlay();
	/** Hide the menu list. Leaves the credits page as it is. */
	void CloseMainMenu();
	bool IsMainMenuOpen() const { return bShowMainMenu; }
	int32 GetMainMenuIndex() const { return MainMenuIndex; }
	void MoveMainMenuSelection(int32 Delta);
	void SetMainMenuIndex(int32 Index);

	/** Rows from the choice list drawn this frame. Empty for the journal and credits. */
	void SetChoicePointerRows(const TArray<SolidMenuPointer::FRow>& Rows);
	/**
	 * When the main menu or the portal list is open and the point hits a row, select it.
	 * A miss leaves the keyboard selection alone.
	 */
	bool SelectChoiceAt(float X, float Y);

	/** Empty clears the label. Shown beside the pointer while it rests on the cave opening. */
	void SetCavePassageTip(const FString& Tip);
	const FString& GetCavePassageTip() const { return CavePassageTip; }

protected:
	void EnsureSightIndicator();
	void RemoveSightIndicator();
	void DrawCreditsPopup() const;
	void DrawJournalPopup() const;
	void DrawCharacterEditorPanel() const;
	void DrawMainMenuPopup();
	void DrawPortalPopup();
	void DrawPartyBuilderPopup();
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

	/** Party builder, opened from the main menu once two companions are allowed. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowPartyBuilder = false;

	/** Highlighted companion while the party builder is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 PartyBuilderIndex = 0;

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

	/** Full-screen companion stage from the main menu. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowCharacterEditor = false;

	/** Company roster index while the character editor is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 CharacterEditorIndex = 0;

	/** Top-right eye / eagle widget has been added to the viewport. */
	bool bSightIndicatorAdded = false;

	/** Enter cave / Leave cave, or empty when the pointer is elsewhere. */
	FString CavePassageTip;
};
