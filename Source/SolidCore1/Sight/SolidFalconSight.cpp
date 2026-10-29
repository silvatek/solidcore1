#include "SolidFalconSight.h"
#include "SolidCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/RotationMatrix.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"

void ASolidCharacter::ApplyFalconSightCamera()
{
	if (!CameraBoom)
	{
		return;
	}

	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FVector Origin = CameraBoom->GetComponentLocation();
	const FVector2D Captain(Origin.X, Origin.Y);
	FVector2D Horizontal = FVector2D::ZeroVector;
	float GroundZ = Origin.Z;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World))
		{
			if (const USolidTerrainMap* Map = Streamer->GetTerrainMap())
			{
				if (Map->IsBuilt())
				{
					const float MaxCm = FVector2D::Distance(Map->GetWorldMinXY(), Map->GetWorldMaxXY());
					const FVector2D Stand = SolidSight::FalconClearXY(
						Captain,
						ArmRot,
						200.f,
						MaxCm,
						[Map](const FVector2D& At)
						{
							return SolidSight::CameraGroundFog(*Map, At.X, At.Y);
						});
					Horizontal = Stand - Captain;
				}
			}
			GroundZ = Streamer->GetHeightAt(Origin + FVector(Horizontal.X, Horizontal.Y, 0.f))
				+ Streamer->CollisionHeightBias;
		}
	}

	const float Alpha = SolidSight::FalconZoomAlpha(UserZoomArmLength, CameraZoomMin, CameraZoomMax);
	const FVector WorldOffset(
		Horizontal.X,
		Horizontal.Y,
		GroundZ + SolidSight::FalconHeightCm(Alpha) - Origin.Z);

	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FRotationMatrix(ArmRot).InverseTransformVector(WorldOffset);
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bDoCollisionTest = false;
}

void ASolidCharacter::AimFalconAtCaptain()
{
	if (!CameraBoom)
	{
		return;
	}

	ApplyFalconSightCamera();
	if (!Controller)
	{
		return;
	}

	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FVector Camera = CameraBoom->GetComponentLocation()
		+ FRotationMatrix(ArmRot).TransformVector(CameraBoom->SocketOffset);
	Controller->SetControlRotation(SolidSight::FalconLookAtCaptain(Camera, GetActorLocation()));
	ApplyFalconSightCamera();
}
