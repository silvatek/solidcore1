#include "SolidCharacter.h"
#include "Companion/SolidCompanionCharacter.h"
#include "HUD/SolidPointer.h"
#include "SolidClickMove.h"
#include "SolidGroundMove.h"
#include "Lairs/SolidLairInterior.h"
#include "Lairs/SolidLairEntrance.h"
#include "Strongholds/Sites/SolidStrongholdSite.h"
#include "Strongholds/Actors/SolidStrongholdExterior.h"
#include "Strongholds/Actors/SolidStrongholdInterior.h"
#include "SolidStronghold.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "SolidPlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "SolidClipLocomotion.h"
#include "SolidCore1.h"
#include "Company/SolidBattlePlan.h"
#include "Company/SolidCaptain.h"
#include "Company/SolidCompany.h"
#include "SolidGameMode.h"
#include "Character/SolidCharacterStage.h"
#include "HUD/SolidHUD.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Vegetation/SolidForage.h"
#include "Vegetation/SolidForagePile.h"
#include "Menus/SolidCharacterEditor.h"
#include "Menus/SolidMainMenu.h"
#include "SolidBodyParams.h"
#include "Menus/SolidSquadBuilder.h"
#include "Menus/SolidExit.h"
#include "Menus/SolidPortal.h"
#include "SolidNameLabel.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPath.h"

// Captain core: construction, lifecycle, movement / look / zoom / sprint.
// Input factory → SolidCharacterInput.cpp
// Visuals / clip selection → SolidCharacterVisuals.cpp (shared play via SolidClipLocomotion)
// Squad camera → SolidSquadCamera.cpp

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
	GetCharacterMovement()->AddTickPrerequisiteActor(this);

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
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->AddTickPrerequisiteActor(this);
	}
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

void ASolidCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CharacterStage)
	{
		CharacterStage->Destroy();
		CharacterStage = nullptr;
	}
	SetEditorWorldHidden(false);
	SetEditorCameraLocked(false);
	Super::EndPlay(EndPlayReason);
}

void ASolidCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	EnsureRuntimeInputAssets();
	AddMappingContext();
	ApplyMeshGroundOffset();
	if (ASolidGameMode* GameMode = Cast<ASolidGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		if (USolidCompany* Company = GameMode->GetCompany())
		{
			if (USolidCaptain* Captain = Company->GetCaptain())
			{
				Captain->SetActor(this);
			}
		}
	}
}

void ASolidCharacter::UnPossessed()
{
	if (ASolidGameMode* GameMode = Cast<ASolidGameMode>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		if (USolidCompany* Company = GameMode->GetCompany())
		{
			if (USolidCaptain* Captain = Company->GetCaptain())
			{
				if (Captain->GetActor() == this)
				{
					Captain->SetActor(nullptr);
				}
			}
		}
	}
	Super::UnPossessed();
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
	TickSquadFormationDrill(DeltaTime);
	UpdatePointerMouseLook();
	UpdateMenuPointerHover();
	UpdateCavePassageHover();
	TickClickMove();
	SolidGroundMove::Hold(this);
	UpdateBodyShadow();
	UpdateCharacterEditorPointer();
	UpdateInventoryPointer();
	UpdateSquadBuilderPointer();
	UpdateLocomotionAnim();
	if (const ASolidHUD* SolidHud = GetSolidHUD())
	{
		if (SolidHud->IsCharacterEditorOpen())
		{
			if (Controller)
			{
				FRotator Level = Controller->GetControlRotation();
				Level.Pitch = 0.f;
				Level.Roll = 0.f;
				Controller->SetControlRotation(Level);
			}
		}
		else
		{
			UpdateSquadCameraFraming(DeltaTime);
			ClampCameraAboveTerrain(DeltaTime);
		}
	}
	else
	{
		UpdateSquadCameraFraming(DeltaTime);
		ClampCameraAboveTerrain(DeltaTime);
	}
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
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanFaceRight);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanFaceLeft);
	PlayerInputComponent->BindKey(EKeys::Gamepad_FaceButton_Top, IE_Pressed, this, &ASolidCharacter::SelectBattlePlanFaceTop);
	PlayerInputComponent->BindKey(EKeys::F9, IE_Pressed, this, &ASolidCharacter::ToggleSightFromInput);
	PlayerInputComponent->BindKey(SolidSight::ControllerToggleKey(), IE_Pressed, this, &ASolidCharacter::ToggleSightFromInput);
	PlayerInputComponent->BindKey(EKeys::F10, IE_Pressed, this, &ASolidCharacter::ToggleMainMenuFromInput);
	PlayerInputComponent->BindKey(SolidMainMenu::ControllerKey(), IE_Pressed, this, &ASolidCharacter::ToggleMainMenuFromInput);
	PlayerInputComponent->BindKey(SolidInventory::ControllerToggleKey(), IE_Pressed, this, &ASolidCharacter::ToggleInventoryFromController);
	PlayerInputComponent->BindKey(EKeys::F12, IE_Pressed, this, &ASolidCharacter::ToggleDebugPanelFromInput);
	PlayerInputComponent->BindKey(EKeys::Slash, IE_Pressed, this, &ASolidCharacter::ToggleInventoryFromSlash);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASolidCharacter::CloseMenuOverlayFromInput);
	PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ASolidCharacter::MainMenuMoveUp);
	PlayerInputComponent->BindKey(EKeys::Up, IE_Repeat, this, &ASolidCharacter::MainMenuMoveUp);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ASolidCharacter::MainMenuMoveDown);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Repeat, this, &ASolidCharacter::MainMenuMoveDown);
	PlayerInputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ASolidCharacter::CharacterEditorNudgeLeft);
	PlayerInputComponent->BindKey(EKeys::Left, IE_Repeat, this, &ASolidCharacter::CharacterEditorNudgeLeft);
	PlayerInputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ASolidCharacter::CharacterEditorNudgeRight);
	PlayerInputComponent->BindKey(EKeys::Right, IE_Repeat, this, &ASolidCharacter::CharacterEditorNudgeRight);
	PlayerInputComponent->BindKey(EKeys::PageUp, IE_Pressed, this, &ASolidCharacter::CharacterEditorPrevious);
	PlayerInputComponent->BindKey(EKeys::PageUp, IE_Repeat, this, &ASolidCharacter::CharacterEditorPrevious);
	PlayerInputComponent->BindKey(EKeys::PageDown, IE_Pressed, this, &ASolidCharacter::CharacterEditorNext);
	PlayerInputComponent->BindKey(EKeys::PageDown, IE_Repeat, this, &ASolidCharacter::CharacterEditorNext);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ASolidCharacter::ToggleCharacterEditorSkeleton);
	PlayerInputComponent->BindKey(EKeys::Home, IE_Pressed, this, &ASolidCharacter::TurnCharacterEditorClockwise);
	PlayerInputComponent->BindKey(EKeys::Home, IE_Repeat, this, &ASolidCharacter::TurnCharacterEditorClockwise);
	PlayerInputComponent->BindKey(EKeys::End, IE_Pressed, this, &ASolidCharacter::TurnCharacterEditorAnticlockwise);
	PlayerInputComponent->BindKey(EKeys::End, IE_Repeat, this, &ASolidCharacter::TurnCharacterEditorAnticlockwise);
	PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASolidCharacter::MainMenuConfirm);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASolidCharacter::MainMenuChoose3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ASolidCharacter::MainMenuChoose4);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ASolidCharacter::MainMenuChoose5);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ASolidCharacter::MainMenuChoose6);
	PlayerInputComponent->BindKey(EKeys::Seven, IE_Pressed, this, &ASolidCharacter::MainMenuChoose7);
	PlayerInputComponent->BindKey(EKeys::Eight, IE_Pressed, this, &ASolidCharacter::MainMenuChoose8);
	PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
	PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ASolidCharacter::MainMenuChoose3);
	PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ASolidCharacter::MainMenuChoose4);
	PlayerInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &ASolidCharacter::MainMenuChoose5);
	PlayerInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &ASolidCharacter::MainMenuChoose6);
	PlayerInputComponent->BindKey(EKeys::NumPadSeven, IE_Pressed, this, &ASolidCharacter::MainMenuChoose7);
	PlayerInputComponent->BindKey(EKeys::NumPadEight, IE_Pressed, this, &ASolidCharacter::MainMenuChoose8);
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
		return HUD->IsMainMenuOpen()
			|| HUD->IsPortalOpen()
			|| HUD->IsSquadBuilderOpen()
			|| HUD->IsCreditsVisible()
			|| HUD->IsJournalVisible()
			|| HUD->IsInventoryVisible()
			|| HUD->IsCharacterEditorOpen()
			|| HUD->IsMessageVisible()
			|| HUD->IsStrongholdChoiceOpen();
	}
	return false;
}

