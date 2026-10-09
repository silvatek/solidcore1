#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SolidCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMesh;
class UAnimInstance;
class UAnimSequence;

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

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Zoom(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();

	void ApplyWalkSpeed() const;
	void AddMappingContext();

	/** Builds transient Enhanced Input assets when Content assets are not assigned (playable out of the box). */
	void EnsureRuntimeInputAssets();

	/** Applies Viking mesh / locomotion setup. */
	void ApplyCharacterVisuals();

	/** Applies MeshGroundZOffset while preserving BP yaw/pitch/roll. */
	void ApplyMeshGroundOffset();

	void CacheVikingLocomotionAnims();
	void UpdateVikingLocomotionAnim();
	bool PlayVikingLocomotionClip(UAnimSequence* Anim);

	/** Pull the boom toward the group center and lengthen it so companions stay framed. */
	void UpdateGroupCameraFraming(float DeltaTime);

	/** Lift the boom socket so the camera stays above procedural terrain (no arm collapse). */
	void ClampCameraAboveTerrain(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** When true, camera frames this pawn plus all Solid companions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bFrameCompanions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "200.0"))
	float FramingMinArmLength = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "400.0"))
	float FramingMaxArmLength = 4000.f;

	/** Extra world centimeters outside the projected group extents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.0"))
	float FramingPadding = 280.f;

	/** Zoom-out speed when the group needs a wider shot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomOutSpeed = 8.f;

	/** Zoom-in speed (kept low so terrain boom-collision / brief gaps do not pop companions out). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingZoomInSpeed = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingOffsetInterpSpeed = 6.f;

	/**
	 * Spring-arm collision against hills collapses TargetArmLength and clips companions.
	 * When true, collision probes are disabled while any companion is present.
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
	 * When true, companion framing may lengthen the boom past zoom.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ZoomAction;

	/** Fab Viking skeletal mesh (required). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> VikingIdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> VikingWalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> VikingRunAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Anim")
	TSoftObjectPtr<UAnimSequence> VikingJumpAnim;

	/** Planar speed above which the run clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Anim", meta = (ClampMin = "0.0"))
	float VikingRunAnimSpeedThreshold = 380.f;

	/** Planar speed above which the walk clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Anim", meta = (ClampMin = "0.0"))
	float VikingWalkAnimSpeedThreshold = 30.f;

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

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedVikingIdleAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedVikingWalkAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedVikingRunAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CachedVikingJumpAnim;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveVikingLocomotionAnim;
};
