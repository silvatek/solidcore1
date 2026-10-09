#include "SolidCharacter.h"
#include "Camera/CameraComponent.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Terrain/SolidTerrainStreamer.h"

void ASolidCharacter::UpdatePartyCameraFraming(float DeltaTime)
{
	if (!CameraBoom || !bFrameCompanions)
	{
		return;
	}

	// Only the locally controlled Captain drives the Party framing camera.
	if (!IsLocallyControlled())
	{
		return;
	}

	struct FPartyMember
	{
		FVector Location;
		float CapsuleHalfHeight;
	};

	// Party ≈ Captain + Companions currently present (not the full Company).
	TArray<FPartyMember, TInlineAllocator<8>> PartyMembers;
	{
		float HalfHeight = 96.f;
		if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
		{
			HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
		}
		PartyMembers.Add({ GetActorLocation(), HalfHeight });
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ASolidCompanionCharacter> It(World); It; ++It)
		{
			if (!IsValid(*It))
			{
				continue;
			}
			float HalfHeight = 96.f;
			if (const UCapsuleComponent* Capsule = It->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
			PartyMembers.Add({ It->GetActorLocation(), HalfHeight });
		}
	}

	const bool bPartyHasCompanions = PartyMembers.Num() > 1;

	if (bDisableBoomCollisionWhileFraming)
	{
		// SC1-0025 popped narrower when the boom probe hit hills and collapsed arm length.
		CameraBoom->bDoCollisionTest = !bPartyHasCompanions;
	}

	FVector DesiredTargetOffset = FVector::ZeroVector;
	float FramingFitArm = 0.f;

	if (bPartyHasCompanions)
	{
		FVector Center = FVector::ZeroVector;
		for (const FPartyMember& Member : PartyMembers)
		{
			Center += Member.Location;
		}
		Center /= static_cast<float>(PartyMembers.Num());

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
		for (const FPartyMember& Member : PartyMembers)
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
	if (bPartyHasCompanions && bFramingCanOverrideZoom && FramingFitArm > DesiredArmLength)
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
	if (!CameraBoom || !IsLocallyControlled())
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
	const FVector DesiredCam =
		ArmOrigin
		- ArmRot.Vector() * CameraBoom->TargetArmLength
		+ ArmMatrix.TransformVector(FVector(CameraBoom->SocketOffset.X, CameraBoom->SocketOffset.Y, 0.f));

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
