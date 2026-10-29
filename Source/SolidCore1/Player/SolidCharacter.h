#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Company/SolidSquadDrill.h"
#include "SolidSight.h"
#include "Lairs/SolidLair.h"
#include "SolidCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMesh;
class UAnimSequence;
class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class ASolidCharacterStage;
class ASolidHUD;
class ASolidForagePile;
class ASolidStrongholdSite;
class ASolidNameSign;

/**
 * Captain — the single player-controlled character.
 * Squad camera framing lives in SolidSquadCamera.cpp;
 * visuals/clip selection in SolidCharacterVisuals.cpp (shared play via SolidClipLocomotion);
 * runtime input in SolidCharacterInput.cpp.
 */
UCLASS(config = Game)
class SOLIDCORE1_API ASolidCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASolidCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE float GetUserZoomArmLength() const { return UserZoomArmLength; }
	FORCEINLINE USceneComponent* GetNameLabelRoot() const { return NameLabelRoot; }
	FORCEINLINE UTextRenderComponent* GetNameLabel() const { return NameLabel; }
	FORCEINLINE UStaticMeshComponent* GetNameLabelBorder() const { return NameLabelBorder; }
	FORCEINLINE UStaticMeshComponent* GetNameLabelBackground() const { return NameLabelBackground; }

	/** Shown on the floating nameplate (not always "Captain"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Name")
	FString CharacterDisplayName = TEXT("Outcast");

	void SetCharacterDisplayName(const FString& NewName);
	const FString& GetCharacterDisplayName() const { return CharacterDisplayName; }

	/** Square-walk formation demo (main menu: Test Drill). Cycles F1–F4, 1.5s per side. */
	void StartSquadFormationDrill();
	void StopSquadFormationDrill();
	bool IsSquadFormationDrillActive() const;
	bool IsSquadFormationDrillTurning() const;
	int32 GetSquadFormationDrillLeg() const { return SquadFormationDrill.CurrentLeg; }
	float GetSquadFormationDrillPhaseRemaining() const { return SquadFormationDrill.PhaseSecondsRemaining; }

	/** Mouse-wheel boom limit (cm). Fog curtains are taller than this. */
	float GetCameraZoomMax() const { return CameraZoomMax; }

	/** F9 cycles the sights that are unlocked. */
	ESolidSight GetSight() const { return Sight; }
	bool IsTrueSight() const { return SolidSight::IsTrueSight(Sight); }
	bool IsFalconSight() const { return SolidSight::IsFalconSight(Sight); }
	/** Entering true sight levels the camera. Yaw stays. */
	void SetSight(ESolidSight NewSight);
	/** Raven sight, this look, and this boom length. */
	void AimRaven(float PitchDeg, float YawDeg, float ArmCm);
	/** Raven-sight left click. Cleared on arrival, WASD, or true sight. */
	bool HasClickMoveDestination() const { return bHasClickMoveDestination; }
	FVector GetClickMoveDestination() const { return ClickMoveDestination; }
	/** Raven-sight walk toward a ground point. True sight ignores it. */
	void SetClickMoveDestination(const FVector& Destination);
	/**
	 * One frame of a mapped key, such as W.
	 * The mapping's modifiers run, then the bound handler. Hardware input stays ignored while the script runs.
	 */
	void SimulateKey(FKey Key);
	/**
	 * One step of a right-button drag toward this yaw. Pitch comes level.
	 * MaxDegrees caps each axis this frame, so the turn sweeps.
	 * The button stays down until the view arrives, or until ReleaseRightMouse.
	 */
	void DragRightMouse(float YawDegrees, float MaxDegrees);
	/** Lets the right button up after a script drag. */
	void ReleaseRightMouse();
	bool IsScriptRightMouseHeld() const { return bScriptRightMouse; }
	/** Move the pointer and press the left button. The click handler runs. */
	void SimulateLeftClick(float ScreenX, float ScreenY);
	/** Open the site stone with the unlocked choices. */
	void OfferStrongholdSite(const ASolidStrongholdSite* Site);
	/** Build the selected size and type, then travel into that stronghold. */
	void ConfirmStrongholdChoice();
	/**
	 * The main menu, the exit confirmation, and the portal follow the pointer.
	 * The site page keeps its pick until a click.
	 */
	static bool MenuSelectsOnHover(
		bool bMainMenu,
		bool bExitConfirm,
		bool bPortal,
		bool bStrongholdChoice);
	/** The stick a move key produces after its mapping modifiers. W is (0, 1), forward. */
	bool StickForKey(FKey Key, FVector2D& OutStick);
	/** Move the captain onto a landing. Pitch 0 is a level camera. Clears a click-walk and a formation drill. */
	void ArriveByPortal(const FVector& NewLocation, float YawDegrees, float PitchDegrees = 0.f);
	void ToggleSight();
	/** Hide the nameplate in true sight, in a stronghold or lair, and while no companions are out. */
	void UpdateNameLabelVisibility();
	/**
	 * Hide the captain while arrange mode is on.
	 * The pawn stays, so the camera does not jump. His mesh and capsule miss the pointer ray.
	 */
	void SetArrangeHidden(bool bHide);

