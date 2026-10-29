#include "SolidTrueSight.h"
#include "SolidCharacter.h"
#include "GameFramework/SpringArmComponent.h"

void ASolidCharacter::ApplyTrueSightCamera()
{
	if (!CameraBoom)
	{
		return;
	}

	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->TargetOffset = FVector(0.f, 0.f, SolidSight::TrueSightEyeHeightCm);
	CameraBoom->SocketOffset = FVector(SolidSight::TrueSightForwardCm, 0.f, 0.f);
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bDoCollisionTest = false;
}