void ASolidCharacter::ApplySightPresentation()
{
	const bool bTrue = IsTrueSight();

	bUseControllerRotationYaw = bTrue;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		if (!IsSquadFormationDrillActive())
		{
			Move->bOrientRotationToMovement = !bTrue;
		}
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetOwnerNoSee(bTrue);
	}
	UpdateBodyShadow();

	UpdateNameLabelVisibility();

	if (bTrue)
	{
		ApplyTrueSightCamera();
	}
	else
	{
		ApplyRavenSightCamera();
	}
}

void ASolidCharacter::UpdateBodyShadow()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh)
	{
		return;
	}

	bool bInLair = false;
	if (const UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* Mode = World->GetAuthGameMode<ASolidGameMode>())
		{
			bInLair = Mode->IsInLair();
		}
	}

	const bool bCast = SolidLair::BodyCastsShadow(bInLair);
	if (CharacterMesh->CastShadow != static_cast<uint8>(bCast))
	{
		CharacterMesh->SetCastShadow(bCast);
	}
	const bool bHiddenShadow = bCast && IsTrueSight();
	if (CharacterMesh->bCastHiddenShadow != static_cast<uint8>(bHiddenShadow))
	{
		CharacterMesh->SetCastHiddenShadow(bHiddenShadow);
	}
}

void ASolidCharacter::UpdateNameLabelVisibility()
{
	if (!NameLabelRoot)
	{
		return;
	}

	bool bInStronghold = false;
	int32 CompanionCount = 0;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			bInStronghold = GameMode->IsInClosedVisit();
			for (const TObjectPtr<ASolidCompanionCharacter>& Companion : GameMode->GetCompanions())
			{
				if (IsValid(Companion))
				{
					++CompanionCount;
				}
			}
		}
	}

	const bool bShow = SolidNameLabel::ShowsCaptainName(IsTrueSight(), bInStronghold, CompanionCount);
	NameLabelRoot->SetVisibility(bShow, /*bPropagateToChildren=*/true);
}

void ASolidCharacter::SetSight(const ESolidSight NewSight)
{
	if (Sight == NewSight)
	{
		return;
	}

	Sight = NewSight;
	if (SolidSight::IsTrueSight(NewSight))
	{
		ClearClickMove();
		if (Controller)
		{
			Controller->SetControlRotation(
				SolidSight::ViewForSight(NewSight, Controller->GetControlRotation()));
		}
	}
	ApplySightPresentation();
	UE_LOG(LogSolid, Warning, TEXT("Sight: %s"), SolidSight::Label(Sight));
}

void ASolidCharacter::ToggleSight()
{
	SetSight(SolidSight::Toggle(Sight));
}

void ASolidCharacter::ToggleSightFromInput()
{
	if (IsMenuOverlayOpen() || IsSquadFormationDrillActive())
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

void ASolidCharacter::ToggleInventoryFromController()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->ToggleInventory();
		SyncCharacterStage();
	}
}

void ASolidCharacter::ToggleMainMenuFromInput()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->HandleMenuKey();
		SyncCharacterStage();
	}
}

void ASolidCharacter::ToggleInventoryFromSlash()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	const bool bShift = PC
		&& (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift));
	if (!SolidInventory::IsToggleKey(EKeys::Slash, bShift))
	{
		return;
	}
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->ToggleInventory();
		SyncCharacterStage();
	}
}

void ASolidCharacter::ToggleDebugPanelFromInput()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->ToggleDebugPanel();
	}
}

void ASolidCharacter::CloseMenuOverlayFromInput()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		HUD->CloseMenuOverlay();
		SyncCharacterStage();
	}
}

void ASolidCharacter::CharacterEditorPrevious()
{
	MoveCharacterEditor(-1);
}

void ASolidCharacter::CharacterEditorNext()
{
	MoveCharacterEditor(1);
}

void ASolidCharacter::CharacterEditorNudgeLeft()
{
	NudgeCharacterEditorSlider(-1);
}

void ASolidCharacter::CharacterEditorNudgeRight()
{
	NudgeCharacterEditorSlider(1);
}

void ASolidCharacter::MoveCharacterEditor(const int32 Delta)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsCharacterEditorOpen())
	{
		return;
	}
	HUD->MoveCharacterEditor(Delta);
	SyncCharacterStage();
}

