#include "SolidCompanionCharacter.h"
#include "SolidClipLocomotion.h"
#include "SolidCore1.h"
#include "SolidGameMode.h"
#include "SolidNameLabel.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidParty.h"
#include "GameFramework/GameModeBase.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPath.h"

ASolidCompanionCharacter::ASolidCompanionCharacter()
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

	// Default mesh/clips are Fab Viking; Party members may override CompanionMesh later.
	CompanionMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));
	IdleAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultIdlePath));
	WalkAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultWalkPath));
	RunAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultRunPath));

	NameLabelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NameLabelRoot"));
	NameLabelRoot->SetupAttachment(RootComponent);
	NameLabelBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBorder"));
	NameLabelBorder->SetupAttachment(NameLabelRoot);
	NameLabelBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBackground"));
	NameLabelBackground->SetupAttachment(NameLabelRoot);
	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(NameLabelRoot);
	ApplyNameLabel();
}

void ASolidCompanionCharacter::ApplyNameLabel()
{
	const float CapsuleHalf = GetCapsuleComponent()
		? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()
		: 96.f;
	SolidNameLabel::Configure(
		NameLabelRoot,
		NameLabelBorder,
		NameLabelBackground,
		NameLabel,
		CharacterDisplayName,
		SolidNameLabel::EStyle::Companion,
		CapsuleHalf,
		this);
}

void ASolidCompanionCharacter::SetCharacterDisplayName(const FString& NewName)
{
	CharacterDisplayName = NewName;
	ApplyNameLabel();
}

void ASolidCompanionCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyVisuals();
}

void ASolidCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyVisuals();
	ApplyNameLabel();
	CacheLocomotionAnims();
	ResolveFollowTarget();
	UpdateLocomotionAnim();

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		FVector Rel = CharacterMesh->GetRelativeLocation();
		Rel.Z = MeshGroundZOffset;
		CharacterMesh->SetRelativeLocation(Rel);
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Companion BeginPlay mesh=%s idle=%s walk=%s run=%s follow=%s"),
		GetMesh() && GetMesh()->GetSkeletalMeshAsset()
			? *GetMesh()->GetSkeletalMeshAsset()->GetName()
			: TEXT("<none>"),
		CachedIdleAnim ? *CachedIdleAnim->GetName() : TEXT("<null>"),
		CachedWalkAnim ? *CachedWalkAnim->GetName() : TEXT("<null>"),
		CachedRunAnim ? *CachedRunAnim->GetName() : TEXT("<null>"),
		FollowTarget.IsValid() ? *FollowTarget->GetName() : TEXT("<none>"));
}

void ASolidCompanionCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FollowTarget.IsValid())
	{
		ResolveFollowTarget();
	}

	UpdateFollow(DeltaSeconds);
	UpdateLocomotionAnim();
	SolidNameLabel::FaceViewCamera(NameLabelRoot, GetWorld());
}

void ASolidCompanionCharacter::SetFollowTarget(AActor* NewTarget)
{
	FollowTarget = NewTarget;
}

void ASolidCompanionCharacter::ResolveFollowTarget()
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

void ASolidCompanionCharacter::CacheLocomotionAnims()
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
}

void ASolidCompanionCharacter::ApplyVisuals()
{
	SolidClipLocomotion::ApplyMeshAndSingleNodeMode(
		GetMesh(),
		CompanionMesh,
		SolidClipLocomotion::DefaultMeshPath,
		TEXT("Companion"),
		/*bOnlyIfMeshUnset=*/true);
}

bool ASolidCompanionCharacter::PlayLocomotionClip(UAnimSequence* Anim)
{
	return SolidClipLocomotion::PlayLoopingClip(
		GetMesh(), Anim, ActiveLocomotionAnim, TEXT("Companion"));
}

