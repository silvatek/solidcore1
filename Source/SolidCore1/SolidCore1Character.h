#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "SolidCore1Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class USkeletalMesh;
class UAnimInstance;

UCLASS(config = Game)
class SOLIDCORE1_API ASolidCore1Character : public ACharacter
{
	GENERATED_BODY()

public:
	ASolidCore1Character();

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void StartSprint();
	void StopSprint();

	void ApplyWalkSpeed() const;
	void AddMappingContext();

	/** Builds transient Enhanced Input assets when Content assets are not assigned (playable out of the box). */
	void EnsureRuntimeInputAssets();

	/** Loads mannequin mesh / anim BP from soft paths when the mesh is still empty. */
	void ApplyCharacterVisuals();

	/** Applies MeshGroundZOffset while preserving BP yaw/pitch/roll. */
	void ApplyMeshGroundOffset();

	/** Pull the boom toward the group center and lengthen it so companions stay framed. */
	void UpdateGroupCameraFraming(float DeltaTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	/** When true, camera frames this pawn plus all SolidCore1 companions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing")
	bool bFrameCompanions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "200.0"))
	float FramingMinArmLength = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "400.0"))
	float FramingMaxArmLength = 1400.f;

	/** Extra world centimeters added outside the group bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.0"))
	float FramingPadding = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Framing", meta = (ClampMin = "0.1"))
	float FramingInterpSpeed = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SprintAction;

	/** Defaults to Epic Third Person mannequin paths; assign in defaults if you use different content. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftObjectPtr<USkeletalMesh> DefaultSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Visual")
	TSoftClassPtr<UAnimInstance> DefaultAnimBlueprint;

	/** Mesh relative Z (mannequin feet at capsule bottom ≈ -capsule half-height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Visual")
	float MeshGroundZOffset = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 900.f;

	bool bIsSprinting = false;
};