void ASolidCharacter::NudgeCharacterEditorSlider(const int32 Direction)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsCharacterEditorOpen())
	{
		return;
	}
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	USolidCompany* Company = GameMode ? GameMode->GetCompany() : nullptr;
	const int32 Index = HUD->GetCharacterEditorIndex();
	const FSolidCompanion* Record = Company ? Company->GetCompanion(Index) : nullptr;
	if (!Record)
	{
		return;
	}

	const FString Name = Record->Name;
	FSolidBodyControls Controls;
	Controls.Height = Record->BodyHeight;
	Controls.Build = Record->BodyBuild;
	Controls.Gender = Record->BodyGender;
	SolidBodyControls::Nudge(Controls, HUD->GetCharacterEditorSlider(), Direction);
	if (!Company->SetCompanionBodyControls(Index, Controls.Height, Controls.Build, Controls.Gender))
	{
		return;
	}
	PushEditorBodyToSquad(Name, Controls.Height, Controls.Build, Controls.Gender);
	SyncCharacterStage();
}

void ASolidCharacter::ApplyEditorSliderFraction(const int32 Slider, const float Fraction)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsCharacterEditorOpen())
	{
		return;
	}
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	USolidCompany* Company = GameMode ? GameMode->GetCompany() : nullptr;
	const int32 Index = HUD->GetCharacterEditorIndex();
	const FSolidCompanion* Record = Company ? Company->GetCompanion(Index) : nullptr;
	if (!Record)
	{
		return;
	}

	const FString Name = Record->Name;
	FSolidBodyControls Controls;
	Controls.Height = Record->BodyHeight;
	Controls.Build = Record->BodyBuild;
	Controls.Gender = Record->BodyGender;
	SolidBodyControls::SetFraction(Controls, Slider, Fraction);
	if (!Company->SetCompanionBodyControls(Index, Controls.Height, Controls.Build, Controls.Gender))
	{
		return;
	}
	PushEditorBodyToSquad(Name, Controls.Height, Controls.Build, Controls.Gender);
	SyncCharacterStage();
}

void ASolidCharacter::PushEditorBodyToSquad(
	const FString& Name,
	const float Height,
	const float Build,
	const float Gender)
{
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}
	for (const TObjectPtr<ASolidCompanionCharacter>& Companion : GameMode->GetCompanions())
	{
		ASolidCompanionCharacter* Actor = Companion.Get();
		if (!Actor || Actor->IsStagePreview())
		{
			continue;
		}
		if (Actor->GetCharacterDisplayName() != Name)
		{
			continue;
		}
		Actor->SetBodyControls(Height, Build, Gender);
	}
}

void ASolidCharacter::SetEditorWorldHidden(const bool bHideWorld)
{
	if (bHideWorld == bEditorWorldHidden)
	{
		return;
	}
	bEditorWorldHidden = bHideWorld;

	if (USkeletalMeshComponent* CaptainMesh = GetMesh())
	{
		CaptainMesh->SetVisibility(!bHideWorld, true);
	}
	if (bHideWorld)
	{
		if (NameLabelRoot)
		{
			NameLabelRoot->SetVisibility(false, true);
		}
	}
	else
	{
		UpdateNameLabelVisibility();
	}

	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode)
	{
		return;
	}
	for (const TObjectPtr<ASolidCompanionCharacter>& Companion : GameMode->GetCompanions())
	{
		ASolidCompanionCharacter* Actor = Companion.Get();
		if (Actor && !Actor->IsStagePreview())
		{
			Actor->SetActorHiddenInGame(bHideWorld);
		}
	}
}

void ASolidCharacter::SetEditorCameraLocked(const bool bLocked)
{
	if (bLocked == bEditorCameraLocked)
	{
		return;
	}
	bEditorCameraLocked = bLocked;
	if (bLocked)
	{
		if (CameraBoom)
		{
			bEditorSavedBoomCollision = CameraBoom->bDoCollisionTest;
			CameraBoom->bDoCollisionTest = false;
		}
		if (Controller)
		{
			EditorSavedControlRotation = Controller->GetControlRotation();
			FRotator Level = EditorSavedControlRotation;
			Level.Pitch = 0.f;
			Level.Roll = 0.f;
			Controller->SetControlRotation(Level);
		}
		return;
	}

	if (CameraBoom)
	{
		CameraBoom->bDoCollisionTest = bEditorSavedBoomCollision;
	}
	if (Controller)
	{
		Controller->SetControlRotation(EditorSavedControlRotation);
	}
}

void ASolidCharacter::SyncCharacterStage()
{
	ASolidHUD* HUD = GetSolidHUD();
	const bool bOpen = HUD && HUD->IsCharacterEditorOpen();
	if (!bOpen)
	{
		SetEditorCameraLocked(false);
		SetEditorWorldHidden(false);
		if (CharacterStage)
		{
			CharacterStage->Destroy();
			CharacterStage = nullptr;
		}
		return;
	}

	SetEditorWorldHidden(true);
	SetEditorCameraLocked(true);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!CharacterStage)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CharacterStage = World->SpawnActor<ASolidCharacterStage>(
			ASolidCharacterStage::StaticClass(),
			GetActorTransform(),
			Params);
	}
	if (!CharacterStage || !HUD)
	{
		return;
	}

	const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>();
	const USolidCompany* Company = GameMode ? GameMode->GetCompany() : nullptr;
	const FSolidCompanion* Companion = Company
		? Company->GetCompanion(HUD->GetCharacterEditorIndex())
		: nullptr;
	if (Companion)
	{
		CharacterStage->ShowCompanion(*Companion);
	}
	CharacterStage->SetSkeletonLines(HUD->IsCharacterEditorSkeleton());
}

void ASolidCharacter::ToggleCharacterEditorSkeleton()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsCharacterEditorOpen())
	{
		return;
	}
	HUD->SetCharacterEditorSkeleton(!HUD->IsCharacterEditorSkeleton());
	if (CharacterStage)
	{
		CharacterStage->SetSkeletonLines(HUD->IsCharacterEditorSkeleton());
	}
}

void ASolidCharacter::TurnCharacterEditorClockwise()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsCharacterEditorOpen() && CharacterStage)
		{
			CharacterStage->AddPreviewYaw(SolidCharacterEditor::TurnYawDelta(true));
		}
	}
}

void ASolidCharacter::TurnCharacterEditorAnticlockwise()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsCharacterEditorOpen() && CharacterStage)
		{
			CharacterStage->AddPreviewYaw(SolidCharacterEditor::TurnYawDelta(false));
		}
	}
}

