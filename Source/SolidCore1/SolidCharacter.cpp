#include "SolidCharacter.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/SkeletalMesh.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "SolidClipLocomotion.h"
#include "SolidCore1.h"
#include "SolidGameMode.h"
#include "HUD/SolidHUD.h"
#include "Menus/SolidMainMenu.h"
#include "SolidNameLabel.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPath.h"

// Captain core: construction, lifecycle, movement / look / zoom / sprint.
// Input factory → SolidCharacterInput.cpp
// Visuals / clip selection → SolidCharacterVisuals.cpp (shared play via SolidClipLocomotion)
// Party camera → SolidPartyCamera.cpp

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

	// Default mesh/clips are Fab Viking; Captain may override DefaultSkeletalMesh later.
	DefaultSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(
		FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));
	IdleAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultIdlePath));
	WalkAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultWalkPath));
	RunAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultRunPath));
	JumpAnim = TSoftObjectPtr<UAnimSequence>(
		FSoftObjectPath(SolidClipLocomotion::DefaultJumpPath));

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

	NameLabelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("NameLabelRoot"));
	NameLabelRoot->SetupAttachment(RootComponent);
	NameLabelBorder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBorder"));
	NameLabelBorder->SetupAttachment(NameLabelRoot);
	NameLabelBackground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NameLabelBackground"));
	NameLabelBackground->SetupAttachment(NameLabelRoot);
	NameLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
	NameLabel->SetupAttachment(NameLabelRoot);
	ApplyNameLabel();
	ApplySightPresentation();
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

void ASolidCharacter::ApplyNameLabel()
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
		SolidNameLabel::EStyle::Captain,
		CapsuleHalf,
		this);
}

void ASolidCharacter::SetCharacterDisplayName(const FString& NewName)
{
	CharacterDisplayName = NewName;
	ApplyNameLabel();
}

void ASolidCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyCharacterVisuals();
	ApplyNameLabel();
	CacheLocomotionAnims();
	UpdateLocomotionAnim();
	EnsureRuntimeInputAssets();
	ApplyWalkSpeed();
	AddMappingContext();
	ApplyMeshGroundOffset();
	ApplySightPresentation();
}

void ASolidCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureRuntimeInputAssets();
	AddMappingContext();
	ApplyMeshGroundOffset();
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
	TickPartyFormationDrill(DeltaTime);
	UpdateLocomotionAnim();
	UpdatePartyCameraFraming(DeltaTime);
	ClampCameraAboveTerrain(DeltaTime);
	SolidNameLabel::FaceViewCamera(NameLabelRoot, GetWorld());
}

void ASolidCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureRuntimeInputAssets();

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ASolidCharacter::StartJump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASolidCharacter::StopJumpFromInput);
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

	// Battle-plan hotkeys (F1–F8) via classic key binds on the same input component.
	PlayerInputComponent->BindKey(EKeys::F1, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot1);
	PlayerInputComponent->BindKey(EKeys::F2, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot2);
	PlayerInputComponent->BindKey(EKeys::F3, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot3);
	PlayerInputComponent->BindKey(EKeys::F4, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot4);
	PlayerInputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot5);
	PlayerInputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot6);
	PlayerInputComponent->BindKey(EKeys::F7, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot7);
	PlayerInputComponent->BindKey(EKeys::F8, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanSlot8);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ASolidCharacter::ToggleSightFromInput);
	PlayerInputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ASolidCharacter::ToggleMainMenuFromInput);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASolidCharacter::CloseMenuOverlayFromInput);
	PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ASolidCharacter::MainMenuMoveUp);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ASolidCharacter::MainMenuMoveDown);
	PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASolidCharacter::MainMenuConfirm);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
	PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
}

ASolidHUD* ASolidCharacter::GetSolidHUD() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		return Cast<ASolidHUD>(PC->GetHUD());
	}
	return nullptr;
}

bool ASolidCharacter::IsMenuOverlayOpen() const
{
	if (const ASolidHUD* HUD = GetSolidHUD())
	{
		return HUD->IsMainMenuOpen() || HUD->IsCreditsVisible();
	}
	return false;
}

void ASolidCharacter::ApplySightPresentation()
{
	const bool bTrue = IsTrueSight();

	bUseControllerRotationYaw = bTrue;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (!IsPartyFormationDrillActive())
		{
			Move->bOrientRotationToMovement = !bTrue;
		}
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(bTrue);
		CharacterMesh->SetCastHiddenShadow(bTrue);
	}

	if (NameLabelRoot)
	{
		NameLabelRoot->SetVisibility(!bTrue, /*bPropagateToChildren=*/true);
	}

	if (bTrue)
	{
		ApplyTrueSightCamera();
	}
	else
	{
		ApplyRavenSightCamera();
	}
}

