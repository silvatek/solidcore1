#include "SolidCharacter.h"
#include "Companion/SolidCompanionCharacter.h"
#include "HUD/SolidPointer.h"
#include "SolidClickMove.h"
#include "Strongholds/SolidDungeonPassage.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
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
#include "SolidGameMode.h"
#include "HUD/SolidHUD.h"
#include "Menus/SolidMainMenu.h"
#include "Menus/SolidPartyBuilder.h"
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
	UpdatePointerMouseLook();
	UpdateMenuPointerHover();
	UpdateCavePassageHover();
	TickClickMove();
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
	PlayerInputComponent->BindKey(EKeys::F12, IE_Pressed, this, &ASolidCharacter::ToggleDebugPanelFromInput);
	PlayerInputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ASolidCharacter::CloseMenuOverlayFromInput);
	PlayerInputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ASolidCharacter::MainMenuMoveUp);
	PlayerInputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ASolidCharacter::MainMenuMoveDown);
	PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &ASolidCharacter::MainMenuConfirm);
	PlayerInputComponent->BindKey(EKeys::One, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
	PlayerInputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ASolidCharacter::MainMenuChoose3);
	PlayerInputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ASolidCharacter::MainMenuChoose4);
	PlayerInputComponent->BindKey(EKeys::Five, IE_Pressed, this, &ASolidCharacter::MainMenuChoose5);
	PlayerInputComponent->BindKey(EKeys::Six, IE_Pressed, this, &ASolidCharacter::MainMenuChoose6);
	PlayerInputComponent->BindKey(EKeys::NumPadOne, IE_Pressed, this, &ASolidCharacter::MainMenuChoose1);
	PlayerInputComponent->BindKey(EKeys::NumPadTwo, IE_Pressed, this, &ASolidCharacter::MainMenuChoose2);
	PlayerInputComponent->BindKey(EKeys::NumPadThree, IE_Pressed, this, &ASolidCharacter::MainMenuChoose3);
	PlayerInputComponent->BindKey(EKeys::NumPadFour, IE_Pressed, this, &ASolidCharacter::MainMenuChoose4);
	PlayerInputComponent->BindKey(EKeys::NumPadFive, IE_Pressed, this, &ASolidCharacter::MainMenuChoose5);
	PlayerInputComponent->BindKey(EKeys::NumPadSix, IE_Pressed, this, &ASolidCharacter::MainMenuChoose6);
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
			|| HUD->IsPartyBuilderOpen()
			|| HUD->IsCreditsVisible()
			|| HUD->IsJournalVisible();
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
			bInStronghold = GameMode->IsInDungeon();
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
	}
}

void ASolidCharacter::MainMenuMoveUp()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsPortalOpen())
		{
			HUD->MovePortalSelection(-1);
		}
		else if (HUD->IsPartyBuilderOpen())
		{
			HUD->MovePartyBuilderSelection(-1);
		}
		else if (HUD->IsMainMenuOpen())
		{
			HUD->MoveMainMenuSelection(-1);
		}
	}
}