void ASolidCharacter::MainMenuMoveUp()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsCharacterEditorOpen())
		{
			HUD->MoveCharacterEditorSlider(-1);
			return;
		}
		if (HUD->IsStrongholdChoiceOpen())
		{
			HUD->MoveStrongholdChoice(-1);
			return;
		}
		if (HUD->IsExitConfirmOpen())
		{
			HUD->MoveExitConfirm(-1);
		}
		else if (HUD->IsPortalOpen())
		{
			HUD->MovePortalSelection(-1);
		}
		else if (!HUD->IsSquadBuilderOpen() && HUD->IsMainMenuOpen())
		{
			HUD->MoveMainMenuSelection(-1);
		}
	}
}

void ASolidCharacter::MainMenuMoveDown()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsCharacterEditorOpen())
		{
			HUD->MoveCharacterEditorSlider(1);
			return;
		}
		if (HUD->IsStrongholdChoiceOpen())
		{
			HUD->MoveStrongholdChoice(1);
			return;
		}
		if (HUD->IsExitConfirmOpen())
		{
			HUD->MoveExitConfirm(1);
		}
		else if (HUD->IsPortalOpen())
		{
			HUD->MovePortalSelection(1);
		}
		else if (!HUD->IsSquadBuilderOpen() && HUD->IsMainMenuOpen())
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

void ASolidCharacter::MainMenuChoose3()
{
	ChooseMainMenuIndex(2);
}

void ASolidCharacter::MainMenuChoose4()
{
	ChooseMainMenuIndex(3);
}

void ASolidCharacter::MainMenuChoose5()
{
	ChooseMainMenuIndex(4);
}

void ASolidCharacter::MainMenuChoose6()
{
	ChooseMainMenuIndex(5);
}

void ASolidCharacter::MainMenuChoose7()
{
	ChooseMainMenuIndex(6);
}

void ASolidCharacter::MainMenuChoose8()
{
	ChooseMainMenuIndex(7);
}

void ASolidCharacter::ChooseMainMenuIndex(const int32 Index)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD)
	{
		return;
	}
	if (HUD->IsExitConfirmOpen())
	{
		if (Index < 0 || Index >= SolidExit::ChoiceCount)
		{
			return;
		}
		HUD->SetExitConfirmIndex(Index);
		ConfirmExitSelection();
		return;
	}
	if (HUD->IsStrongholdChoiceOpen())
	{
		if (Index < 0 || Index >= HUD->GetStrongholdChoiceCount())
		{
			return;
		}
		HUD->SetStrongholdChoiceIndex(Index);
		ConfirmStrongholdChoice();
		return;
	}
	if (HUD->IsPortalOpen())
	{
		SolidPortal::FChoice Choice;
		if (!SolidPortal::FindChoice(Index, Choice))
		{
			return;
		}
		HUD->SetPortalIndex(Index);
		ConfirmPortalSelection();
		return;
	}
	if (HUD->IsSquadBuilderOpen())
	{
		return;
	}
	if (!HUD->IsMainMenuOpen() || Index < 0 || Index >= SolidMainMenu::EntryCount())
	{
		return;
	}
	HUD->SetMainMenuIndex(Index);
	MainMenuConfirm();
}

void ASolidCharacter::ConfirmPortalSelection()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsPortalOpen())
	{
		return;
	}

	SolidPortal::FChoice Choice;
	if (!SolidPortal::FindChoice(HUD->GetPortalIndex(), Choice))
	{
		return;
	}

	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	const bool bTown = Choice.Place == SolidPortal::EPlace::Town;
	const bool bArrived = GameMode && (bTown
		? GameMode->TeleportSquadToTown(Choice.TownId)
		: GameMode->TeleportSquadToTunnelMouth(Choice.TunnelEnd));
	if (!bArrived)
	{
		return;
	}
	HUD->ClosePortal();
}

void ASolidCharacter::ArriveByPortal(const FVector& NewLocation, const float YawDegrees, const float PitchDegrees)
{
	if (IsSquadFormationDrillActive())
	{
		StopSquadFormationDrill();
	}
	SetActorLocation(NewLocation);
	SetActorRotation(FRotator(0.f, YawDegrees, 0.f));
	if (Controller)
	{
		const FRotator View(PitchDegrees, YawDegrees, 0.f);
		Controller->SetControlRotation(View);
		if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			if (APlayerCameraManager* CameraManager = PlayerController->PlayerCameraManager)
			{
				CameraManager->SetGameCameraCutThisFrame();
			}
		}
	}
	ClearClickMove();
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->SetMovementMode(MOVE_Walking);
	}
}

void ASolidCharacter::MainMenuConfirm()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD)
	{
		return;
	}
	if (HUD->IsMessageVisible())
	{
		HUD->CloseMessage();
		return;
	}
	if (HUD->IsExitConfirmOpen())
	{
		ConfirmExitSelection();
		return;
	}
	if (HUD->IsStrongholdChoiceOpen())
	{
		ConfirmStrongholdChoice();
		return;
	}
	if (HUD->IsPortalOpen())
	{
		ConfirmPortalSelection();
		return;
	}
	if (HUD->IsSquadBuilderOpen())
	{
		return;
	}
	if (!HUD->IsMainMenuOpen())
	{
		return;
	}

	FSolidMainMenuEntry Entry;
	if (!SolidMainMenu::FindEntry(HUD->GetMainMenuIndex(), Entry))
	{
		return;
	}

	if (SolidMainMenu::ActionForItem(Entry.Item) == ESolidMainMenuAction::OpenSquad
		&& !SolidSquadBuilder::CanOpen(CurrentMaxSquadSize()))
	{
		return;
	}

	HUD->CloseMainMenu();
	switch (SolidMainMenu::ActionForItem(Entry.Item))
	{
	case ESolidMainMenuAction::OpenPortal:
		HUD->OpenPortal();
		break;
	case ESolidMainMenuAction::ShowJournal:
		HUD->SetJournalVisible(true);
		break;
	case ESolidMainMenuAction::ShowCredits:
		HUD->SetCreditsVisible(true);
		break;
	case ESolidMainMenuAction::Quit:
		HUD->OpenExitConfirm();
		break;
	case ESolidMainMenuAction::OpenSquad:
		if (UWorld* World = GetWorld())
		{
			if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				GameMode->EnsureCompanyAndSquad();
			}
		}
		HUD->OpenSquadBuilder();
		break;
	case ESolidMainMenuAction::OpenCharacterEditor:
		if (UWorld* World = GetWorld())
		{
			if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				GameMode->EnsureCompanyAndSquad();
			}
		}
		HUD->OpenCharacterEditor();
		SyncCharacterStage();
		break;
	case ESolidMainMenuAction::PlaceCaptain:
		if (UWorld* World = GetWorld())
		{
			if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				GameMode->TeleportSquadToReptile();
			}
		}
		break;
	default:
		break;
	}
}