void ASolidCompanionCharacter::UpdateLocomotionAnim()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterMesh->GetSkeletalMeshAsset())
	{
		return;
	}

	CacheLocomotionAnims();

	const float Speed = GetVelocity().Size2D();
	float PlayerSpeed = 0.f;
	if (const AActor* Target = FollowTarget.Get())
	{
		PlayerSpeed = Target->GetVelocity().Size2D();
	}

	// Prefer run when clearly running/sprinting (walk≈500, sprint≈900).
	const bool bPreferRun =
		Speed >= RunAnimSpeedThreshold
		|| PlayerSpeed >= RunAnimSpeedThreshold
		|| (GetCharacterMovement() && GetCharacterMovement()->MaxWalkSpeed >= CatchUpSpeed - 1.f);

	const SolidClipLocomotion::EClip Kind = SolidClipLocomotion::SelectClip(
		Speed,
		WalkAnimSpeedThreshold,
		bPreferRun,
		/*bInAir=*/false,
		/*bAllowJump=*/false);

	UAnimSequence* Desired = CachedIdleAnim;
	switch (Kind)
	{
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
		UE_LOG(LogSolid, Warning, TEXT("Companion: no locomotion anim loaded (still T-pose)."));
		return;
	}

	if (SolidClipLocomotion::IsPlayingClip(CharacterMesh, ActiveLocomotionAnim, Desired))
	{
		return;
	}

	PlayLocomotionClip(Desired);
}

void ASolidCompanionCharacter::UpdateFollow(const float DeltaSeconds)
{
	AActor* Target = FollowTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector TargetLoc = Target->GetActorLocation();
	const FVector TargetForward = Target->GetActorForwardVector();
	const FVector TargetRight = Target->GetActorRightVector();

	float AlongForward = -FollowDistance;
	float AlongRight = SideOffset;
	ESolidBattleFormation Formation = ESolidBattleFormation::Line;
	if (UWorld* World = GetWorld())
	{
		if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (const USolidParty* Party = GameMode->GetParty())
			{
				Formation = Party->GetActiveFormation();
				const int32 CompanionCount = FMath::Max(GameMode->GetCompanions().Num(), 1);
				const FVector2D Slot = SolidBattleFormationSlots::SlotOffset(
					Formation,
					PartySlotIndex,
					CompanionCount,
					Party->GetActiveSpacing());
				AlongForward = Slot.X;
				AlongRight = Slot.Y;
			}
		}
	}

	const FVector FollowPoint =
		TargetLoc
		+ TargetForward * AlongForward
		+ TargetRight * AlongRight;

	FVector ToFollow = FollowPoint - GetActorLocation();
	ToFollow.Z = 0.f;
	const float PlanarDist = ToFollow.Size();

	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	float DesiredMaxSpeed = WalkSpeed;
	if (bMatchFollowTargetSpeed)
	{
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			if (const UCharacterMovementComponent* TargetMove = TargetCharacter->GetCharacterMovement())
			{
				DesiredMaxSpeed = FMath::Max(WalkSpeed, TargetMove->MaxWalkSpeed);
			}
		}
	}
	if (PlanarDist >= CatchUpDistance)
	{
		DesiredMaxSpeed = FMath::Max(DesiredMaxSpeed, CatchUpSpeed);
	}

	const bool bFaceCaptain = SolidBattleFormationSlots::ShouldFaceCaptain(
		Formation, PlanarDist, AcceptanceRadius);
	Move->bOrientRotationToMovement = !bFaceCaptain;
	if (bFaceCaptain)
	{
		const float Yaw = SolidBattleFormationSlots::YawFacingPoint(
			FVector2D(GetActorLocation().X, GetActorLocation().Y),
			FVector2D(TargetLoc.X, TargetLoc.Y));
		const FRotator Desired(0.f, Yaw, 0.f);
		SetActorRotation(FMath::RInterpTo(
			GetActorRotation(), Desired, DeltaSeconds, SolidBattleFormationSlots::FaceTurnInterpSpeed));
	}

	if (PlanarDist <= AcceptanceRadius)
	{
		Move->MaxWalkSpeed = DesiredMaxSpeed;
		return;
	}

	Move->MaxWalkSpeed = DesiredMaxSpeed;
	AddMovementInput(ToFollow.GetSafeNormal(), 1.f);
}
