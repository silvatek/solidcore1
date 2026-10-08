#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SolidCore1CompanionCharacter.generated.h"

class USkeletalMesh;
class UAnimSequence;

/**
 * Companion that steers toward a follow point behind the player.
 * Uses direct CharacterMovement (no NavMesh) so it works on procedural terrain.
 * Default visual is the Fab Viking (custom skeleton + clip anims).
 */
UCLASS()
class SOLIDCORE1_API ASolidCore1CompanionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ASolidCore1CompanionCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void SetFollowTarget(AActor* NewTarget);

	AActor* GetFollowTarget() const { return FollowTarget.Get(); }

	/** Desired distance behind the follow target (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "50.0"))
	float FollowDistance = 280.f;

	/** Stop steering when within this planar distance of the follow point (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "10.0"))
	float AcceptanceRadius = 90.f;

	/** Beyond this planar separation, use CatchUpSpeed (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow", meta = (ClampMin = "100.0"))
	float CatchUpDistance = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float WalkSpeed = 480.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float CatchUpSpeed = 850.f;

	/** Soft lateral offset so the companion is not glued to the player's exact trail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float SideOffset = 80.f;

	/** Planar speed above which the run clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Anim", meta = (ClampMin = "0.0"))
	float RunAnimSpeedThreshold = 600.f;

	/** Planar speed above which the walk clip plays (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Anim", meta = (ClampMin = "0.0"))
	float WalkAnimSpeedThreshold = 30.f;

protected:
	virtual void PostInitializeComponents() override;

	void ApplyVisuals();
	void ResolveFollowTarget();
	void UpdateFollow(float DeltaSeconds);
	void UpdateLocomotionAnim();
	UAnimSequence* LoadAnim(const TSoftObjectPtr<UAnimSequence>& SoftAnim, const TCHAR* FallbackPath) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Visual")
	TSoftObjectPtr<USkeletalMesh> CompanionMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float MeshGroundZOffset = -90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> IdleAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> WalkAnim;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Anim")
	TSoftObjectPtr<UAnimSequence> RunAnim;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FollowTarget;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> ActiveLocomotionAnim;
};
