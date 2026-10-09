#include "SolidCharacter.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SolidCore1.h"
#include "UObject/SoftObjectPath.h"

void ASolidCharacter::ApplyCharacterVisuals()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	USkeletalMesh* LoadedMesh = DefaultSkeletalMesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		LoadedMesh = Cast<USkeletalMesh>(
			StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")));
	}

	if (!LoadedMesh)
	{
		UE_LOG(LogSolid, Error, TEXT("Viking skeletal mesh missing (/Game/Viking/Mesh/SK_Viking)."));
		return;
	}

	if (CharacterMesh->GetSkeletalMeshAsset() != LoadedMesh)
	{
		CharacterMesh->SetSkeletalMeshAsset(LoadedMesh);
		UE_LOG(LogSolid, Warning, TEXT("Applied Captain mesh: %s"), *LoadedMesh->GetPathName());
	}

	CharacterMesh->SetVisibility(true);
	CharacterMesh->SetHiddenInGame(false);
	CharacterMesh->SetCastShadow(true);

	// Custom skeleton — single-node clip playback (not Epic AnimBP).
	CharacterMesh->SetAnimInstanceClass(nullptr);
	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	CharacterMesh->bPauseAnims = false;
	CharacterMesh->bNoSkeletonUpdate = false;
	CharacterMesh->InitAnim(true);
}

void ASolidCharacter::CacheVikingLocomotionAnims()
{
	auto LoadClip = [](TSoftObjectPtr<UAnimSequence>& Soft, const TCHAR* Path) -> UAnimSequence*
	{
		if (UAnimSequence* Loaded = Soft.LoadSynchronous())
		{
			return Loaded;
		}
		return Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr, Path));
	};

	if (!CachedVikingIdleAnim)
	{
		CachedVikingIdleAnim = LoadClip(
			VikingIdleAnim, TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1"));
	}
	if (!CachedVikingWalkAnim)
	{
		CachedVikingWalkAnim = LoadClip(
			VikingWalkAnim, TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk"));
	}
	if (!CachedVikingRunAnim)
	{
		CachedVikingRunAnim = LoadClip(
			VikingRunAnim, TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run"));
	}
	if (!CachedVikingJumpAnim)
	{
		CachedVikingJumpAnim = LoadClip(
			VikingJumpAnim, TEXT("/Game/Viking/Animations/Anim_Viking_jump.Anim_Viking_jump"));
	}
}

bool ASolidCharacter::PlayVikingLocomotionClip(UAnimSequence* Anim)
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !Anim)
	{
		return false;
	}

	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	CharacterMesh->InitAnim(true);

	if (UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance())
	{
		if (SingleNode->GetAnimationAsset() != Anim || !SingleNode->IsPlaying())
		{
			SingleNode->SetAnimationAsset(Anim, false);
			SingleNode->SetLooping(true);
			SingleNode->SetPlaying(true);
			SingleNode->SetPlayRate(1.f);
		}
		ActiveVikingLocomotionAnim = Anim;
		return true;
	}

	CharacterMesh->PlayAnimation(Anim, true);
	if (UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance())
	{
		ActiveVikingLocomotionAnim = Anim;
		return SingleNode->GetAnimationAsset() == Anim;
	}

	UE_LOG(LogSolid, Error, TEXT("Captain Viking: failed AnimSingleNodeInstance for %s"), *Anim->GetName());
	return false;
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

	const UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance();
	const bool bAlreadyPlaying =
		ActiveVikingLocomotionAnim == Desired
		&& SingleNode
		&& SingleNode->GetAnimationAsset() == Desired
		&& SingleNode->IsPlaying();
	if (bAlreadyPlaying)
	{
		return;
	}

	PlayVikingLocomotionClip(Desired);
}