void ASolidCharacter::ConfirmExitSelection()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->AcceptExitConfirm())
	{
		return;
	}
	SolidExit::Perform(this, Cast<APlayerController>(GetController()));
}

int32 ASolidCharacter::CurrentMaxSquadSize() const
{
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			return GameMode->GetMaxSquadSize();
		}
	}
	return 0;
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

void ASolidCharacter::SelectBattlePlanFromFace(const uint8 FaceIndex)
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || FaceIndex >= SolidBattlePlanInput::FaceCount)
	{
		return;
	}
	const int32 Slot = SolidBattlePlanInput::SlotForShoulderAndFace(
		PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftShoulder),
		PlayerController->IsInputKeyDown(EKeys::Gamepad_RightShoulder),
		static_cast<SolidBattlePlanInput::EFace>(FaceIndex));
	if (Slot == INDEX_NONE)
	{
		return;
	}
	SelectBattlePlanSlotFromInput(Slot);
}

void ASolidCharacter::SelectBattlePlanFaceBottom()
{
	SelectBattlePlanFromFace(static_cast<uint8>(SolidBattlePlanInput::EFace::Bottom));
}

void ASolidCharacter::SelectBattlePlanFaceRight()
{
	SelectBattlePlanFromFace(static_cast<uint8>(SolidBattlePlanInput::EFace::Right));
}

void ASolidCharacter::SelectBattlePlanFaceLeft()
{
	SelectBattlePlanFromFace(static_cast<uint8>(SolidBattlePlanInput::EFace::Left));
}

void ASolidCharacter::SelectBattlePlanFaceTop()
{
	SelectBattlePlanFromFace(static_cast<uint8>(SolidBattlePlanInput::EFace::Top));
}

void ASolidCharacter::Move(const FInputActionValue& Value)
{
	if (IsSquadFormationDrillActive() || IsMenuOverlayOpen())
	{
		return;
	}

	if (!SolidClickMove::AllowsDirectMove(IsTrueSight()))
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (!MovementVector.IsNearlyZero())
	{
		ClearClickMove();
	}

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
	if (IsSquadFormationDrillActive() || IsMenuOverlayOpen())
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

void ASolidCharacter::ClearClickMove()
{
	bHasClickMoveDestination = false;
}

bool ASolidCharacter::SampleGroundZ(const float X, const float Y, float& OutZ) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (const ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World))
	{
		OutZ = Streamer->GetHeightAt(FVector(X, Y, 0.f)) + Streamer->CollisionHeightBias;
		return true;
	}
	return false;
}

bool ASolidCharacter::ResolveClickMoveDestination(
	const FVector& RayOrigin,
	const FVector& RayDirection,
	FVector& OutDestination) const
{
	const FVector Direction = RayDirection.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SolidClickMove), /*bTraceComplex=*/true, this);
		Params.AddIgnoredActor(this);
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			for (const TObjectPtr<ASolidCompanionCharacter>& Companion : GameMode->GetCompanions())
			{
				ASolidCompanionCharacter* Actor = Companion.Get();
				if (Actor)
				{
					Params.AddIgnoredActor(Actor);
				}
			}
		}

		const FVector End = RayOrigin + Direction * SolidClickMove::MaxRayCm;
		if (World->LineTraceSingleByChannel(Hit, RayOrigin, End, ECC_Visibility, Params))
		{
			float GroundZ = Hit.ImpactPoint.Z;
			SampleGroundZ(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, GroundZ);
			OutDestination = FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, GroundZ);
			return true;
		}
	}

	float ProbeZ = 0.f;
	if (!SampleGroundZ(RayOrigin.X, RayOrigin.Y, ProbeZ))
	{
		return false;
	}

	return SolidClickMove::IntersectHeight(
		RayOrigin,
		Direction,
		[this](const float X, const float Y)
		{
			float GroundZ = 0.f;
			SampleGroundZ(X, Y, GroundZ);
			return GroundZ;
		},
		OutDestination);
}

void ASolidCharacter::ClickMoveFromInput()
{
	if (!SolidClickMove::AllowsClick(!IsTrueSight(), IsMenuOverlayOpen() || IsSquadFormationDrillActive()))
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	if (!ReadPointerRay(Origin, Direction))
	{
		return;
	}

	FVector Destination = FVector::ZeroVector;
	if (!ResolveClickMoveDestination(Origin, Direction, Destination))
	{
		return;
	}

	const SolidLair::FPassageHit Blocking = PassageOverlapping(Destination);
	if (Blocking.Action != SolidLair::EHit::None)
	{
		ActivatePassage(Blocking);
		return;
	}

	bHasClickMoveDestination = true;
	ClickMoveDestination = Destination;
	UE_LOG(LogSolid, Warning, TEXT("Click move to %s"), *Destination.ToCompactString());
}

bool ASolidCharacter::ReadPointerRay(FVector& OutOrigin, FVector& OutDirection) const
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		return false;
	}
	if (PlayerController->DeprojectMousePositionToWorld(OutOrigin, OutDirection) && !OutDirection.IsNearlyZero())
	{
		return true;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	return ReadPointerPosition(MouseX, MouseY)
		&& PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, OutOrigin, OutDirection)
		&& !OutDirection.IsNearlyZero();
}

bool ASolidCharacter::IsPointerShown() const
{
	float X = 0.f;
	float Y = 0.f;
	if (!ReadPointerPosition(X, Y))
	{
		return false;
	}
	bool bRightDown = false;
	if (const ASolidPlayerController* SolidPC = Cast<ASolidPlayerController>(Controller))
	{
		bRightDown = SolidPC->IsRightMouseHeld();
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		bRightDown = PlayerController->IsInputKeyDown(EKeys::RightMouseButton);
	}
	return SolidPointer::ShouldDraw(true, bRightDown);
}

