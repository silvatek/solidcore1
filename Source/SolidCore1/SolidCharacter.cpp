#include "SolidCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "SolidCore1.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "EngineUtils.h"
#include "UObject/SoftObjectPath.h"

namespace SolidInput
{
	static UInputModifierSwizzleAxis* MakeSwizzleYXZ(UObject* Outer)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Outer);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		return Swizzle;
	}

	static UInputModifierNegate* MakeNegate(UObject* Outer)
	{
		return NewObject<UInputModifierNegate>(Outer);
	}
}

ASolidCharacter::ASolidCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// Mesh sits in the capsule. Yaw -90 aligns mesh forward with character forward.
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, MeshGroundZOffset), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetMesh()->SetVisibility(true);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// Fab Viking (custom skeleton + single-node clip locomotion).
	DefaultSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")));
	VikingIdleAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1")));
	VikingWalkAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk")));
	VikingRunAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run")));
	VikingJumpAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(TEXT("/Game/Viking/Animations/Anim_Viking_jump.Anim_Viking_jump")));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = UserZoomArmLength;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;
	CameraBoom->bDoCollisionTest = true;
	CameraBoom->ProbeSize = 12.f;
	CameraBoom->ProbeChannel = ECC_Camera;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ASolidCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyCharacterVisuals();
}

void ASolidCharacter::ApplyMeshGroundOffset()
{
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		FVector Rel = CharacterMesh->GetRelativeLocation();
		Rel.Z = MeshGroundZOffset;
		CharacterMesh->SetRelativeLocation(Rel);
	}
}

void ASolidCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyCharacterVisuals();
	CacheVikingLocomotionAnims();
	UpdateVikingLocomotionAnim();
	EnsureRuntimeInputAssets();
	ApplyWalkSpeed();
	AddMappingContext();
	ApplyMeshGroundOffset();

	// Backup spawn path: Blueprint GameModes sometimes skip C++ BeginPlay.
	ASolidTerrainStreamer::EnsureExists(GetWorld());
}

void ASolidCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureRuntimeInputAssets();
	AddMappingContext();
	ApplyMeshGroundOffset();
	ASolidTerrainStreamer::EnsureExists(GetWorld());
	UE_LOG(LogTemp, Warning, TEXT("[SolidCore1] Character PossessedBy - ensured terrain streamer"));
}

void ASolidCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	EnsureRuntimeInputAssets();
	AddMappingContext();
}

void ASolidCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateVikingLocomotionAnim();
	UpdateGroupCameraFraming(DeltaTime);
	ClampCameraAboveTerrain(DeltaTime);
}

