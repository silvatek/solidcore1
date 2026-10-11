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

void ASolidCharacter::CacheLocomotionAnims()
{
	if (!CachedIdleAnim)
	{
		CachedIdleAnim = SolidClipLocomotion::LoadClip(
			IdleAnim, SolidClipLocomotion::DefaultIdlePath);
	}
	if (!CachedWalkAnim)
	{
		CachedWalkAnim = SolidClipLocomotion::LoadClip(
			WalkAnim, SolidClipLocomotion::DefaultWalkPath);
	}
	if (!CachedRunAnim)
	{
		CachedRunAnim = SolidClipLocomotion::LoadClip(
			RunAnim, SolidClipLocomotion::DefaultRunPath);
	}
	if (!CachedJumpAnim)
	{
		CachedJumpAnim = SolidClipLocomotion::LoadClip(
			JumpAnim, SolidClipLocomotion::DefaultJumpPath);
	}
}

bool ASolidCharacter::PlayLocomotionClip(UAnimSequence* Anim)
{
	return SolidClipLocomotion::PlayLoopingClip(
		GetMesh(), Anim, ActiveLocomotionAnim, TEXT("Captain"));
}

void ASolidCharacter::UpdateLocomotionAnim()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	CacheLocomotionAnims();

	const UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	const bool bInAir = MoveComp && MoveComp->IsFalling();
	const float Speed = GetVelocity().Size2D();
	const bool bPreferRun =
		Speed >= RunAnimSpeedThreshold
		|| bIsSprinting
		|| (MoveComp && MoveComp->MaxWalkSpeed >= SprintSpeed - 1.f);

	const SolidClipLocomotion::EClip Kind = SolidClipLocomotion::SelectClip(
		Speed,
		WalkAnimSpeedThreshold,
		bPreferRun,
		bInAir,
		/*bAllowJump=*/CachedJumpAnim != nullptr);

	UAnimSequence* Desired = CachedIdleAnim;
	switch (Kind)
	{
	case SolidClipLocomotion::EClip::Jump:
		Desired = CachedJumpAnim ? CachedJumpAnim.Get() : Desired;
		break;
	case SolidClipLocomotion::EClip::Run:
		if (CachedRunAnim)
		{
			Desired = CachedRunAnim;
		}
		else if (CachedWalkAnim && Speed >= WalkAnimSpeedThreshold)
		{
			Desired = CachedWalkAnim;
		}
		break;
	case SolidClipLocomotion::EClip::Walk:
		Desired = CachedWalkAnim ? CachedWalkAnim.Get() : Desired;
		break;
	default:
		break;
	}

	if (!Desired)
	{
		return;
	}

	if (SolidClipLocomotion::IsPlayingClip(CharacterMesh, ActiveLocomotionAnim, Desired))
	{
		return;
	}

	PlayLocomotionClip(Desired);
}