namespace
{
	template<typename HitsFn>
	SolidLair::FPassageHit SolidClosedMouthBox(UWorld* World, HitsFn Hits)
	{
		if (!World)
		{
			return SolidLair::FPassageHit();
		}

		for (TActorIterator<ASolidLairEntrance> Entrance(World); Entrance; ++Entrance)
		{
			const UStaticMeshComponent* Mouth = Entrance->GetMouthMesh();
			if (Mouth && Hits(Mouth->CalcBounds(Mouth->GetComponentTransform()).GetBox()))
			{
				return SolidLair::HitOnMouth(Entrance->IsLairMouth(), Entrance->GetEndMarker(), false);
			}
		}
		for (TActorIterator<ASolidStrongholdExterior> Exterior(World); Exterior; ++Exterior)
		{
			const UStaticMeshComponent* Passage = Exterior->GetPassageMesh();
			if (Passage && Hits(Passage->CalcBounds(Passage->GetComponentTransform()).GetBox()))
			{
				return SolidLair::HitOnStronghold(Exterior->GetStrongholdCell(), false);
			}
		}
		for (TActorIterator<ASolidStrongholdInterior> Interior(World); Interior; ++Interior)
		{
			const UStaticMeshComponent* Passage = Interior->GetPassageMesh();
			if (Passage && Hits(Passage->CalcBounds(Passage->GetComponentTransform()).GetBox()))
			{
				return SolidLair::HitOnStronghold(Interior->GetStrongholdCell(), true);
			}
		}
		for (TActorIterator<ASolidLairInterior> Interior(World); Interior; ++Interior)
		{
			const TCHAR Ends[] = { USolidWorldMap::XMarker, USolidWorldMap::YMarker };
			for (const TCHAR End : Ends)
			{
				const UStaticMeshComponent* Mouth = Interior->GetEndMouth(End);
				if (Mouth && Hits(Mouth->CalcBounds(Mouth->GetComponentTransform()).GetBox()))
				{
					return SolidLair::HitOnMouth(true, End, true);
				}
			}
		}
		return SolidLair::FPassageHit();
	}
}

SolidLair::FPassageHit ASolidCharacter::AllowPassage(const SolidLair::FPassageHit& Hit) const
{
	const UWorld* World = GetWorld();
	const ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	const SolidLair::FPassageHit Allowed = SolidLair::AllowPassage(
		Hit,
		GameMode && GameMode->AreTunnelsOpen());
	if (!GameMode || Allowed.Action != SolidLair::EHit::Enter)
	{
		return Allowed;
	}

	if (!Allowed.bStronghold)
	{
		return Allowed;
	}
	const USolidGameState* State = GameMode->GetConfiguration();
	const USolidCompany* Company = GameMode->GetCompany();
	const FSolidStronghold* Stronghold = Company ? Company->GetStronghold() : nullptr;
	return State && Stronghold && Stronghold->AcceptsEnter(*State, Stronghold, Allowed.StrongholdCell)
		? Allowed
		: SolidLair::FPassageHit();
}

ASolidStrongholdSite* ASolidCharacter::SiteUnderPointer() const
{
	if (!SolidPointer::AcceptsPointer(IsPointerShown(), IsMenuOverlayOpen() || IsSquadFormationDrillActive()))
	{
		return nullptr;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	if (!ReadPointerRay(Origin, Direction))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ASolidStrongholdSite> Site(World); Site; ++Site)
	{
		const UStaticMeshComponent* Block = Site->GetBlockMesh();
		if (!Block)
		{
			continue;
		}
		const FBox Box = Block->CalcBounds(Block->GetComponentTransform()).GetBox();
		if (SolidPointer::RayHitsBox(Origin, Direction, Box, SolidClickMove::MaxRayCm))
		{
			return *Site;
		}
	}
	return nullptr;
}

ASolidForagePile* ASolidCharacter::NearestForagePile(float& OutGroundDistanceCm) const
{
	OutGroundDistanceCm = 0.f;
	if (!SolidPointer::AcceptsPointer(IsPointerShown(), IsMenuOverlayOpen() || IsSquadFormationDrillActive()))
	{
		return nullptr;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	if (!ReadPointerRay(Origin, Direction))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	ASolidForagePile* Best = nullptr;
	float BestRay = 0.f;
	for (TActorIterator<ASolidForagePile> Pile(World); Pile; ++Pile)
	{
		const UStaticMeshComponent* PileMesh = Pile->GetPileMesh();
		if (!PileMesh)
		{
			continue;
		}
		const FBox Box = PileMesh->CalcBounds(PileMesh->GetComponentTransform()).GetBox();
		float RayCm = 0.f;
		if (!SolidForage::RayBoxDistance(Origin, Direction, Box, SolidClickMove::MaxRayCm, RayCm))
		{
			continue;
		}
		if (Best && RayCm >= BestRay)
		{
			continue;
		}
		Best = *Pile;
		BestRay = RayCm;
		OutGroundDistanceCm = FVector::Dist2D(GetActorLocation(), Best->GetActorLocation());
	}
	return Best;
}

void ASolidCharacter::OfferStrongholdSite(const ASolidStrongholdSite* Site)
{
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	ASolidHUD* HUD = GetSolidHUD();
	if (!Site || !GameMode || !HUD)
	{
		return;
	}

	const USolidCompany* Company = GameMode->GetCompany();
	USolidGameState* State = GameMode->GetConfiguration();
	TArray<FSolidStrongholdChoice> Choices;
	if (!Company
		|| !State
		|| SolidStrongholdTransition::Consider(
			*State,
			Company->GetStronghold(),
			Choices)
			!= SolidStrongholdTransition::EOffer::Choose)
	{
		return;
	}
	HUD->OpenStrongholdChoice(Site->GetCell(), Choices);
}

void ASolidCharacter::ConfirmStrongholdChoice()
{
	ASolidHUD* HUD = GetSolidHUD();
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!HUD || !HUD->IsStrongholdChoiceOpen() || !GameMode)
	{
		return;
	}

	const FIntPoint Cell = HUD->GetStrongholdChoiceCell();
	const ESolidStrongholdSize Size = HUD->GetStrongholdChoiceSize();
	const ESolidStrongholdType Type = HUD->GetStrongholdChoiceType();
	HUD->CloseStrongholdChoice();
	GameMode->EnterChosenStronghold(Cell, Size, Type);
}

SolidLair::FPassageHit ASolidCharacter::PassageOverlapping(const FVector& Location) const
{
	return AllowPassage(SolidClosedMouthBox(GetWorld(), [&Location](const FBox& Box)
	{
		return Box.IsInsideOrOn(Location);
	}));
}

