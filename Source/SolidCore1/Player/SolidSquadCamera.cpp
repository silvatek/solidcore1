#include "SolidCharacter.h"
#include "SolidCameraFog.h"
#include "Camera/CameraComponent.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"

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

void ASolidCharacter::ApplyRavenSightCamera()
{
	if (!CameraBoom)
	{
		return;
	}

	CameraBoom->TargetArmLength = UserZoomArmLength;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FVector::ZeroVector;
	CameraTerrainLiftCm = 0.f;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bDoCollisionTest = true;
}

void ASolidCharacter::ApplyFalconSightCamera()
{
	if (!CameraBoom)
	{
		return;
	}

	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FVector Horizontal(SolidSight::FalconHorizontalOffset(ArmRot), 0.f);
	const FVector Origin = CameraBoom->GetComponentLocation();
	float GroundZ = Origin.Z;
	if (UWorld* World = GetWorld())
	{
		if (const ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World))
		{
			GroundZ = Streamer->GetHeightAt(Origin + Horizontal) + Streamer->CollisionHeightBias;
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
	if (!Controller || !CameraBoom)
	{
		return;
	}

	ApplyFalconSightCamera();
	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FVector Camera = CameraBoom->GetComponentLocation()
		+ FRotationMatrix(ArmRot).TransformVector(CameraBoom->SocketOffset);
	Controller->SetControlRotation(SolidSight::FalconLookAtCaptain(Camera, GetActorLocation()));
	ApplyFalconSightCamera();
}

void ASolidCharacter::UpdateSquadCameraFraming(float DeltaTime)
{
	if (IsTrueSight())
	{
		ApplyTrueSightCamera();
		return;
	}

	if (IsFalconSight())
	{
		ApplyFalconSightCamera();
		return;
	}

	if (!CameraBoom || !bFrameCompanions)
	{
		return;
	}

	// Only the locally controlled Captain drives the Squad framing camera.
	if (!IsLocallyControlled())
	{
		return;
	}

	struct FSquadMember
	{
		FVector Location;
		float CapsuleHalfHeight;
	};

	// Squad ≈ Captain + Companions currently present (not the full Company).
	TArray<FSquadMember, TInlineAllocator<8>> SquadMembers;
	{
		float HalfHeight = 96.f;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
		SquadMembers.Add({ GetActorLocation(), HalfHeight });
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
		{
			if (!IsValid(*It) || It->IsStagePreview())
			{
				continue;
			}
			if (FVector::DistSquared2D(GetActorLocation(), It->GetActorLocation())
				> FramingMaxMemberCm * FramingMaxMemberCm)
			{
				continue;
			}
			float HalfHeight = 96.f;
			if (const UCapsuleComponent* Capsule = It->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
			SquadMembers.Add({ It->GetActorLocation(), HalfHeight });
		}
	}

	const bool bSquadHasCompanions = SquadMembers.Num() > 1;

	if (bDisableBoomCollisionWhileFraming)
	{
		// SC1-0025 popped narrower when the boom probe hit hills and collapsed arm length.
		CameraBoom->bDoCollisionTest = !bSquadHasCompanions;
	}

	FVector DesiredTargetOffset = FVector::ZeroVector;
	float FramingFitArm = 0.f;

	if (bSquadHasCompanions)
	{
		FVector Center = FVector::ZeroVector;
		for (const FSquadMember& Member : SquadMembers)
		{
			Center += Member.Location;
		}
		Center /= static_cast<float>(SquadMembers.Num());

		FVector ToCenter = Center - GetActorLocation();
		ToCenter.Z *= 0.45f;
		DesiredTargetOffset = ToCenter;

		FRotator ViewRot = GetControlRotation();
		if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			ViewRot = PC->GetControlRotation();
		}
		const FRotationMatrix ViewMatrix(ViewRot);
		const FVector CamRight = ViewMatrix.GetUnitAxis(EAxis::Y);
		const FVector CamUp = ViewMatrix.GetUnitAxis(EAxis::Z);

		float MaxRight = 0.f;
		float MaxUp = 0.f;
		for (const FSquadMember& Member : SquadMembers)
		{
			const FVector Delta = Member.Location - Center;
			MaxRight = FMath::Max(MaxRight, FMath::Abs(FVector::DotProduct(Delta, CamRight)) + 45.f);
			MaxUp = FMath::Max(
				MaxUp,
				FMath::Abs(FVector::DotProduct(Delta, CamUp)) + Member.CapsuleHalfHeight);
		}

		MaxRight += FramingPadding;
		MaxUp += FramingPadding * 0.65f;

		float VerticalFovDeg = FollowCamera ? FollowCamera->FieldOfView : 90.f;
		VerticalFovDeg = FMath::Clamp(VerticalFovDeg, 40.f, 120.f);
		const float HalfVFovRad = FMath::DegreesToRadians(VerticalFovDeg * 0.5f);
		const float HalfHFovRad = FMath::Atan(FMath::Tan(HalfVFovRad) * FramingAspectRatio);

		const float DistForWidth = MaxRight / FMath::Max(FMath::Tan(HalfHFovRad), 0.05f);
		const float DistForHeight = MaxUp / FMath::Max(FMath::Tan(HalfVFovRad), 0.05f);
		FramingFitArm = FMath::Clamp(
			FMath::Max(DistForWidth, DistForHeight),
			FramingMinArmLength,
			FramingMaxArmLength);
	}

	// Wheel zoom owns arm length by default. Framing may only pull out if explicitly allowed
	// (otherwise zoom-in hits CameraZoomMin while arm stays long — feels "stuck").
	float DesiredArmLength = UserZoomArmLength;
	if (bSquadHasCompanions && bFramingCanOverrideZoom && FramingFitArm > DesiredArmLength)
	{
		DesiredArmLength = FramingFitArm;
	}
	DesiredArmLength = FMath::Clamp(DesiredArmLength, CameraZoomMin, CameraZoomMax);

	CameraBoom->TargetOffset = FMath::VInterpTo(
		CameraBoom->TargetOffset, DesiredTargetOffset, DeltaTime, FramingOffsetInterpSpeed);

	const float ArmInterpSpeed = (DesiredArmLength > CameraBoom->TargetArmLength)
		? FramingZoomOutSpeed
		: FramingZoomInSpeed;
	CameraBoom->TargetArmLength = FMath::FInterpTo(
		CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, ArmInterpSpeed);
}

void ASolidCharacter::ClampCameraAboveTerrain(float DeltaTime)
{
	if (IsTrueSight() || IsFalconSight() || !CameraBoom || !IsLocallyControlled())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
	if (!Streamer)
	{
		return;
	}

	// Predict camera location the same way USpringArmComponent does (without terrain lift).
	const FRotator ArmRot = CameraBoom->GetTargetRotation();
	const FRotationMatrix ArmMatrix(ArmRot);
	const FVector ArmOrigin = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
	const FVector SocketXY = ArmMatrix.TransformVector(
		FVector(CameraBoom->SocketOffset.X, CameraBoom->SocketOffset.Y, 0.f));
	auto CameraAtScale = [&](float Scale)
	{
		return ArmOrigin
			- ArmRot.Vector() * (CameraBoom->TargetArmLength * Scale)
			+ SocketXY;
	};

	// Keep the camera over clear trail. Do not sample the camera to decide fog amount.
	if (const USolidTerrainMap* Map = Streamer->GetTerrainMap())
	{
		if (Map->IsBuilt())
		{
			const float ClearScale = SolidCameraFog::MaxClearScale([&](float Scale)
			{
				const FVector At = CameraAtScale(Scale);
				return Map->GetNearestPoint(At.X, At.Y).Fog;
			});
			if (ClearScale < 1.f - KINDA_SMALL_NUMBER)
			{
				CameraBoom->TargetArmLength *= ClearScale;
			}
		}
	}

	const FVector DesiredCam = CameraAtScale(1.f);

	const float TerrainZ = Streamer->GetHeightAt(DesiredCam) + Streamer->CollisionHeightBias;
	const float MinCamZ = TerrainZ + CameraTerrainClearance;
	const float NeededLift = FMath::Clamp(
		FMath::Max(0.f, MinCamZ - DesiredCam.Z),
		0.f,
		CameraTerrainLiftMax);

	// Drop lift faster than we add it so the boom does not stay stuck high after cresting a hill.
	const float LiftInterpSpeed = (NeededLift < CameraTerrainLiftCm)
		? CameraTerrainLiftSpeed * 1.8f
		: CameraTerrainLiftSpeed;
	CameraTerrainLiftCm = FMath::FInterpTo(
		CameraTerrainLiftCm, NeededLift, DeltaTime, LiftInterpSpeed);

	// Convert world-up lift into spring-arm local SocketOffset so attachment keeps it.
	const FVector LocalLift = ArmMatrix.InverseTransformVector(FVector(0.f, 0.f, CameraTerrainLiftCm));
	CameraBoom->SocketOffset = FVector(
		CameraBoom->SocketOffset.X,
		CameraBoom->SocketOffset.Y,
		0.f) + LocalLift;
}