void ASolidCharacter::SetSight(const ESolidSight NewSight)
{
	if (Sight == NewSight)
	{
		return;
	}

	Sight = NewSight;
	ApplySightPresentation();
	UE_LOG(LogSolid, Warning, TEXT("Sight: %s"), SolidSight::Label(Sight));
}

void ASolidCharacter::ToggleSight()
{
	SetSight(SolidSight::Toggle(Sight));
}

void ASolidCharacter::ToggleSightFromInput()
{
	if (IsMenuOverlayOpen() || IsPartyFormationDrillActive())
	{
		return;
	}

	const ESolidSight Next = SolidSight::Toggle(Sight);
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (!GameMode->IsSightEnabled(Next))
			{
				UE_LOG(LogSolid, Warning, TEXT("Sight locked: %s"), SolidSight::Label(Next));
				return;
			}
		}
	}
	ToggleSight();
}

void ASolidCharacter::ToggleMainMenuFromInput()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->HandleMenuKey();
	}
}

void ASolidCharacter::CloseMenuOverlayFromInput()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->CloseMenuOverlay();
	}
}

void ASolidCharacter::MainMenuMoveUp()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsMainMenuOpen())
		{
			HUD->MoveMainMenuSelection(-1);
		}
	}
}

void ASolidCharacter::MainMenuMoveDown()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsMainMenuOpen())
		{
			HUD->MoveMainMenuSelection(1);
		}
	}
}

void ASolidCharacter::MainMenuChoose1()
{
	ChooseMainMenuIndex(0);
}

void ASolidCharacter::MainMenuChoose2()
{
	ChooseMainMenuIndex(1);
}

void ASolidCharacter::ChooseMainMenuIndex(const int32 Index)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsMainMenuOpen())
	{
		return;
	}
	HUD->SetMainMenuIndex(Index);
	MainMenuConfirm();
}

void ASolidCharacter::MainMenuConfirm()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsMainMenuOpen())
	{
		return;
	}

	FSolidMainMenuEntry Entry;
	if (!SolidMainMenu::FindEntry(HUD->GetMainMenuIndex(), Entry))
	{
		return;
	}

	HUD->CloseMainMenu();
	switch (SolidMainMenu::ActionForItem(Entry.Item))
	{
	case ESolidMainMenuAction::StartTestDrill:
		StartPartyFormationDrill();
		break;
	case ESolidMainMenuAction::ShowCredits:
		HUD->SetCreditsVisible(true);
		break;
	default:
		break;
	}
}

void ASolidCharacter::SelectBattlePlanSlot(const int32 SlotIndex)
{
	if (UWorld* World = GetWorld())
	{
		if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			GameMode->SelectBattlePlanSlot(SlotIndex);
		}
	}
}

void ASolidCharacter::SelectBattlePlanSlotFromInput(const int32 SlotIndex)
{
	if (!IsMenuOverlayOpen())
	{
		SelectBattlePlanSlot(SlotIndex);
	}
}

void ASolidCharacter::SelectBattlePlanSlot1() { SelectBattlePlanSlotFromInput(0); }
void ASolidCharacter::SelectBattlePlanSlot2() { SelectBattlePlanSlotFromInput(1); }
void ASolidCharacter::SelectBattlePlanSlot3() { SelectBattlePlanSlotFromInput(2); }
void ASolidCharacter::SelectBattlePlanSlot4() { SelectBattlePlanSlotFromInput(3); }
void ASolidCharacter::SelectBattlePlanSlot5() { SelectBattlePlanSlotFromInput(4); }
void ASolidCharacter::SelectBattlePlanSlot6() { SelectBattlePlanSlotFromInput(5); }
void ASolidCharacter::SelectBattlePlanSlot7() { SelectBattlePlanSlotFromInput(6); }
void ASolidCharacter::SelectBattlePlanSlot8() { SelectBattlePlanSlotFromInput(7); }

void ASolidCharacter::Move(const FInputActionValue& Value)
{
	if (IsPartyFormationDrillActive() || IsMenuOverlayOpen())
	{
		return;
	}

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
	if (IsPartyFormationDrillActive() || IsMenuOverlayOpen())
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ASolidCharacter::StartJump()
{
	if (IsMenuOverlayOpen())
	{
		return;
	}
	Jump();
}

void ASolidCharacter::StopJumpFromInput()
{
	StopJumping();
}

void ASolidCharacter::Zoom(const FInputActionValue& Value)
{
	if (IsMenuOverlayOpen() || IsTrueSight())
	{
		return;
	}

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
	if (IsPartyFormationDrillActive() || IsMenuOverlayOpen())
	{
		return;
	}
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