SolidLair::FPassageHit ASolidCharacter::PassageUnderPointer() const
{
	if (!SolidPointer::AcceptsPointer(IsPointerShown(), IsMenuOverlayOpen() || IsSquadFormationDrillActive()))
	{
		return SolidLair::FPassageHit();
	}

	const SolidLair::FPassageHit Inside = PassageOverlapping(GetActorLocation());
	if (Inside.Action != SolidLair::EHit::None)
	{
		return Inside;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	if (!ReadPointerRay(Origin, Direction))
	{
		return SolidLair::FPassageHit();
	}

	return AllowPassage(SolidClosedMouthBox(GetWorld(), [&Origin, &Direction](const FBox& Box)
	{
		return SolidPointer::RayHitsBox(Origin, Direction, Box, SolidClickMove::MaxRayCm);
	}));
}

void ASolidCharacter::ActivatePassage(const SolidLair::FPassageHit& Hit)
{
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode || Hit.Action == SolidLair::EHit::None)
	{
		return;
	}

	ClearClickMove();
	UE_LOG(LogSolid, Warning, TEXT("Closed passage: %s"), SolidLair::PassageTip(Hit));
	if (Hit.Action == SolidLair::EHit::Enter)
	{
		if (Hit.bLair)
		{
			GameMode->TravelToLair(Hit.EndMarker);
		}
		else
		{
			GameMode->EnterStronghold();
		}
		return;
	}
	if (Hit.bLair)
	{
		GameMode->TravelFromLair(Hit.EndMarker);
		return;
	}
	GameMode->ExitStronghold();
}

void ASolidCharacter::UpdateCavePassageHover()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD)
	{
		return;
	}
	const SolidLair::FPassageHit Hit = PassageUnderPointer();
	const UWorld* World = GetWorld();
	const ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	const USolidCompany* Company = GameMode ? GameMode->GetCompany() : nullptr;
	const FSolidStronghold* Stronghold = Company ? Company->GetStronghold() : nullptr;
	FString Tip;
	if (Hit.bStronghold && Hit.Action == SolidLair::EHit::Enter && Stronghold)
	{
		const FString Name = Stronghold->Name.IsEmpty() ? SolidStronghold::Label(*Stronghold) : Stronghold->Name;
		Tip = SolidStronghold::EnterHover(Name);
	}
	else if (Hit.bStronghold && Hit.Action == SolidLair::EHit::Leave)
	{
		Tip = SolidStronghold::LeaveHover;
	}
	else
	{
		Tip = SolidLair::PassageTip(Hit);
	}
	HUD->SetCavePassageTip(Tip);
	float GroundCm = 0.f;
	const ASolidForagePile* Pile = NearestForagePile(GroundCm);
	const bool bNamed = Pile && SolidForage::IsWithinReach(GroundCm);
	HUD->SetForageTip(bNamed ? FString(SolidItemType::NameFor(Pile->GetItemId())) : FString());
}

bool ASolidCharacter::ReadPointerPosition(float& OutX, float& OutY) const
{
	const APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		return false;
	}
	if (PlayerController->GetMousePosition(OutX, OutY))
	{
		return true;
	}
	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			FVector2D Position = FVector2D::ZeroVector;
			if (Viewport->GetMousePosition(Position))
			{
				OutX = Position.X;
				OutY = Position.Y;
				return true;
			}
		}
	}
	return false;
}

void ASolidCharacter::UpdateMenuPointerHover()
{
	ASolidHUD* HUD = GetSolidHUD();
	const bool bChoiceList = HUD && (HUD->IsMainMenuOpen() || HUD->IsExitConfirmOpen() || HUD->IsPortalOpen() || HUD->IsStrongholdChoiceOpen());
	if (!bChoiceList)
	{
		bMenuPointerSampled = false;
		MenuPointerPage = 0;
		return;
	}

	const int32 Page = HUD->IsExitConfirmOpen() ? 5 : (HUD->IsStrongholdChoiceOpen() ? 4 : (HUD->IsPortalOpen() ? 2 : 1));
	if (Page != MenuPointerPage)
	{
		MenuPointerPage = Page;
		bMenuPointerSampled = false;
	}

	float X = 0.f;
	float Y = 0.f;
	if (!ReadPointerPosition(X, Y))
	{
		return;
	}

	const FVector2D Position(X, Y);
	if (bMenuPointerSampled && Position.Equals(MenuPointerSample, 0.5f))
	{
		return;
	}
	bMenuPointerSampled = true;
	MenuPointerSample = Position;
	HUD->SelectChoiceAt(X, Y);
}

void ASolidCharacter::PollClickMovePress()
{
	bool bDown = false;
	if (const ASolidPlayerController* PlayerController = Cast<ASolidPlayerController>(Controller))
	{
		bDown = PlayerController->IsLeftMouseHeld();
	}
	// Travel to the cave loads a new pawn while this button is still down, and the
	// captain arrives facing the other opening. That held press must not travel back.
	if (!SolidClickMove::ConsumePress(bClickSawRelease, bLeftMouseWasDown, bDown))
	{
		return;
	}

	if (ASolidHUD* MessageHUD = GetSolidHUD())
	{
		if (MessageHUD->IsMessageVisible())
		{
			MessageHUD->CloseMessage();
			return;
		}
	}

	float X = 0.f;
	float Y = 0.f;
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsCharacterEditorOpen())
		{
			int32 Slider = 0;
			bool bBar = false;
			float Fraction = 0.f;
			if (ReadPointerPosition(X, Y)
				&& HUD->PressCharacterEditorSlider(X, Y, Slider, bBar, Fraction)
				&& bBar)
			{
				ApplyEditorSliderFraction(Slider, Fraction);
			}
			return;
		}
	}
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsSquadBuilderOpen())
		{
			return;
		}
		if ((HUD->IsMainMenuOpen() || HUD->IsExitConfirmOpen() || HUD->IsPortalOpen() || HUD->IsStrongholdChoiceOpen())
			&& ReadPointerPosition(X, Y)
			&& HUD->SelectChoiceAt(X, Y))
		{
			MainMenuConfirm();
			return;
		}
	}

	const SolidLair::FPassageHit Passage = PassageUnderPointer();
	if (Passage.Action != SolidLair::EHit::None)
	{
		ActivatePassage(Passage);
		return;
	}
	float ForageGroundCm = 0.f;
	if (ASolidForagePile* Pile = NearestForagePile(ForageGroundCm))
	{
		if (SolidForage::IsWithinReach(ForageGroundCm))
		{
			UWorld* World = GetWorld();
			ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
			if (GameMode && GameMode->TakeForagePile(Pile))
			{
				ClearClickMove();
			}
			return;
		}
	}
	if (const ASolidStrongholdSite* Site = SiteUnderPointer())
	{
		OfferStrongholdSite(Site);
		return;
	}
	ClickMoveFromInput();
}

