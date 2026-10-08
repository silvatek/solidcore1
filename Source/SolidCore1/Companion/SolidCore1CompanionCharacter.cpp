#include "SolidCore1CompanionCharacter.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/SoftObjectPath.h"

namespace SolidCore1CompanionPrivate
{
	static UAnimSequence* LoadAnimPath(const TCHAR* Path)
	{
		return Cast<UAnimSequence>(StaticLoadObject(UAnimSequence::StaticClass(), nullptr, Path));
	}
}

ASolidCore1CompanionCharacter::ASolidCore1CompanionCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->JumpZVelocity = 700.f;
	Move->AirControl = 0.35f;
	Move->MaxWalkSpeed = WalkSpeed;
	Move->MinAnalogWalkSpeed = 20.f;
	Move->BrakingDecelerationWalking = 2000.f;
	Move->BrakingDecelerationFalling = 1500.f;
	Move->bRunPhysicsWithNoController = true;

	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.f, 0.f, MeshGroundZOffset), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetMesh()->SetVisibility(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	GetMesh()->bPauseAnims = false;
	GetMesh()->bNoSkeletonUpdate = false;

	// Fab Viking — custom skeleton; locomotion uses single-node clip playback.
	CompanionMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")));
	IdleAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1")));
	WalkAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk")));
	RunAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run")));
}

void ASolidCore1CompanionCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyVisuals();
}

void ASolidCore1CompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisuals();
	CacheLocomotionAnims();
	ResolveFollowTarget();
	UpdateLocomotionAnim();

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		FVector Rel = CharacterMesh->GetRelativeLocation();
		Rel.Z = MeshGroundZOffset;
		CharacterMesh->SetRelativeLocation(Rel);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[SolidCore1] Companion BeginPlay mesh=%s idle=%s walk=%s run=%s follow=%s"),
		GetMesh() && GetMesh()->GetSkeletalMeshAsset()
			? *GetMesh()->GetSkeletalMeshAsset()->GetName()
			: TEXT("<none>"),
		CachedIdleAnim ? *CachedIdleAnim->GetName() : TEXT("<null>"),
		CachedWalkAnim ? *CachedWalkAnim->GetName() : TEXT("<null>"),
		CachedRunAnim ? *CachedRunAnim->GetName() : TEXT("<null>"),
		FollowTarget.IsValid() ? *FollowTarget->GetName() : TEXT("<none>"));
	UE_LOG(LogSolidCore1, Warning,
		TEXT("Companion BeginPlay mesh=%s idle=%s walk=%s run=%s"),
		GetMesh() && GetMesh()->GetSkeletalMeshAsset()
			? *GetMesh()->GetSkeletalMeshAsset()->GetName()
			: TEXT("<none>"),
		CachedIdleAnim ? *CachedIdleAnim->GetName() : TEXT("<null>"),
		CachedWalkAnim ? *CachedWalkAnim->GetName() : TEXT("<null>"),
		CachedRunAnim ? *CachedRunAnim->GetName() : TEXT("<null>"));
}

void ASolidCore1CompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FollowTarget.IsValid())
	{
		ResolveFollowTarget();
	}

	UpdateFollow(DeltaSeconds);
	UpdateLocomotionAnim();
}

void ASolidCore1CompanionCharacter::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

void ASolidCore1CompanionCharacter::ResolveFollowTarget()
{
	if (FollowTarget.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (Pawn != this)
				{
					FollowTarget = Pawn;
				}
			}
		}
	}
}

