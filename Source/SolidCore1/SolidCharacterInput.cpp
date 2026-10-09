#include "SolidCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"
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

		// Mouse look
		DefaultMappingContext->MapKey(LookAction, EKeys::MouseX);
		{
			FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(LookAction, EKeys::MouseY);
			Mapping.Modifiers.Add(SolidCaptainInput::MakeNegate(DefaultMappingContext));
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

	if (!ZoomMappingContext && ZoomAction)
	{
		ZoomMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Zoom"), RF_Transient);
		ZoomMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);
	}
}