void ASolidCharacter::UpdateInventoryPointer()
{
	ASolidHUD* HUD = GetSolidHUD();
	bool bDown = false;
	if (const ASolidPlayerController* SolidController = Cast<ASolidPlayerController>(Controller))
	{
		bDown = SolidController->IsLeftMouseHeld();
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		bDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
	}

	if (!HUD || !HUD->IsInventoryVisible())
	{
		if (HUD)
		{
			HUD->CancelInventoryDrag();
		}
		bInventoryPointerWasDown = bDown;
		return;
	}

	float X = 0.f;
	float Y = 0.f;
	const bool bHasPoint = ReadPointerPosition(X, Y);
	if (bDown && !bInventoryPointerWasDown && bHasPoint)
	{
		HUD->PressInventory(X, Y);
	}
	else if (!bDown && bInventoryPointerWasDown)
	{
		if (bHasPoint)
		{
			HUD->ReleaseInventory(X, Y);
		}
		else
		{
			HUD->CancelInventoryDrag();
		}
	}
	bInventoryPointerWasDown = bDown;
}

void ASolidCharacter::UpdateSquadBuilderPointer()
{
	ASolidHUD* HUD = GetSolidHUD();
	bool bDown = false;
	if (const ASolidPlayerController* SolidController = Cast<ASolidPlayerController>(Controller))
	{
		bDown = SolidController->IsLeftMouseHeld();
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		bDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
	}

	if (!HUD || !HUD->IsSquadBuilderOpen())
	{
		if (HUD)
		{
			HUD->CancelSquadDrag();
		}
		bSquadPointerWasDown = bDown;
		return;
	}

	float X = 0.f;
	float Y = 0.f;
	const bool bHasPoint = ReadPointerPosition(X, Y);
	if (bDown && !bSquadPointerWasDown && bHasPoint)
	{
		HUD->PressSquadBuilder(X, Y);
	}
	else if (!bDown && bSquadPointerWasDown)
	{
		if (bHasPoint)
		{
			HUD->ReleaseSquadBuilder(X, Y);
		}
		else
		{
			HUD->CancelSquadDrag();
		}
	}
	bSquadPointerWasDown = bDown;
}

void ASolidCharacter::UpdateCharacterEditorPointer()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD || !HUD->IsCharacterEditorOpen())
	{
		if (HUD)
		{
			HUD->EndCharacterEditorSliderDrag();
			HUD->SetSkeletonBoneTip(FString());
		}
		return;
	}

	if (!CharacterStage)
	{
		HUD->SetSkeletonBoneTip(FString());
	}
	else
	{
		float HoverX = 0.f;
		float HoverY = 0.f;
		HUD->SetSkeletonBoneTip(
			ReadPointerPosition(HoverX, HoverY)
				? CharacterStage->BoneUnderPointer(HoverX, HoverY)
				: FString());
	}

	bool bDown = false;
	if (const ASolidPlayerController* SolidController = Cast<ASolidPlayerController>(Controller))
	{
		bDown = SolidController->IsLeftMouseHeld();
	}
	else if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		bDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
	}
	if (!bDown)
	{
		HUD->EndCharacterEditorSliderDrag();
		return;
	}
	if (!HUD->IsCharacterEditorSliderDragging())
	{
		return;
	}

	float X = 0.f;
	float Y = 0.f;
	if (!ReadPointerPosition(X, Y))
	{
		return;
	}
	int32 Slider = 0;
	float Fraction = 0.f;
	if (HUD->DragCharacterEditorSlider(X, Slider, Fraction))
	{
		ApplyEditorSliderFraction(Slider, Fraction);
	}
}

void ASolidCharacter::TickClickMove()
{
	// The pawn key bind misses a plain click while the cursor is up; Slate still
	// sees that press. A second click is what used to get through as a double-click.
	PollClickMovePress();

	if (!bHasClickMoveDestination)
	{
		return;
	}
	if (IsTrueSight())
	{
		ClearClickMove();
		return;
	}
	if (IsMenuOverlayOpen() || IsSquadFormationDrillActive())
	{
		return;
	}

	const FVector Location = GetActorLocation();
	const FVector2D Direction = SolidClickMove::PlanarDirection(
		FVector2D(Location.X, Location.Y),
		FVector2D(ClickMoveDestination.X, ClickMoveDestination.Y));
	if (Direction.IsNearlyZero())
	{
		ClearClickMove();
		return;
	}
	if (!SolidGroundMove::AllowsStep(this, Direction))
	{
		ClearClickMove();
		return;
	}

	AddMovementInput(FVector(Direction.X, Direction.Y, 0.f), 1.f);
}

void ASolidCharacter::UpdatePointerMouseLook()
{
	if (IsSquadFormationDrillActive() || IsMenuOverlayOpen())
	{
		return;
	}

	ASolidPlayerController* PlayerController = Cast<ASolidPlayerController>(Controller);
	if (!PlayerController || !SolidPointer::AllowsMouseLook(PlayerController->IsRightMouseHeld()))
	{
		return;
	}

	// Same value the old mouse Look mapping produced (Y already negated).
	// If that action is quiet, use the raw axes with the same sign.
	FVector2D LookAxis = PlayerController->GetMappedAxis2D(MouseLookAction);
	if (LookAxis.IsNearlyZero())
	{
		float DeltaX = 0.f;
		float DeltaY = 0.f;
		PlayerController->GetInputMouseDelta(DeltaX, DeltaY);
		LookAxis = SolidPointer::MouseLookDelta(DeltaX, DeltaY);
	}

	if (!LookAxis.IsNearlyZero())
	{
		AddControllerYawInput(LookAxis.X);
		AddControllerPitchInput(LookAxis.Y);
	}
}

void ASolidCharacter::StartJump()
{
	if (IsMenuOverlayOpen())
	{
		return;
	}
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (PlayerController
		&& PlayerController->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom)
		&& (PlayerController->IsInputKeyDown(EKeys::Gamepad_LeftShoulder)
			|| PlayerController->IsInputKeyDown(EKeys::Gamepad_RightShoulder)))
	{
		SelectBattlePlanFaceBottom();
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
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsSquadBuilderOpen())
		{
			float X = 0.f;
			float Y = 0.f;
			if (ReadPointerPosition(X, Y))
			{
				HUD->ScrollSquadBuilder(X, Y, Value.Get<float>());
			}
			return;
		}
	}
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
	if (IsSquadFormationDrillActive() || IsMenuOverlayOpen())
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