protected:
	void ApplyNameLabel();
	void BeginSquadFormationDrillLeg();
	void BeginSquadFormationDrillWalk();
	void ApplySquadFormationDrillYaw(float YawDegrees);
	void TickSquadFormationDrill(float DeltaTime);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;
	virtual void OnRep_PlayerState() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	/** WASD in true sight. X is right and Y is forward, both along the control yaw. */
	void AddTrueSightMove(const FVector2D& Stick);
	/** Apply a script look before the following step reads the view. */
	void FlushScriptLook();
	bool ValueForMappedKey(FKey Key, const UInputAction*& OutAction, FVector2D& OutStick) const;
	/** Right-button look. The free pointer does not turn the camera. */
	void UpdatePointerMouseLook();
	void ClickMoveFromInput();
	void PollClickMovePress();
	void UpdateCharacterEditorPointer();
	void UpdateInventoryPointer();
	void UpdateSquadBuilderPointer();
	void UpdateMenuPointerHover();
	void UpdateCavePassageHover();
	bool ReadPointerPosition(float& OutX, float& OutY) const;
	bool ReadPointerRay(FVector& OutOrigin, FVector& OutDirection) const;
	bool IsPointerShown() const;
	SolidLair::FPassageHit PassageUnderPointer() const;
	SolidLair::FPassageHit PassageOverlapping(const FVector& Location) const;
	/** Enter is ignored until that place is open. A previous stronghold is not an enter. Leave stays available. */
	SolidLair::FPassageHit AllowPassage(const SolidLair::FPassageHit& Hit) const;
	void ActivatePassage(const SolidLair::FPassageHit& Hit);
	/** A stronghold site under the pointer, when a click may ask it for a type. */
	ASolidStrongholdSite* SiteUnderPointer() const;
	/** The storage container under the pointer, or INDEX_NONE. A fireplace and a bed are none. */
	int32 StorageContainerUnderPointer() const;
	/** The company stronghold's name sign. A click opens the name and does not enter. */
	ASolidNameSign* NameSignUnderPointer() const;
	/** Nearest forage pile along the pointer ray. Ground distance is the captain's, in cm. */
	ASolidForagePile* NearestForagePile(float& OutGroundDistanceCm) const;
	void TickClickMove();
	void ClearClickMove();
	bool ResolveClickMoveDestination(const FVector& RayOrigin, const FVector& RayDirection, FVector& OutDestination) const;
	bool SampleGroundZ(float X, float Y, float& OutZ) const;
	void Zoom(const FInputActionValue& Value);
	void StartJump();
	void StopJumpFromInput();
	void StartSprint();
	void StopSprint();
	bool IsMenuOverlayOpen() const;
	ASolidHUD* GetSolidHUD() const;

	/** Select Squad assigned battle-plan slot (0 = F1 … 7 = F8). */
	void SelectBattlePlanSlot(int32 SlotIndex);
	void SelectBattlePlanSlotFromInput(int32 SlotIndex);
	void SelectBattlePlanSlot1();
	void SelectBattlePlanSlot2();
	void SelectBattlePlanSlot3();
	void SelectBattlePlanSlot4();
	void SelectBattlePlanSlot5();
	void SelectBattlePlanSlot6();
	void SelectBattlePlanSlot7();
	void SelectBattlePlanSlot8();
	/** Face button while a shoulder is held. Bottom, right, left, top. */
	void SelectBattlePlanFaceBottom();
	void SelectBattlePlanFaceRight();
	void SelectBattlePlanFaceLeft();
	void SelectBattlePlanFaceTop();
	void SelectBattlePlanFromFace(uint8 FaceIndex);
	void ToggleSightFromInput();
	void ApplySightPresentation();
	/** In a lair the body does not cast a shadow, so a torch inside him can light the stone. */
	void UpdateBodyShadow();
	void ApplyTrueSightCamera();
	void ApplyRavenSightCamera();
	void ApplyFalconSightCamera();
	/** Falcon Sight. The view stays on the captain. */
	void AimFalconAtCaptain();
	void ToggleMainMenuFromInput();
	bool IsAutoPlaythroughRunning() const;
	/** Player Esc and F10. True when a running script was stopped. */
	bool StopAutoPlaythrough();
	void ToggleInventoryFromSlash();
	void ToggleInventoryFromController();
	void ToggleDebugPanelFromInput();
	/** Alt-P. One picture of the Play In Editor view, in Saved/Playshots. */
	void TakePlayshot();
	void CloseMenuOverlayFromInput();
	void ToggleArrangeFromInput();
	void TurnArrangeClockwise();
	void TurnArrangeAnticlockwise();
	bool IsArrangingFurniture() const;
	/** The Arrange furniture plate under the pointer, including while the right button looks. */
	bool ArrangeSignUnderPointer() const;
	/** A letter, digit, space, or Backspace while the name page is open. */
	void NameSignKey(FKey Key);
	void OpenStrongholdName();
	/** The board shows the stronghold's name, and the pole stays clear of the opening. */
	void RefreshStrongholdNameSign();
	void MainMenuMoveUp();
	void MainMenuMoveDown();
	void MainMenuConfirm();
	void CharacterEditorPrevious();
	void CharacterEditorNext();
	/** LB and L1. While the inventory is open, the previous storage container. */
	void InventoryContainerPrevious();
	/** RB and R1. While the inventory is open, the next storage container. */
	void InventoryContainerNext();
	void CharacterEditorNudgeLeft();
	void CharacterEditorNudgeRight();
	void ToggleCharacterEditorSkeleton();
	void TurnCharacterEditorClockwise();
	void TurnCharacterEditorAnticlockwise();
	void MoveCharacterEditor(int32 Delta);
	void NudgeCharacterEditorSlider(int32 Direction);
	void ApplyEditorSliderFraction(int32 Slider, float Fraction);
	void PushEditorBodyToSquad(const FString& Name, float Height, float Build, float Gender);
	void SyncCharacterStage();
	void SetEditorWorldHidden(bool bHideWorld);
	void SetEditorCameraLocked(bool bLocked);
	int32 CurrentMaxSquadSize() const;
	void MainMenuChoose1();
	void MainMenuChoose2();
	void MainMenuChoose3();
	void MainMenuChoose4();
	void MainMenuChoose5();
	void MainMenuChoose6();
	void MainMenuChoose7();
	void MainMenuChoose8();
	void ChooseMainMenuIndex(int32 Index);
	void ConfirmPortalSelection();
	void ConfirmExitSelection();

	void ApplyWalkSpeed() const;
	void AddMappingContext();

	/** Builds transient Enhanced Input assets when Content assets are not assigned (playable out of the box). */
	void EnsureRuntimeInputAssets();

	/** Applies mesh / single-node locomotion setup (Captain). */
	void ApplyCharacterVisuals();

	/** Applies MeshGroundZOffset while preserving BP yaw/pitch/roll. */
	void ApplyMeshGroundOffset();

	void CacheLocomotionAnims();
	void UpdateLocomotionAnim();
	bool PlayLocomotionClip(UAnimSequence* Anim);

	/**
	 * Squad camera: pull the boom toward the Squad center and optionally lengthen
	 * so the Captain and active Companions stay framed. (Company is not framed.)
	 */
	void UpdateSquadCameraFraming(float DeltaTime);

	/** Lift the boom socket so the camera stays above procedural terrain (no arm collapse). */
	void ClampCameraAboveTerrain(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NameLabelRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NameLabelBorder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NameLabelBackground;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Character|Name", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextRenderComponent> NameLabel;

	/** When true, camera frames the Squad (Captain + Companions currently in the world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bFrameCompanions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "200.0"))
	float FramingMinArmLength = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "400.0"))
	float FramingMaxArmLength = 4000.f;

	/** Extra world centimeters outside the projected Squad extents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.0"))
	float FramingPadding = 280.f;

	/** A companion farther than this is left out of the frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "200.0"))
	float FramingMaxMemberCm = 2500.f;

	/** Zoom-out speed when the Squad needs a wider shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomOutSpeed = 8.f;

	/** Zoom-in speed (kept low so terrain boom-collision / brief gaps do not pop Companions out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomInSpeed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingOffsetInterpSpeed = 6.f;

	/**
	 * Spring-arm collision against hills collapses TargetArmLength and clips Companions.
	 * When true, collision probes are disabled while any Companion is in the Squad frame.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bDisableBoomCollisionWhileFraming = true;

	/** Assumed viewport aspect when computing horizontal FOV fit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "1.0"))
	float FramingAspectRatio = 16.f / 9.f;

	/** Minimum camera height above sampled terrain surface (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Terrain", meta = (ClampMin = "0.0"))
	float CameraTerrainClearance = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Terrain", meta = (ClampMin = "0.1"))
	float CameraTerrainLiftSpeed = 10.f;

	/** Hard cap on terrain lift so hill samples cannot fling the boom (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Terrain", meta = (ClampMin = "0.0"))
	float CameraTerrainLiftMax = 280.f;

	/**
	 * When true, Squad framing may lengthen the boom past zoom.
	 * When false (default), mouse-wheel zoom fully controls arm length; framing only recenters.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bFramingCanOverrideZoom = false;

	/** Player-chosen boom length (cm); mouse wheel adjusts this. Framing may only pull farther out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "100.0"))
	float UserZoomArmLength = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "100.0"))
	float CameraZoomMin = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "300.0"))
	float CameraZoomMax = 5000.f;

	/** Arm cm change per mouse-wheel notch (MouseWheelAxis is typically ±1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Zoom", meta = (ClampMin = "1.0"))
	float CameraZoomStep = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	/** Additive context so zoom works even when a Content IMC is assigned without wheel bindings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> ZoomMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	/** Mouse axes. Separate from LookAction so a free pointer does not turn the camera. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ZoomAction;

	/** Default skeletal mesh soft ptr (Fab Viking unless overridden). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> RunAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> JumpAnim;

	/** Planar speed above which the run clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Anim", meta = (ClampMin = "0.0"))
	float RunAnimSpeedThreshold = 380.f;

	/** Planar speed above which the walk clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Anim", meta = (ClampMin = "0.0"))
	float WalkAnimSpeedThreshold = 30.f;

	/** Mesh relative Z (feet at capsule bottom ≈ -capsule half-height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Visual")
	float MeshGroundZOffset = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f;

	bool bIsSprinting = false;

	/** Smoothed world-Z lift applied via spring-arm SocketOffset (keeps camera above terrain). */
	float CameraTerrainLiftCm = 0.f;

	/** True sight until an event enables raven sight. */
	ESolidSight Sight = ESolidSight::True;

	/** Ground point chosen by a raven-sight left click. */
	bool bHasClickMoveDestination = false;
	FVector ClickMoveDestination = FVector::ZeroVector;
	/** Set while the script is inside a simulated key, look, or click. Hardware input stays ignored. */
	bool bScriptInput = false;
	/** Left button held by SimulateLeftClick, for the click poll in that same call. */
	bool bScriptClickDown = false;
	/** Right button held by a script drag. Released when that drag stops. */
	bool bScriptRightMouse = false;
	/** Previous frame's left button, so one click is a single press. */
	bool bLeftMouseWasDown = false;
	/** The name page hears a key once per frame. Shift and the plain chord can both match. */
	FKey NameSignKeySeen;
	uint64 NameSignKeyFrame = 0;
	/** False until the left button has been up on this pawn. A carried hold does not click. */
	bool bClickSawRelease = false;
	/** Previous frame's left button while the inventory page is open. */
	bool bInventoryPointerWasDown = false;
	/** Previous frame's left button while the squad builder is open. */
	bool bSquadPointerWasDown = false;
	/** Last pointer sample while a choice list was open. A still pointer does not fight the keys. */
	bool bMenuPointerSampled = false;
	FVector2D MenuPointerSample = FVector2D::ZeroVector;
	int32 MenuPointerPage = 0;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedIdleAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedWalkAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedRunAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedJumpAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveLocomotionAnim;

	/** Square-walk formation demo state. */
	SolidSquadDrill::FState SquadFormationDrill;

	/** Plain room and one companion, shown from the F10 character editor. */
	UPROPERTY(Transient)
	TObjectPtr<ASolidCharacterStage> CharacterStage;

	bool bEditorWorldHidden = false;
	bool bEditorCameraLocked = false;
	bool bEditorSavedBoomCollision = true;
	FRotator EditorSavedControlRotation = FRotator::ZeroRotator;

	/** Arrange mode has hidden this pawn. The cell under him does not change. */
	bool bArrangeHidden = false;
	bool bArrangeCollisionSaved = false;
	ECollisionResponse SavedCapsuleVisibility = ECR_Block;
	ECollisionResponse SavedMeshVisibility = ECR_Block;
};
