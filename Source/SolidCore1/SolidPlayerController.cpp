#include "SolidPlayerController.h"
#include "HUD/SolidPointer.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GenericPlatform/GenericApplication.h"
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
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::None;
	CurrentMouseCursor = EMouseCursor::None;

	if (!FSlateApplication::IsInitialized())
	{
		return;
	}

	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	FVector2D Unused = FVector2D::ZeroVector;
	const bool bOverGame = Viewport && Viewport->GetMousePosition(Unused);
	if (!bOverGame && !IsRightMouseHeld())
	{
		return;
	}

	const TSharedPtr<GenericApplication> Platform = FSlateApplication::Get().GetPlatformApplication();
	if (Platform.IsValid() && Platform->Cursor.IsValid())
	{
		Platform->Cursor->SetType(EMouseCursor::None);
	}
}

bool ASolidPlayerController::IsRightMouseHeld() const
{
	const bool bSlateDown = FSlateApplication::IsInitialized()
		&& FSlateApplication::Get().GetPressedMouseButtons().Contains(EKeys::RightMouseButton);
	return SolidPointer::IsRightMouseHeld(IsInputKeyDown(EKeys::RightMouseButton), bSlateDown);
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