void ASolidCore1CompanionCharacter::CacheLocomotionAnims()
{
	if (!CachedIdleAnim)
	{
		CachedIdleAnim = IdleAnim.LoadSynchronous();
		if (!CachedIdleAnim)
		{
			CachedIdleAnim = SolidCore1CompanionPrivate::LoadAnimPath(
				TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1"));
		}
	}
	if (!CachedWalkAnim)
	{
		CachedWalkAnim = WalkAnim.LoadSynchronous();
		if (!CachedWalkAnim)
		{
			CachedWalkAnim = SolidCore1CompanionPrivate::LoadAnimPath(
				TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk"));
		}
	}
	if (!CachedRunAnim)
	{
		CachedRunAnim = RunAnim.LoadSynchronous();
		if (!CachedRunAnim)
		{
			CachedRunAnim = SolidCore1CompanionPrivate::LoadAnimPath(
				TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run"));
		}
	}
}

void ASolidCore1CompanionCharacter::ApplyVisuals()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	if (!CharacterMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = CompanionMesh.LoadSynchronous();
		if (!LoadedMesh)
		{
			static const TCHAR* Fallbacks[] = {
				TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
			};
			for (const TCHAR* Path : Fallbacks)
			{
				LoadedMesh = Cast<USkeletalMesh>(
					StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, Path));
				if (LoadedMesh)
				{
					break;
				}
			}
		}

		if (LoadedMesh)
		{
			CharacterMesh->SetSkeletalMeshAsset(LoadedMesh);
			CharacterMesh->SetVisibility(true);
			CharacterMesh->SetHiddenInGame(false);
			CharacterMesh->SetCastShadow(true);
			UE_LOG(LogSolidCore1, Warning, TEXT("Companion mesh: %s"), *LoadedMesh->GetPathName());
		}
		else
		{
			UE_LOG(LogSolidCore1, Error, TEXT("Companion: no skeletal mesh found (Viking/Quinn)."));
			return;
		}
	}

	// Viking uses a custom skeleton — drive clips via single-node, not Manny's AnimBP.
	CharacterMesh->SetAnimInstanceClass(nullptr);
	CharacterMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	CharacterMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	CharacterMesh->bPauseAnims = false;
	CharacterMesh->bNoSkeletonUpdate = false;
	CharacterMesh->InitAnim(true);
}

bool ASolidCore1CompanionCharacter::PlayLocomotionClip(UAnimSequence* Anim)
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
		ActiveLocomotionAnim = Anim;
		return true;
	}

	// Fallback path used by some engine versions.
	CharacterMesh->PlayAnimation(Anim, true);
	if (UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance())
	{
		ActiveLocomotionAnim = Anim;
		return SingleNode->GetAnimationAsset() == Anim;
	}

	UE_LOG(LogSolidCore1, Error,
		TEXT("Companion: failed to create AnimSingleNodeInstance for %s"), *Anim->GetName());
	return false;
}

void ASolidCore1CompanionCharacter::UpdateLocomotionAnim()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	CacheLocomotionAnims();

	const float Speed = GetVelocity().Size2D();
	UAnimSequence* Desired = CachedIdleAnim;
	if (Speed >= RunAnimSpeedThreshold && CachedRunAnim)
	{
		Desired = CachedRunAnim;
	}
	else if (Speed >= WalkAnimSpeedThreshold && CachedWalkAnim)
	{
		Desired = CachedWalkAnim;
	}

	if (!Desired)
	{
		UE_LOG(LogSolidCore1, Warning, TEXT("Companion: no locomotion anim loaded (still T-pose)."));
		return;
	}

	// Retry until the single-node instance is actually playing this clip.
	const UAnimSingleNodeInstance* SingleNode = CharacterMesh->GetSingleNodeInstance();
	const bool bAlreadyPlaying =
		ActiveLocomotionAnim == Desired
		&& SingleNode
		&& SingleNode->GetAnimationAsset() == Desired
		&& SingleNode->IsPlaying();

	if (bAlreadyPlaying)
	{
		return;
	}

	PlayLocomotionClip(Desired);
}

void ASolidCore1CompanionCharacter::UpdateFollow(float /*DeltaSeconds*/)
{
	AActor* Target = FollowTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector TargetLoc = Target->GetActorLocation();
	const FVector TargetForward = Target->GetActorForwardVector();
	const FVector TargetRight = Target->GetActorRightVector();

	const FVector FollowPoint =
		TargetLoc
		- TargetForward * FollowDistance
		+ TargetRight * SideOffset;

	FVector ToFollow = FollowPoint - GetActorLocation();
	ToFollow.Z = 0.f;
	const float PlanarDist = ToFollow.Size();

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	if (PlanarDist <= AcceptanceRadius)
	{
		Move->MaxWalkSpeed = WalkSpeed;
		return;
	}

	Move->MaxWalkSpeed = (PlanarDist >= CatchUpDistance) ? CatchUpSpeed : WalkSpeed;
	AddMovementInput(ToFollow.GetSafeNormal(), 1.f);
}
