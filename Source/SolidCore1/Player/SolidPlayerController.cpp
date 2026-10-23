#include "SolidPlayerController.h"
#include "HUD/SolidPointer.h"
#include "SolidCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ASolidPlayerController::ASolidPlayerController()
{
	// Shown so Slate owns the cursor, but None so it draws nothing.
	// The HUD paints the bronze arrow. A hidden game cursor lets Windows draw its arrow on top.
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::None;
	CurrentMouseCursor = EMouseCursor::None;
}

void ASolidPlayerController::BeginPlay()
{
	Super::BeginPlay();
	ApplyPointerMode();
}

void ASolidPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// GameAndUI and the editor will put the capture mode and the Windows
	// cursor back. Put them back each frame while this pawn is in play.
	EnsurePointerCaptureMode();
	HideHardwareCursor();
}

void ASolidPlayerController::ApplyPointerMode()
{
	// GameAndUI tracks a mouse position without capturing on sight.
	// CaptureDuringRightMouseDown is applied after, so holding the right
	// button locks the pointer and feeds MouseX/MouseY to look.
	FInputModeGameAndUI Mode;
	Mode.SetHideCursorDuringCapture(true);
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture);
	SetInputMode(Mode);

	EnsurePointerCaptureMode();
	HideHardwareCursor();
}

void ASolidPlayerController::EnsurePointerCaptureMode()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	if (Viewport->GetMouseCaptureMode() != EMouseCaptureMode::CaptureDuringRightMouseDown)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringRightMouseDown);
	}
	Viewport->SetMouseLockMode(EMouseLockMode::LockOnCapture);
	Viewport->SetHideCursorDuringCapture(true);
}

void ASolidPlayerController::HideHardwareCursor()
{
	// Slate draws this cursor. None is empty, so the Windows arrow does not sit
	// on the bronze pointer. The platform cursor object is not exported to game modules.
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::None;
	CurrentMouseCursor = EMouseCursor::None;
}

bool ASolidPlayerController::IsMouseButtonHeld(const FKey& Button) const
{
	const bool bSlateDown = FSlateApplication::IsInitialized()
		&& FSlateApplication::Get().GetPressedMouseButtons().Contains(Button);
	return SolidPointer::IsRightMouseHeld(IsInputKeyDown(Button), bSlateDown);
}

bool ASolidPlayerController::IsRightMouseHeld() const
{
	if (IsMouseButtonHeld(EKeys::RightMouseButton))
	{
		return true;
	}
	const ASolidCharacter* Captain = Cast<ASolidCharacter>(GetPawn());
	return Captain && Captain->IsScriptRightMouseHeld();
}

bool ASolidPlayerController::IsLeftMouseHeld() const
{
	return IsMouseButtonHeld(EKeys::LeftMouseButton);
}

FVector2D ASolidPlayerController::GetMappedAxis2D(const UInputAction* Action) const
{
	if (const UEnhancedPlayerInput* EnhancedInput = Cast<UEnhancedPlayerInput>(PlayerInput))
	{
		if (Action)
		{
			const FInputActionValue Value = EnhancedInput->GetActionValue(Action);
			if (Value.GetValueType() == EInputActionValueType::Axis2D)
			{
				return Value.Get<FVector2D>();
			}
		}
	}
	return FVector2D::ZeroVector;
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
