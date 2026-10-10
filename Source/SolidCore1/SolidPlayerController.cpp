#include "SolidPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputMappingContext.h"

ASolidPlayerController::ASolidPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::None;
	CurrentMouseCursor = EMouseCursor::None;
}

void ASolidPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ApplyPointerMode();
}

void ASolidPlayerController::ApplyPointerMode()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::None;
	CurrentMouseCursor = EMouseCursor::None;

	// GameAndUI keeps a mouse position. The hardware cursor stays hidden;
	// the HUD draws the bronze pointer.
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(true);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(Mode);
	bShowMouseCursor = true;
	CurrentMouseCursor = EMouseCursor::None;

	if (UWorld* World = GetWorld())
	{
		if (UGameViewportClient* Viewport = World->GetGameViewport())
		{
			Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
			Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
			Viewport->SetHideCursorDuringCapture(true);
		}
	}
}

void ASolidPlayerController::SetMouseLookHeld(const bool bHeld)
{
	if (bHeld)
	{
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		SetInputMode(Mode);
		bShowMouseCursor = false;
		return;
	}

	ApplyPointerMode();
}

void ASolidPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
		}
	}
}
