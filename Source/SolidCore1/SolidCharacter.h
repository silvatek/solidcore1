#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Party/SolidPartyDrill.h"
#include "SolidSight.h"
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
class ASolidHUD;

/**
 * Captain — the single player-controlled character.
 * Party camera framing lives in SolidPartyCamera.cpp;
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
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
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
	void StartPartyFormationDrill();
	void StopPartyFormationDrill();
	bool IsPartyFormationDrillActive() const;
	bool IsPartyFormationDrillTurning() const;
	int32 GetPartyFormationDrillLeg() const { return PartyFormationDrill.CurrentLeg; }
	float GetPartyFormationDrillPhaseRemaining() const { return PartyFormationDrill.PhaseSecondsRemaining; }

	/** Mouse-wheel boom limit (cm). Fog curtains are taller than this. */
	float GetCameraZoomMax() const { return CameraZoomMax; }

	/** Raven sight (third person) or true sight (first person). F9 toggles once both are enabled. */
	ESolidSight GetSight() const { return Sight; }
	bool IsTrueSight() const { return SolidSight::IsTrueSight(Sight); }
	void SetSight(ESolidSight NewSight);
	void ToggleSight();

protected:
	void ApplyNameLabel();
	void BeginPartyFormationDrillLeg();
	void BeginPartyFormationDrillWalk();
	void ApplyPartyFormationDrillYaw(float YawDegrees);
	void TickPartyFormationDrill(float DeltaTime);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	/** Right-button look. The free pointer does not turn the camera. */
	void UpdatePointerMouseLook();
	void Zoom(const FInputActionValue& Value);
	void StartJump();
	void StopJumpFromInput();
	void StartSprint();
	void StopSprint();
	bool IsMenuOverlayOpen() const;
	ASolidHUD* GetSolidHUD() const;

	/** Select Party assigned battle-plan slot (0 = F1 … 7 = F8). */
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
	void ToggleSightFromInput();
	void ApplySightPresentation();
	void ApplyTrueSightCamera();
	void ApplyRavenSightCamera();
	void ToggleMainMenuFromInput();
	void ToggleDebugPanelFromInput();
	void CloseMenuOverlayFromInput();
	void MainMenuMoveUp();
	void MainMenuMoveDown();
	void MainMenuConfirm();
	void MainMenuChoose1();
	void MainMenuChoose2();
	void MainMenuChoose3();
	void ChooseMainMenuIndex(int32 Index);

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
	 * Party camera: pull the boom toward the Party center and optionally lengthen
	 * so the Captain and active Companions stay framed. (Company is not framed.)
	 */
	void UpdatePartyCameraFraming(float DeltaTime);

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

	/** When true, camera frames the Party (Captain + Companions currently in the world). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bFrameCompanions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "200.0"))
	float FramingMinArmLength = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "400.0"))
	float FramingMaxArmLength = 4000.f;

	/** Extra world centimeters outside the projected Party extents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.0"))
	float FramingPadding = 280.f;

	/** Zoom-out speed when the Party needs a wider shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomOutSpeed = 8.f;

	/** Zoom-in speed (kept low so terrain boom-collision / brief gaps do not pop Companions out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomInSpeed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingOffsetInterpSpeed = 6.f;

	/**
	 * Spring-arm collision against hills collapses TargetArmLength and clips Companions.
	 * When true, collision probes are disabled while any Companion is in the Party frame.
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
	 * When true, Party framing may lengthen the boom past zoom.
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
	SolidPartyDrill::FState PartyFormationDrill;
};
