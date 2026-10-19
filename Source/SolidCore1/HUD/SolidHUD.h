#pragma once

#include "CoreMinimal.h"
#include "Menus/SolidCharacterEditor.h"
#include "Menus/SolidMenuPointer.h"
#include "Inventory/SolidInventory.h"
#include "Menus/SolidSquadBuilder.h"
#include "GameFramework/HUD.h"
#include "SolidStronghold.h"
#include "SolidHUD.generated.h"

UCLASS()
class SOLIDCORE1_API ASolidHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsCreditsVisible() const { return bShowCredits; }
	void SetCreditsVisible(bool bVisible) { bShowCredits = bVisible; }

	bool IsJournalVisible() const { return bShowJournal; }
	void SetJournalVisible(bool bVisible) { bShowJournal = bVisible; }

	/** # shows the worn boxes and the pack. # again, Esc, or F10 closes it. */
	void ToggleInventory();
	bool IsInventoryVisible() const { return bShowInventory; }

	/** Press a slot that holds an item, or the Craft button. Release on another slot, or on the button. */
	void PressInventory(float X, float Y);
	void ReleaseInventory(float X, float Y);
	void CancelInventoryDrag();

	/** Company, squad boxes, and battle-plan keys. Drag a name onto a box. */
	void OpenSquadBuilder();
	bool IsSquadBuilderOpen() const { return bShowSquadBuilder; }
	void PressSquadBuilder(float X, float Y);
	void ReleaseSquadBuilder(float X, float Y);
	void CancelSquadDrag();
	/** Wheel over Company, the squad column, or Plans. A key does not scroll. */
	void ScrollSquadBuilder(float X, float Y, float Wheel);
	/** Esc, F10, and every other close forget the filters and the drag. */
	void ForgetSquadBuilderPage();
	/** Keeps the boxes and the keys, refreshes who walks with the captain, and closes the page. */
	void ConfirmSquadBuilder();

	/** Town list opened from Use portal. */
	void OpenPortal();
	void ClosePortal();
	bool IsPortalOpen() const { return bShowPortal; }
	int32 GetPortalIndex() const { return PortalIndex; }
	void MovePortalSelection(int32 Delta);
	void SetPortalIndex(int32 Index);

	/** Exit Game asks before it closes. Yes is the first row. */
	void OpenExitConfirm();
	void CloseExitConfirm();
	bool IsExitConfirmOpen() const { return bShowExitConfirm; }
	int32 GetExitConfirmIndex() const { return ExitConfirmIndex; }
	void MoveExitConfirm(int32 Delta);
	void SetExitConfirmIndex(int32 Index);
	/** Closes the page. True when the selected row is Yes. */
	bool AcceptExitConfirm();

	/** A stronghold site asked for a type. Esc closes it and leaves the site. */
	void OpenStrongholdChoice(FIntPoint Cell, ESolidStrongholdSize Size, const TArray<ESolidStrongholdType>& Types);
	void CloseStrongholdChoice();
	bool IsStrongholdChoiceOpen() const { return bShowStrongholdChoice; }
	void MoveStrongholdChoice(int32 Delta);
	void SetStrongholdChoiceIndex(int32 Index);
	int32 GetStrongholdChoiceIndex() const { return StrongholdChoiceIndex; }
	int32 GetStrongholdChoiceCount() const { return StrongholdChoiceTypes.Num(); }
	FIntPoint GetStrongholdChoiceCell() const { return StrongholdChoiceCell; }
	ESolidStrongholdSize GetStrongholdChoiceSize() const { return static_cast<ESolidStrongholdSize>(StrongholdChoiceSize); }
	ESolidStrongholdType GetStrongholdChoiceType() const;

	/** F12. The tech readout in the corner. Battle plans stay up either way. */
	void ToggleDebugPanel();
	bool IsDebugPanelVisible() const { return bShowDebugPanel; }
	void SetDebugPanelVisible(bool bVisible) { bShowDebugPanel = bVisible; }

	/** One companion on a plain stage. Page Up and Page Down cycle the company. F10 returns to the world. */
	void OpenCharacterEditor();
	void CloseCharacterEditor();
	bool IsCharacterEditorOpen() const { return bShowCharacterEditor; }
	int32 GetCharacterEditorIndex() const { return CharacterEditorIndex; }
	void MoveCharacterEditor(int32 Delta);

	/** Up and Down wrap height, build, and gender. */
	void MoveCharacterEditorSlider(int32 Delta);
	int32 GetCharacterEditorSlider() const { return CharacterEditorSlider; }

	/**
	 * A press on a slider label selects it. A press on its track also starts a drag and writes OutFraction.
	 * Returns false when the point misses every slider.
	 */
	bool PressCharacterEditorSlider(float X, float Y, int32& OutSlider, bool& bBar, float& OutFraction);
	/** Continue a track drag from the horizontal position, including after the pointer leaves the row. */
	bool DragCharacterEditorSlider(float X, int32& OutSlider, float& OutFraction);
	void EndCharacterEditorSliderDrag();
	bool IsCharacterEditorSliderDragging() const { return bCharacterEditorSliderDrag; }

	/** Space. The stage draws the Viking skeleton as lines instead of the skinned body. */
	void SetCharacterEditorSkeleton(bool bShow);
	bool IsCharacterEditorSkeleton() const { return bCharacterEditorSkeleton; }

	/** Name of the bone under the pointer. Empty hides the tip. */
	void SetSkeletonBoneTip(const FString& Tip);
	const FString& GetSkeletonBoneTip() const { return SkeletonBoneTip; }

	/** F10. Closes an open page, including the character editor, or toggles the main menu. */
	void HandleMenuKey();
	/** Esc. Closes the menu, the exit confirmation, the portal, the squad builder, the journal, the inventory, the credits page, the character editor, and a stronghold type choice. */
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

	/** Catalog name of a pile within reach, or empty. A passage tip wins when both are set. */
	void SetForageTip(const FString& Tip);
	const FString& GetForageTip() const { return ForageTip; }

	/** An event message. A click, Enter, Esc, or F10 closes it. */
	void ShowMessage(const TCHAR* MessageId, const TCHAR* Text);
	bool IsMessageVisible() const { return bShowMessage; }
	const FString& GetMessageText() const { return MessageText; }
	const FString& GetMessageId() const { return MessageId; }
	void CloseMessage();

