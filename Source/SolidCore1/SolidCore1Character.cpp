#include "SolidCore1Character.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
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
#include "Modules/ModuleManager.h"
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

	// Soft paths match common UE 5.7+/5.8 Third Person / mannequin content.
	DefaultSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")));
	DefaultAnimBlueprint = TSoftClassPtr<UAnimInstance>(
		FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C")));

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

static USkeletalMesh* LoadMeshFromAssetData(const FAssetData& Asset)
{
	if (USkeletalMesh* AlreadyLoaded = Cast<USkeletalMesh>(Asset.FastGetAsset(false)))
	{
		return AlreadyLoaded;
	}

	const FSoftObjectPath SoftPath = Asset.ToSoftObjectPath();
	return Cast<USkeletalMesh>(SoftPath.TryLoad());
}

static USkeletalMesh* FindMannequinMeshByRegistry()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Characters/Mannequins")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	UE_LOG(LogSolidCore1, Log, TEXT("Mannequin mesh search: found %d skeletal meshes under /Game/Characters/Mannequins"), Assets.Num());

	USkeletalMesh* Ranked[3] = {nullptr, nullptr, nullptr}; // Simple Manny, Manny, Quinn/other

	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		UE_LOG(LogSolidCore1, Log, TEXT("  candidate mesh: %s"), *Asset.GetObjectPathString());

		USkeletalMesh* Mesh = LoadMeshFromAssetData(Asset);
		if (!Mesh)
		{
			continue;
		}

		if (Name.Contains(TEXT("Manny_Simple"), ESearchCase::IgnoreCase))
		{
			Ranked[0] = Mesh;
		}
		else if (Name.Contains(TEXT("Manny"), ESearchCase::IgnoreCase) && !Ranked[1])
		{
			Ranked[1] = Mesh;
		}
		else if (Name.Contains(TEXT("Quinn"), ESearchCase::IgnoreCase) && !Ranked[2])
		{
			Ranked[2] = Mesh;
		}
		else if (!Ranked[2])
		{
			// Any other mannequin skeletal mesh as last resort.
			Ranked[2] = Mesh;
		}
	}

	for (USkeletalMesh* Candidate : Ranked)
	{
		if (Candidate)
		{
			return Candidate;
		}
	}

	return nullptr;
}

static UClass* FindMannequinAnimClassByRegistry()
{
	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Characters/Mannequins")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UAnimBlueprint::StaticClass()->GetClassPathName());

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	UE_LOG(LogSolidCore1, Log, TEXT("Mannequin anim search: found %d anim blueprints under /Game/Characters/Mannequins"), Assets.Num());

	UClass* Ranked[3] = {nullptr, nullptr, nullptr}; // Manny ABP, Quinn ABP, any ABP

	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		UE_LOG(LogSolidCore1, Log, TEXT("  candidate anim: %s"), *Asset.GetObjectPathString());

		if (Name.Contains(TEXT("PostProcess"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		const FString ClassPath = Asset.GetObjectPathString() + TEXT("_C");
		UClass* AnimClass = StaticLoadClass(UAnimInstance::StaticClass(), nullptr, *ClassPath);
		if (!AnimClass)
		{
			continue;
		}

		if (Name.Contains(TEXT("Unarmed"), ESearchCase::IgnoreCase) && !Ranked[0])
		{
			Ranked[0] = AnimClass;
		}
		else if (Name.Contains(TEXT("Manny"), ESearchCase::IgnoreCase) && !Ranked[1])
		{
			Ranked[1] = AnimClass;
		}
		else if (!Ranked[2])
		{
			Ranked[2] = AnimClass;
		}
	}

	for (UClass* Candidate : Ranked)
	{
		if (Candidate)
		{
			return Candidate;
		}
	}

	return nullptr;
}

void ASolidCore1Character::ApplyCharacterVisuals()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	UE_LOG(LogSolidCore1, Log, TEXT("ApplyCharacterVisuals: current mesh=%s"),
		CharacterMesh->GetSkeletalMeshAsset() ? *CharacterMesh->GetSkeletalMeshAsset()->GetPathName() : TEXT("<none>"));

	if (!CharacterMesh->GetSkeletalMeshAsset())
	{
		USkeletalMesh* LoadedMesh = DefaultSkeletalMesh.LoadSynchronous();

		if (!LoadedMesh)
		{
			static const TCHAR* MeshFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
				TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"),
			};

			for (const TCHAR* Path : MeshFallbacks)
			{
				LoadedMesh = Cast<USkeletalMesh>(
					StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, Path));
				if (LoadedMesh)
				{
					UE_LOG(LogSolidCore1, Log, TEXT("Loaded mesh via fallback path: %s"), Path);
					break;
				}
			}
		}

		if (!LoadedMesh)
		{
			LoadedMesh = FindMannequinMeshByRegistry();
		}

		if (LoadedMesh)
		{
			CharacterMesh->SetSkeletalMeshAsset(LoadedMesh);
			CharacterMesh->SetVisibility(true);
			CharacterMesh->SetHiddenInGame(false);
			CharacterMesh->SetCastShadow(true);
			UE_LOG(LogSolidCore1, Warning, TEXT("Applied mannequin mesh: %s"), *LoadedMesh->GetPathName());
		}
		else
		{
			UE_LOG(LogSolidCore1, Error,
				TEXT("No mannequin mesh found under /Game/Characters/Mannequins. "
					 "Create BP_SolidCore1Character and assign the mesh in the editor (see README)."));
		}
	}

	if (CharacterMesh->GetSkeletalMeshAsset() && CharacterMesh->GetAnimClass() == nullptr)
	{
		UClass* AnimClass = DefaultAnimBlueprint.LoadSynchronous();

		if (!AnimClass)
		{
			static const TCHAR* AnimFallbacks[] = {
				TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Unarmed.ABP_Unarmed_C"),
				TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
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

		if (!AnimClass)
		{
			AnimClass = FindMannequinAnimClassByRegistry();
		}

		if (AnimClass)
		{
			CharacterMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			CharacterMesh->SetAnimInstanceClass(AnimClass);
			UE_LOG(LogSolidCore1, Warning, TEXT("Applied mannequin anim BP: %s"), *AnimClass->GetPathName());
		}
		else if (CharacterMesh->GetSkeletalMeshAsset())
		{
			UE_LOG(LogSolidCore1, Warning,
				TEXT("Mannequin mesh loaded but no Anim Blueprint found. Character will appear in reference pose."));
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