void ASolidCharacter::MainMenuMoveDown()
{
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if (HUD->IsPortalOpen())
		{
			HUD->MovePortalSelection(1);
		}
		else if (HUD->IsPartyBuilderOpen())
		{
			HUD->MovePartyBuilderSelection(1);
		}
		else if (HUD->IsMainMenuOpen())
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

void ASolidCharacter::ChooseMainMenuIndex(const int32 Index)
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD)
	{
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
	if (HUD->IsPartyBuilderOpen())
	{
		TogglePartyBuilderRow(Index);
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
	if (!GameMode || !GameMode->TeleportPartyToTown(Choice.TownId))
	{
		return;
	}
	HUD->ClosePortal();
}

void ASolidCharacter::ArriveByPortal(const FVector& NewLocation, const float YawDegrees)
{
	if (IsPartyFormationDrillActive())
	{
		StopPartyFormationDrill();
	}
	SetActorLocation(NewLocation);
	const FRotator Facing(0.f, YawDegrees, 0.f);
	SetActorRotation(Facing);
	if (Controller)
	{
		Controller->SetControlRotation(Facing);
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
	if (HUD->IsPortalOpen())
	{
		ConfirmPortalSelection();
		return;
	}
	if (HUD->IsPartyBuilderOpen())
	{
		TogglePartyBuilderRow(HUD->GetPartyBuilderIndex());
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

	if (SolidMainMenu::ActionForItem(Entry.Item) == ESolidMainMenuAction::OpenParty
		&& !SolidPartyBuilder::CanOpen(CurrentMaxPartySize()))
	{
		return;
	}

	HUD->CloseMainMenu();
	switch (SolidMainMenu::ActionForItem(Entry.Item))
	{
	case ESolidMainMenuAction::OpenPortal:
		HUD->OpenPortal();
		break;
	case ESolidMainMenuAction::StartTestDrill:
		StartPartyFormationDrill();
		break;
	case ESolidMainMenuAction::ShowJournal:
		HUD->SetJournalVisible(true);
		break;
	case ESolidMainMenuAction::ShowCredits:
		HUD->SetCreditsVisible(true);
		break;
	case ESolidMainMenuAction::OpenParty:
		if (UWorld* World = GetWorld())
		{
			if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				GameMode->EnsureCompanyAndParty();
			}
		}
		HUD->OpenPartyBuilder();
		break;
	case ESolidMainMenuAction::TravelDungeon:
		if (UWorld* World = GetWorld())
		{
			if (ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
			{
				if (GameMode->IsInDungeon())
				{
					GameMode->TravelToWorld();
				}
				else
				{
					GameMode->TravelToDungeon();
				}
			}
		}
		break;
	default:
		break;
	}
}

int32 ASolidCharacter::CurrentMaxPartySize() const
{
	if (UWorld* World = GetWorld())
	{
		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			return GameMode->GetMaxPartySize();
		}
	}
	return 0;
}

void ASolidCharacter::TogglePartyBuilderRow(const int32 Index)
{
	ASolidHUD* HUD = GetSolidHUD();
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!HUD || !GameMode)
	{
		return;
	}

	GameMode->EnsureCompanyAndParty();
	TArray<SolidPartyBuilder::FRow> Rows;
	SolidPartyBuilder::CollectRows(GameMode->GetParty(), Rows);
	if (!Rows.IsValidIndex(Index))
	{
		return;
	}

	HUD->SetPartyBuilderIndex(Index);
	if (SolidPartyBuilder::Toggle(GameMode->GetParty(), Rows[Index].CompanyIndex, GameMode->GetMaxPartySize()))
	{
		GameMode->RefreshFieldedParty();
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
	if (!SolidClickMove::AllowsClick(!IsTrueSight(), IsMenuOverlayOpen() || IsPartyFormationDrillActive()))
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

	const SolidDungeonPassage::EHit Blocking = CavePassageOverlapping(Destination);
	if (Blocking != SolidDungeonPassage::EHit::None)
	{
		ActivateCavePassage(Blocking);
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
	SolidDungeonPassage::EHit SolidCaveMouthBox(UWorld* World, HitsFn Hits)
	{
		if (!World)
		{
			return SolidDungeonPassage::EHit::None;
		}

		for (TActorIterator<ASolidDungeonEntrance> Entrance(World); Entrance; ++Entrance)
		{
			const UStaticMeshComponent* Mouth = Entrance->GetMouthMesh();
			if (Mouth && Hits(Mouth->CalcBounds(Mouth->GetComponentTransform()).GetBox()))
			{
				return SolidDungeonPassage::EHit::Enter;
			}
		}
		for (TActorIterator<ASolidDungeonInterior> Interior(World); Interior; ++Interior)
		{
			const UStaticMeshComponent* Mouth = Interior->GetMouthMesh();
			if (Mouth && Hits(Mouth->CalcBounds(Mouth->GetComponentTransform()).GetBox()))
			{
				return SolidDungeonPassage::EHit::Leave;
			}
		}
		return SolidDungeonPassage::EHit::None;
	}
}

SolidDungeonPassage::EHit ASolidCharacter::CavePassageOverlapping(const FVector& Location) const
{
	return SolidCaveMouthBox(GetWorld(), [&Location](const FBox& Box)
	{
		return Box.IsInsideOrOn(Location);
	});
}

SolidDungeonPassage::EHit ASolidCharacter::CavePassageUnderPointer() const
{
	if (!SolidDungeonPassage::AcceptsPointer(IsPointerShown(), IsMenuOverlayOpen() || IsPartyFormationDrillActive()))
	{
		return SolidDungeonPassage::EHit::None;
	}

	const SolidDungeonPassage::EHit Inside = CavePassageOverlapping(GetActorLocation());
	if (Inside != SolidDungeonPassage::EHit::None)
	{
		return Inside;
	}

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ZeroVector;
	if (!ReadPointerRay(Origin, Direction))
	{
		return SolidDungeonPassage::EHit::None;
	}

	return SolidCaveMouthBox(GetWorld(), [&Origin, &Direction](const FBox& Box)
	{
		return SolidDungeonPassage::RayHitsBox(Origin, Direction, Box, SolidClickMove::MaxRayCm);
	});
}

void ASolidCharacter::ActivateCavePassage(const SolidDungeonPassage::EHit Hit)
{
	UWorld* World = GetWorld();
	ASolidGameMode* GameMode = World ? World->GetAuthGameMode<ASolidGameMode>() : nullptr;
	if (!GameMode || Hit == SolidDungeonPassage::EHit::None)
	{
		return;
	}

	ClearClickMove();
	UE_LOG(LogSolid, Warning, TEXT("Cave passage: %s"), SolidDungeonPassage::TipText(Hit));
	if (Hit == SolidDungeonPassage::EHit::Enter)
	{
		GameMode->TravelToDungeon();
		return;
	}
	GameMode->TravelToWorld();
}

void ASolidCharacter::UpdateCavePassageHover()
{
	ASolidHUD* HUD = GetSolidHUD();
	if (!HUD)
	{
		return;
	}
	HUD->SetCavePassageTip(SolidDungeonPassage::TipText(CavePassageUnderPointer()));
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
	const bool bChoiceList = HUD && (HUD->IsMainMenuOpen() || HUD->IsPortalOpen() || HUD->IsPartyBuilderOpen());
	if (!bChoiceList)
	{
		bMenuPointerSampled = false;
		MenuPointerPage = 0;
		return;
	}

	const int32 Page = HUD->IsPartyBuilderOpen() ? 3 : (HUD->IsPortalOpen() ? 2 : 1);
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

	float X = 0.f;
	float Y = 0.f;
	if (ASolidHUD* HUD = GetSolidHUD())
	{
		if ((HUD->IsMainMenuOpen() || HUD->IsPortalOpen() || HUD->IsPartyBuilderOpen())
			&& ReadPointerPosition(X, Y)
			&& HUD->SelectChoiceAt(X, Y))
		{
			MainMenuConfirm();
			return;
		}
	}

	const SolidDungeonPassage::EHit Passage = CavePassageUnderPointer();
	if (Passage != SolidDungeonPassage::EHit::None)
	{
		ActivateCavePassage(Passage);
		return;
	}
	ClickMoveFromInput();
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
	if (IsMenuOverlayOpen() || IsPartyFormationDrillActive())
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

	AddMovementInput(FVector(Direction.X, Direction.Y, 0.f), 1.f);
}

void ASolidCharacter::UpdatePointerMouseLook()
{
	if (IsPartyFormationDrillActive() || IsMenuOverlayOpen())
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
