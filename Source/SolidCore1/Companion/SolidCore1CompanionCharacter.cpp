#include "SolidCore1CompanionCharacter.h"
#include "SolidCore1.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPath.h"

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

	// Quinn shares the Epic mannequin skeleton with Manny, so ABP_Unarmed drives her too.
	CompanionMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));
	CompanionAnimBlueprint = TSoftClassPtr<UAnimInstance>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));
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
	ResolveFollowTarget();

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		FVector Rel = CharacterMesh->GetRelativeLocation();
		Rel.Z = MeshGroundZOffset;
		CharacterMesh->SetRelativeLocation(Rel);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Companion Quinn BeginPlay follow=%s"),
		FollowTarget.IsValid() ? *FollowTarget->GetName() : TEXT("<none>"));
	UE_LOG(LogSolidCore1, Warning, TEXT("Companion Quinn BeginPlay follow=%s"),
		FollowTarget.IsValid() ? *FollowTarget->GetName() : TEXT("<none>"));
}

void ASolidCore1CompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FollowTarget.IsValid())
	{
		ResolveFollowTarget();
	}

	UpdateFollow(DeltaSeconds);
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
			static const TCHAR* QuinnFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"),
			};
			for (const TCHAR* Path : QuinnFallbacks)
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
			UE_LOG(LogSolidCore1, Error,
				TEXT("Companion: Quinn mesh not found under /Game/Characters/Mannequins."));
		}
	}

	if (CharacterMesh->GetSkeletalMeshAsset() && CharacterMesh->GetAnimClass() == nullptr)
	{
		UClass* AnimClass = CompanionAnimBlueprint.LoadSynchronous();
		if (!AnimClass)
		{
			static const TCHAR* AnimFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Unarmed.ABP_Unarmed_C"),
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"),
			};
			for (const TCHAR* Path : AnimFallbacks)
			{
				AnimClass = StaticLoadClass(UAnimInstance::StaticClass(), nullptr, Path);
				if (AnimClass)
				{
					break;
				}
			}
		}

		if (AnimClass)
		{
			CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			CharacterMesh->SetAnimInstanceClass(AnimClass);
			UE_LOG(LogSolidCore1, Warning, TEXT("Companion anim BP: %s"), *AnimClass->GetPathName());
		}
	}
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