protected:
	void EnsureSightIndicator();
	void RemoveSightIndicator();
	void DrawCreditsPopup() const;
	void DrawJournalPopup() const;
	void DrawInventoryPopup();
	void DrawMessagePopup() const;
	void DrawCharacterEditorPanel();
	void DrawMainMenuPopup();
	void DrawPortalPopup();
	void DrawExitConfirm();
	void DrawStrongholdChoice();
	void DrawSquadBuilderPopup();
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

	/** Inventory page. Worn boxes and the captain's pack. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowInventory = false;

	/** Squad builder, opened from the main menu once two companions are allowed. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowSquadBuilder = false;

	/** Filters and scroll. Replaced when the page closes. */
	SolidSquadBuilder::FView SquadBuilderView;
	bool bSquadPageReady = false;
	SolidSquadBuilder::FPage SquadPage;
	float SquadOriginX = 0.f;
	float SquadOriginY = 0.f;
	bool bSquadDrag = false;
	SolidSquadBuilder::FDrag SquadDrag;

	/** F10 main menu. Off until toggled. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowMainMenu = false;

	/** Town list from Use portal. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowPortal = false;

	/** Highlighted town while the portal list is open. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 PortalIndex = 0;

	/** Exit Game confirmation. Yes and No. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	bool bShowExitConfirm = false;

	/** Highlighted row while the exit confirmation is open. Yes is 0. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 ExitConfirmIndex = 0;

	/** Type list for a stronghold site. Choosing a row commits the move. */
	bool bShowStrongholdChoice = false;
	int32 StrongholdChoiceIndex = 0;
	FIntPoint StrongholdChoiceCell = FIntPoint::ZeroValue;
	uint8 StrongholdChoiceSize = 0;
	TArray<uint8> StrongholdChoiceTypes;

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

	/** Selected height, build, or gender slider. */
	UPROPERTY(VisibleAnywhere, Category = "HUD")
	int32 CharacterEditorSlider = 0;

	/** Tracks drawn last frame. A click uses these. */
	TArray<FSolidSliderRow> CharacterEditorSliderRows;

	/** The track the pointer grabbed. */
	bool bCharacterEditorSliderDrag = false;
	int32 CharacterEditorSliderDragIndex = INDEX_NONE;

	/** Space is showing bone lines. Closing the editor returns to the body. */
	bool bCharacterEditorSkeleton = false;

	/** Hover name drawn beside the pointer while the skeleton is showing. */
	FString SkeletonBoneTip;

	/** Top-right eye / eagle widget has been added to the viewport. */
	bool bSightIndicatorAdded = false;

	/** Stronghold name and "Click to enter", "Leave stronghold", a lair line, or empty when the pointer is elsewhere. */
	FString CavePassageTip;

	/** Stick or Pitch, or empty when the pointer is not over a pile within reach. */
	FString ForageTip;

	/** Event message. Empty when none is showing. */
	bool bShowMessage = false;
	FString MessageId;
	FString MessageText;

	/** Inventory boxes drawn last frame, in content pixels, plus where that page sits. */
	bool bInventoryHitReady = false;
	bool bInventoryChestOpen = false;
	SolidInventory::FPage InventoryPage;
	float InventoryOriginX = 0.f;
	float InventoryOriginY = 0.f;

	/** An item following the pointer until it is dropped on another slot. */
	bool bInventoryDrag = false;
	SolidInventory::FSlotHit InventoryDragFrom;
	FString InventoryDragName;

	/** The pointer went down on the lit Craft button. */
	bool bCraftButtonPressed = false;
};