void ASolidCharacter::UpdateGroupCameraFraming(float DeltaTime)
{
	if (!CameraBoom || !bFrameCompanions)
	{
		return;
	}

	// Only the locally controlled player drives the framing camera.
	if (!IsLocallyControlled())
	{
		return;
	}

	struct FFramedPoint
	{
		FVector Location;
		float CapsuleHalfHeight;
	};

	TArray<FFramedPoint, TInlineAllocator<8>> Subjects;
	{
		float HalfHeight = 96.f;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
		Subjects.Add({ GetActorLocation(), HalfHeight });
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			float HalfHeight = 96.f;
			if (const UCapsuleComponent* Capsule = It->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
			Subjects.Add({ It->GetActorLocation(), HalfHeight });
		}
	}

	const bool bHasCompanions = Subjects.Num() > 1;

	if (bDisableBoomCollisionWhileFraming)
	{
		// SC1-0025 popped narrower when the boom probe hit hills and collapsed arm length.
		CameraBoom->bDoCollisionTest = !bHasCompanions;
	}

	FVector DesiredTargetOffset = FVector::ZeroVector;
	float FramingFitArm = 0.f;

	if (bHasCompanions)
	{
		FVector Center = FVector::ZeroVector;
		for (const FFramedPoint& Subject : Subjects)
		{
			Center += Subject.Location;
		}
		Center /= static_cast<float>(Subjects.Num());

		FVector ToCenter = Center - GetActorLocation();
		ToCenter.Z *= 0.45f;
		DesiredTargetOffset = ToCenter;

		FRotator ViewRot = GetControlRotation();
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			ViewRot = PC->GetControlRotation();
		}
		const FRotationMatrix ViewMatrix(ViewRot);
		const FVector CamRight = ViewMatrix.GetUnitAxis(EAxis::Y);
		const FVector CamUp = ViewMatrix.GetUnitAxis(EAxis::Z);

		float MaxRight = 0.f;
		float MaxUp = 0.f;
		for (const FFramedPoint& Subject : Subjects)
		{
			const FVector Delta = Subject.Location - Center;
			MaxRight = FMath::Max(MaxRight, FMath::Abs(FVector::DotProduct(Delta, CamRight)) + 45.f);
			MaxUp = FMath::Max(
				MaxUp,
				FMath::Abs(FVector::DotProduct(Delta, CamUp)) + Subject.CapsuleHalfHeight);
		}

		MaxRight += FramingPadding;
		MaxUp += FramingPadding * 0.65f;

		float VerticalFovDeg = FollowCamera ? FollowCamera->FieldOfView : 90.f;
		VerticalFovDeg = FMath::Clamp(VerticalFovDeg, 40.f, 120.f);
		const float HalfVFovRad = FMath::DegreesToRadians(VerticalFovDeg * 0.5f);
		const float HalfHFovRad = FMath::Atan(FMath::Tan(HalfVFovRad) * FramingAspectRatio);

		const float DistForWidth = MaxRight / FMath::Max(FMath::Tan(HalfHFovRad), 0.05f);
		const float DistForHeight = MaxUp / FMath::Max(FMath::Tan(HalfVFovRad), 0.05f);
		FramingFitArm = FMath::Clamp(
			FMath::Max(DistForWidth, DistForHeight),
			FramingMinArmLength,
			FramingMaxArmLength);
	}

	// Wheel zoom owns arm length by default. Framing may only pull out if explicitly allowed
	// (otherwise zoom-in hits CameraZoomMin while arm stays long — feels "stuck").
	float DesiredArmLength = UserZoomArmLength;
	if (bHasCompanions && bFramingCanOverrideZoom && FramingFitArm > DesiredArmLength)
	{
		DesiredArmLength = FramingFitArm;
	}
	DesiredArmLength = FMath::Clamp(DesiredArmLength, CameraZoomMin, CameraZoomMax);

	CameraBoom->TargetOffset = FMath::VInterpTo(
		CameraBoom->TargetOffset, DesiredTargetOffset, DeltaTime, FramingOffsetInterpSpeed);

	const float ArmInterpSpeed = (DesiredArmLength > CameraBoom->TargetArmLength)
		? FramingZoomOutSpeed
		: FramingZoomInSpeed;
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, ArmInterpSpeed);
}

void ASolidCharacter::ClampCameraAboveTerrain(float DeltaTime)
{
	if (!CameraBoom || !IsLocallyControlled())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = nullptr;
	for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
	{
		Streamer = *It;
		break;
	}
	if (!Streamer)
	{
		return;
	}

	// Predict camera location the same way USpringArmComponent does (without terrain lift).
	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FRotationMatrix ArmMatrix(ArmRot);
	const FVector ArmOrigin = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
	const FVector DesiredCam =
		ArmOrigin
		- ArmRot.Vector() * CameraBoom->TargetArmLength
		+ ArmMatrix.TransformVector(FVector(CameraBoom->SocketOffset.X, CameraBoom->SocketOffset.Y, 0.f));

	const float TerrainZ = Streamer->GetHeightAt(DesiredCam) + Streamer->CollisionHeightBias;
	const float MinCamZ = TerrainZ + CameraTerrainClearance;
	const float NeededLift = FMath::Clamp(
		FMath::Max(0.f, MinCamZ - DesiredCam.Z),
		0.f,
		CameraTerrainLiftMax);

	// Drop lift faster than we add it so the boom does not stay stuck high after cresting a hill.
	const float LiftInterpSpeed = (NeededLift < CameraTerrainLiftCm)
		? CameraTerrainLiftSpeed * 1.8f
		: CameraTerrainLiftSpeed;
	CameraTerrainLiftCm = FMath::FInterpTo(
		CameraTerrainLiftCm, NeededLift, DeltaTime, LiftInterpSpeed);

	// Convert world-up lift into spring-arm local SocketOffset so attachment keeps it.
	const FVector LocalLift = ArmMatrix.InverseTransformVector(FVector(0.f, 0.f, CameraTerrainLiftCm));
	CameraBoom->SocketOffset = FVector(
		CameraBoom->SocketOffset.X,
		CameraBoom->SocketOffset.Y,
		0.f) + LocalLift;
}

void ASolidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureRuntimeInputAssets();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}

		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASolidCharacter::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASolidCharacter::Look);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ASolidCharacter::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASolidCharacter::StopSprint);
		}

		if (ZoomAction)
		{
			EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &ASolidCharacter::Zoom);
		}
	}
	else
	{
		UE_LOG(LogSolid, Error,
			TEXT("SolidCharacter requires an Enhanced Input Component. Check DefaultInput.ini DefaultInputComponentClass."));
	}
}

void ASolidCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASolidCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASolidCharacter::Zoom(const FInputActionValue& Value)
{
	// Clamp axis — some platforms deliver large wheel spikes in one tick.
	const float Axis = FMath::Clamp(Value.Get<float>(), -3.f, 3.f);
	if (FMath::IsNearlyZero(Axis))
	{
		return;
	}

	// Scroll up (positive) zooms out → longer arm.
	UserZoomArmLength = FMath::Clamp(
		UserZoomArmLength + Axis * CameraZoomStep,
		CameraZoomMin,
		CameraZoomMax);
}

void ASolidCharacter::StartSprint()
{
	bIsSprinting = true;
	ApplyWalkSpeed();
}

void ASolidCharacter::StopSprint()
{
	bIsSprinting = false;
	ApplyWalkSpeed();
}

void ASolidCharacter::ApplyWalkSpeed() const
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void ASolidCharacter::AddMappingContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
			if (ZoomMappingContext)
			{
				Subsystem->AddMappingContext(ZoomMappingContext, 1);
			}
		}
	}
}

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
		UE_LOG(LogSolid, Warning, TEXT("Applied character mesh: %s"), *LoadedMesh->GetPathName());
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

	UE_LOG(LogSolid, Error, TEXT("Player Viking: failed AnimSingleNodeInstance for %s"), *Anim->GetName());
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
	const UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bInAir = Move && Move->IsFalling();
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
			|| (Move && Move->MaxWalkSpeed >= SprintSpeed - 1.f);

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

void ASolidCharacter::EnsureRuntimeInputAssets()
{
	if (!MoveAction)
	{
		MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"), RF_Transient);
		MoveAction->ValueType = EInputActionValueType::Axis2D;
	}

	if (!LookAction)
	{
		LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"), RF_Transient);
		LookAction->ValueType = EInputActionValueType::Axis2D;
	}

	if (!JumpAction)
	{
		JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"), RF_Transient);
		JumpAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!SprintAction)
	{
		SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint"), RF_Transient);
		SprintAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!ZoomAction)
	{
		ZoomAction = NewObject<UInputAction>(this, TEXT("IA_Zoom"), RF_Transient);
		ZoomAction->ValueType = EInputActionValueType::Axis1D;
	}

	if (!DefaultMappingContext)
	{
		DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"), RF_Transient);

		// WASD → Axis2D (X = strafe, Y = forward)
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
			Mapping.Modifiers.Add(SolidInput::MakeSwizzleYXZ(DefaultMappingContext));
		}
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
			Mapping.Modifiers.Add(SolidInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidInput::MakeSwizzleYXZ(DefaultMappingContext));
		}
		DefaultMappingContext->MapKey(MoveAction, EKeys::D);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
			Mapping.Modifiers.Add(SolidInput::MakeNegate(DefaultMappingContext));
		}

		// Gamepad left stick
		DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftY);
			Mapping.Modifiers.Add(SolidInput::MakeSwizzleYXZ(DefaultMappingContext));
		}

		// Mouse look
		DefaultMappingContext->MapKey(LookAction, EKeys::MouseX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::MouseY);
			Mapping.Modifiers.Add(SolidInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidInput::MakeSwizzleYXZ(DefaultMappingContext));
		}

		// Gamepad right stick
		DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightY);
			Mapping.Modifiers.Add(SolidInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidInput::MakeSwizzleYXZ(DefaultMappingContext));
		}

		DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		DefaultMappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

		DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
		DefaultMappingContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	}

	if (!ZoomMappingContext && ZoomAction)
	{
		ZoomMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Zoom"), RF_Transient);
		ZoomMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	}
}
