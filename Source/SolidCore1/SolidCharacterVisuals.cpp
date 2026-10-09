#include "SolidCharacter.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SolidClipLocomotion.h"

void ASolidCharacter::ApplyCharacterVisuals()
{
	SolidClipLocomotion::ApplyMeshAndSingleNodeMode(
		GetMesh(),
		DefaultSkeletalMesh,
		SolidClipLocomotion::DefaultMeshPath,
		TEXT("Captain"),
		/*bOnlyIfMeshUnset=*/false);
}

void ASolidCharacter::CacheVikingLocomotionAnims()
{
	if (!CachedVikingIdleAnim)
	{
		CachedVikingIdleAnim = SolidClipLocomotion::LoadClip(
			VikingIdleAnim, SolidClipLocomotion::DefaultIdlePath);
	}
	if (!CachedVikingWalkAnim)
	{
		CachedVikingWalkAnim = SolidClipLocomotion::LoadClip(
			VikingWalkAnim, SolidClipLocomotion::DefaultWalkPath);
	}
	if (!CachedVikingRunAnim)
	{
		CachedVikingRunAnim = SolidClipLocomotion::LoadClip(
			VikingRunAnim, SolidClipLocomotion::DefaultRunPath);
	}
	if (!CachedVikingJumpAnim)
	{
		CachedVikingJumpAnim = SolidClipLocomotion::LoadClip(
			VikingJumpAnim, SolidClipLocomotion::DefaultJumpPath);
	}
}

bool ASolidCharacter::PlayVikingLocomotionClip(UAnimSequence* Anim)
{
	return SolidClipLocomotion::PlayLoopingClip(
		GetMesh(), Anim, ActiveVikingLocomotionAnim, TEXT("Captain"));
}

void ASolidCharacter::UpdateVikingLocomotionAnim()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	CacheVikingLocomotionAnims();

	UAnimSequence* Desired = CachedVikingIdleAnim;
	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bInAir = MoveComp && MoveComp->IsFalling();
	if (bInAir && CachedVikingJumpAnim)
	{
		Desired = CachedVikingJumpAnim;
	}
	else
	{
		const float Speed = GetVelocity().Size2D();
		const bool bShouldRun =
			Speed >= VikingRunAnimSpeedThreshold
			|| bIsSprinting
			|| (MoveComp && MoveComp->MaxWalkSpeed >= SprintSpeed - 1.f);

		if (bShouldRun && CachedVikingRunAnim)
		{
			Desired = CachedVikingRunAnim;
		}
		else if (Speed >= VikingWalkAnimSpeedThreshold && CachedVikingWalkAnim)
		{
			Desired = CachedVikingWalkAnim;
		}
	}

	if (!Desired)
	{
		return;
	}

	if (SolidClipLocomotion::IsPlayingClip(CharacterMesh, ActiveVikingLocomotionAnim, Desired))
	{
		return;
	}

	PlayVikingLocomotionClip(Desired);
}
