#include "SolidCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace SolidCaptainInput
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

bool ASolidCharacter::ValueForMappedKey(
	FKey Key,
	const UInputAction*& OutAction,
	FVector2D& OutStick) const
{
	OutAction = nullptr;
	OutStick = FVector2D::ZeroVector;
	if (!DefaultMappingContext)
	{
		return false;
	}

	for (const FEnhancedActionKeyMapping& Mapping : DefaultMappingContext->GetMappings())
	{
		if (Mapping.Key != Key || !Mapping.Action)
		{
			continue;
		}

		FInputActionValue Value(FVector(1.f, 0.f, 0.f));
		for (const TObjectPtr<UInputModifier>& Modifier : Mapping.Modifiers)
		{
			if (Modifier)
			{
				Value = Modifier->ModifyRaw(nullptr, Value, 0.f);
			}
		}
		const FVector Axes = Value.Get<FVector>();
		OutAction = Mapping.Action;
		OutStick = FVector2D(Axes.X, Axes.Y);
		return true;
	}
	return false;
}

bool ASolidCharacter::StickForKey(FKey Key, FVector2D& OutStick)
{
	EnsureRuntimeInputAssets();
	const UInputAction* Action = nullptr;
	if (!ValueForMappedKey(Key, Action, OutStick) || Action != MoveAction)
	{
		OutStick = FVector2D::ZeroVector;
		return false;
	}
	return true;
}

void ASolidCharacter::FlushScriptLook()
{
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController)
	{
		return;
	}
	PlayerController->UpdateRotation(0.f);
	PlayerController->RotationInput = FRotator::ZeroRotator;
}

void ASolidCharacter::SimulateKey(FKey Key)
{
	EnsureRuntimeInputAssets();
	const UInputAction* Action = nullptr;
	FVector2D Stick = FVector2D::ZeroVector;
	if (!ValueForMappedKey(Key, Action, Stick) || Stick.IsNearlyZero())
	{
		return;
	}

	bScriptInput = true;
	if (Action == MoveAction)
	{
		Move(FInputActionValue(Stick));
	}
	else if (Action == LookAction)
	{
		Look(FInputActionValue(Stick));
	}
	bScriptInput = false;
	if (Action == LookAction)
	{
		FlushScriptLook();
	}
}

void ASolidCharacter::SimulateMouseLook(const float DeltaX, const float DeltaY)
{
	if (FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY))
	{
		return;
	}

	bScriptInput = true;
	Look(FInputActionValue(FVector2D(DeltaX, DeltaY)));
	bScriptInput = false;
	FlushScriptLook();
}

void ASolidCharacter::SimulateLeftClick(const float ScreenX, const float ScreenY)
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		PlayerController->SetMouseLocation(static_cast<int32>(ScreenX), static_cast<int32>(ScreenY));
	}

	bScriptInput = true;
	bScriptClickDown = true;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		const FInputKeyEventArgs Press = FInputKeyEventArgs::CreateSimulated(
			EKeys::LeftMouseButton, IE_Pressed, 1.f);
		PlayerController->InputKey(Press);
	}
	PollClickMovePress();
	bScriptClickDown = false;
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		const FInputKeyEventArgs Release = FInputKeyEventArgs::CreateSimulated(
			EKeys::LeftMouseButton, IE_Released, 0.f);
		PlayerController->InputKey(Release);
	}
	PollClickMovePress();
	bScriptInput = false;
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

	if (!MouseLookAction)
	{
		MouseLookAction = NewObject<UInputAction>(this, TEXT("IA_MouseLook"), RF_Transient);
		MouseLookAction->ValueType = EInputActionValueType::Axis2D;
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
			Mapping.Modifiers.Add(SolidCaptainInput::MakeSwizzleYXZ(DefaultMappingContext));
		}
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCaptainInput::MakeSwizzleYXZ(DefaultMappingContext));
		}
		DefaultMappingContext->MapKey(MoveAction, EKeys::D);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeNegate(DefaultMappingContext));
		}

		// Gamepad left stick
		DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::Gamepad_LeftY);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeSwizzleYXZ(DefaultMappingContext));
		}

		// Gamepad right stick
		DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::Gamepad_RightY);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCaptainInput::MakeSwizzleYXZ(DefaultMappingContext));
		}

		DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
		DefaultMappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

		DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
		DefaultMappingContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	}

	// Mouse axes stay off LookAction. Moving the pointer must not turn the camera;
	// MouseLook is applied only while the right button is held.
	if (DefaultMappingContext && MouseLookAction && LookAction)
	{
		bool bMouseLookMapped = false;
		for (const FEnhancedActionKeyMapping& Mapping : DefaultMappingContext->GetMappings())
		{
			if (Mapping.Action == MouseLookAction && Mapping.Key == EKeys::MouseX)
			{
				bMouseLookMapped = true;
				break;
			}
		}
		DefaultMappingContext->UnmapKey(LookAction, EKeys::MouseX);
		DefaultMappingContext->UnmapKey(LookAction, EKeys::MouseY);
		if (!bMouseLookMapped)
		{
			DefaultMappingContext->MapKey(MouseLookAction, EKeys::MouseX);
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MouseLookAction, EKeys::MouseY);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeNegate(DefaultMappingContext));
			Mapping.Modifiers.Add(SolidCaptainInput::MakeSwizzleYXZ(DefaultMappingContext));
		}
	}

	if (!ZoomMappingContext && ZoomAction)
	{
		ZoomMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Zoom"), RF_Transient);
		ZoomMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	}
}
