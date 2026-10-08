#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "SolidCore1CompanionCharacter.generated.h"

class USkeletalMesh;
class UAnimInstance;

/**
 * Quinn companion that steers toward a follow point behind the player.
 * Uses direct CharacterMovement (no NavMesh) so it works on procedural terrain.
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

	/** Soft lateral offset so she is not glued to the player's exact trail. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Follow")
	float SideOffset = 80.f;

protected:
	virtual void PostInitializeComponents() override;

	void ApplyVisuals();
	void ResolveFollowTarget();
	void UpdateFollow(float DeltaSeconds);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Visual")
	TSoftObjectPtr<USkeletalMesh> CompanionMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Companion|Visual")
	TSoftClassPtr<UAnimInstance> CompanionAnimBlueprint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Companion|Visual")
	float MeshGroundZOffset = -90.f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> FollowTarget;
};
