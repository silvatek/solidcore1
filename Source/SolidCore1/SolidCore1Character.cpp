#include "SolidCore1Character.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
#include "UObject/SoftObjectPath.h"

namespace SolidCore1Input
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

ASolidCore1Character::ASolidCore1Character()
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

	// Mannequin mesh sits in the capsule (Epic Third Person offsets).
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	GetMesh()->SetVisibility(true);

	// Soft paths match the UE Third Person template Content layout.
	DefaultSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny")));
	DefaultAnimBlueprint = TSoftClassPtr<UAnimInstance>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C")));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void ASolidCore1Character::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	ApplyCharacterVisuals();
}

void ASolidCore1Character::BeginPlay()
{
	Super::BeginPlay();
	ApplyCharacterVisuals();
	EnsureRuntimeInputAssets();
	ApplyWalkSpeed();
	AddMappingContext();
}

void ASolidCore1Character::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureRuntimeInputAssets();
	AddMappingContext();
}

void ASolidCore1Character::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	EnsureRuntimeInputAssets();
	AddMappingContext();
}

void ASolidCore1Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ASolidCore1Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
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
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASolidCore1Character::Move);
		}

		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASolidCore1Character::Look);
		}

		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ASolidCore1Character::StartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ASolidCore1Character::StopSprint);
		}
	}
	else
	{
		UE_LOG(LogSolidCore1, Error,
			TEXT("SolidCore1Character requires an Enhanced Input Component. Check DefaultInput.ini DefaultInputComponentClass."));
	}
}

void ASolidCore1Character::Move(const FInputActionValue& Value)
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

void ASolidCore1Character::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASolidCore1Character::StartSprint()
{
	bIsSprinting = true;
	ApplyWalkSpeed();
}

void ASolidCore1Character::StopSprint()
{
	bIsSprinting = false;
	ApplyWalkSpeed();
}

void ASolidCore1Character::ApplyWalkSpeed() const
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	}
}

void ASolidCore1Character::AddMappingContext()
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
		}
	}
}

void ASolidCore1Character::ApplyCharacterVisuals()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	if (!CharacterMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = DefaultSkeletalMesh.LoadSynchronous();

		// Fallback paths used by some UE Third Person / Game Animation layouts.
		if (!LoadedMesh)
		{
			static const TCHAR* MeshFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny"),
			};

			for (const TCHAR* Path : MeshFallbacks)
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
		}
		else
		{
			UE_LOG(LogSolidCore1, Warning,
				TEXT("No mannequin mesh found. Migrate Content/Characters/Mannequins from a Third Person template project "
					 "(expected /Game/Characters/Mannequins/Meshes/SKM_Manny)."));
		}
	}

	if (CharacterMesh->GetSkeletalMeshAsset() && CharacterMesh->GetAnimClass() == nullptr)
	{
		UClass* AnimClass = DefaultAnimBlueprint.LoadSynchronous();

		if (!AnimClass)
		{
			static const TCHAR* AnimFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C"),
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny"),
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
		}
		else if (CharacterMesh->GetSkeletalMeshAsset())
		{
			UE_LOG(LogSolidCore1, Warning,
				TEXT("Mannequin mesh loaded but no Anim Blueprint found. Character will appear in reference pose. "
					 "Expected /Game/Characters/Mannequins/Animations/ABP_Manny."));
		}
	}
}

void ASolidCore1Character::EnsureRuntimeInputAssets()
{
	if (MoveAction && LookAction && JumpAction && SprintAction && DefaultMappingContext)
	{
		return;
	}

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

	if (!DefaultMappingContext)
	{
		DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"), RF_Transient);

		// WASD → Axis2D (X = strafe, Y = forward)
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
			Mapping.Modifiers.Add(SolidCore1Input::MakeSwizzleYXZ(DefaultMappingContext));
		}
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
			Mapping.Modifiers.Add(SolidCore1Input::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCore1Input::MakeSwizzleYXZ(DefaultMappingContext));
		}
		DefaultMappingContext->MapKey(MoveAction, EKeys::D);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
			Mapping.Modifiers.Add(SolidCore1Input::MakeNegate(DefaultMappingContext));
		}

		// Gamepad left stick
		DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftY);
			Mapping.Modifiers.Add(SolidCore1Input::MakeSwizzleYXZ(DefaultMappingContext));
		}

		// Mouse look
		DefaultMappingContext->MapKey(LookAction, EKeys::MouseX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::MouseY);
			Mapping.Modifiers.Add(SolidCore1Input::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCore1Input::MakeSwizzleYXZ(DefaultMappingContext));
		}

		// Gamepad right stick
		DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightY);
			Mapping.Modifiers.Add(SolidCore1Input::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCore1Input::MakeSwizzleYXZ(DefaultMappingContext));
		}

		DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		DefaultMappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

		DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
		DefaultMappingContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	}
}
