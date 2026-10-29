#include "SolidCharacter.h"
#include "SolidCameraFog.h"
#include "Camera/CameraComponent.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Content/SolidMaterials.h"
#include "Engine/StaticMesh.h"
#include "Terrain/SolidBiome.h"
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

void ASolidCharacter::ApplyEagleSightCamera(float DeltaSeconds)
{
	if (!CameraBoom)
	{
		return;
	}

	const FRotator Look = SolidSight::EagleLook();
	if (Controller)
	{
		Controller->SetControlRotation(Look);
	}

	FVector2D WorldMin(-51200.f, -51200.f);
	FVector2D WorldMax(51200.f, 51200.f);
	ASolidTerrainStreamer* Streamer = nullptr;
	const USolidTerrainMap* BuiltMap = nullptr;
	if (UWorld* World = GetWorld())
	{
		Streamer = ASolidTerrainStreamer::FindExisting(World);
		if (Streamer)
		{
			if (const USolidTerrainMap* Map = Streamer->GetTerrainMap())
			{
				if (Map->IsBuilt())
				{
					BuiltMap = Map;
					WorldMin = Map->GetWorldMinXY();
					WorldMax = Map->GetWorldMaxXY();
				}
			}
		}
	}

	float ViewportW = 16.f;
	float ViewportH = 9.f;
	int32 SizeX = 0;
	int32 SizeY = 0;
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (PC)
	{
		PC->GetViewportSize(SizeX, SizeY);
		if (SizeX > 0 && SizeY > 0)
		{
			ViewportW = static_cast<float>(SizeX);
			ViewportH = static_cast<float>(SizeY);
		}
	}
	const FVector2D Centre = SolidSight::WorldCentre(WorldMin, WorldMax);
	const FVector2D Captain(GetActorLocation().X, GetActorLocation().Y);
	const FVector2D Ground = SolidSight::EagleGroundXY(
		Captain,
		Centre,
		200.f,
		[BuiltMap](const FVector2D& At)
		{
			return BuiltMap ? BuiltMap->SamplePoint(At.X, At.Y).Fog : 0.f;
		});
	float GroundZ = 0.f;
	if (Streamer)
	{
		GroundZ = Streamer->GetHeightAt(FVector(Ground.X, Ground.Y, 0.f)) + Streamer->CollisionHeightBias;
	}

	if (EagleSightHeightCm <= 1.f)
	{
		float StartHeight = 200.f;
		if (PC && PC->PlayerCameraManager)
		{
			const float CamHeight = PC->PlayerCameraManager->GetCameraLocation().Z - GroundZ;
			if (CamHeight > StartHeight)
			{
				StartHeight = CamHeight;
			}
		}
		EagleSightHeightCm = StartHeight;
	}

	if (PC && PC->PlayerCameraManager && SizeX > 1 && SizeY > 1)
	{
		const FVector Cam = PC->PlayerCameraManager->GetCameraLocation();
		const FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();
		const bool bEagleView = FMath::Abs(CamRot.Pitch + 90.f) < 2.f
			&& FMath::IsNearlyEqual(Cam.X, Ground.X, 500.f)
			&& FMath::IsNearlyEqual(Cam.Y, Ground.Y, 500.f);
		const bool bFillHeight = ViewportW >= ViewportH;
		FVector OriginA;
		FVector DirA;
		FVector OriginB;
		FVector DirB;
		const bool bRays = bEagleView
			&& PC->DeprojectScreenPositionToWorld(
				bFillHeight ? SizeX * 0.5f : 0.f,
				bFillHeight ? 0.f : SizeY * 0.5f,
				OriginA,
				DirA)
			&& PC->DeprojectScreenPositionToWorld(
				bFillHeight ? SizeX * 0.5f : static_cast<float>(SizeX),
				bFillHeight ? static_cast<float>(SizeY) : SizeY * 0.5f,
				OriginB,
				DirB);
		FVector HitA;
		FVector HitB;
		if (bRays
			&& SolidSight::EagleGroundHit(OriginA, DirA, GroundZ, HitA)
			&& SolidSight::EagleGroundHit(OriginB, DirB, GroundZ, HitB))
		{
			const float SeenA = bFillHeight ? HitA.Y : HitA.X;
			const float SeenB = bFillHeight ? HitB.Y : HitB.X;
			const float MapA = bFillHeight ? WorldMin.Y : WorldMin.X;
			const float MapB = bFillHeight ? WorldMax.Y : WorldMax.X;
			const float Target = FMath::Clamp(
				SolidSight::EagleSpanHeightCm(Cam.Z - GroundZ, FMath::Abs(SeenB - SeenA), FMath::Abs(MapB - MapA)),
				1000.f,
				500000.f);
			EagleSightHeightCm = SolidSight::EagleRiseCm(EagleSightHeightCm, Target, DeltaSeconds);
		}
	}

	const FVector Origin = CameraBoom->GetComponentLocation();
	const FVector WorldOffset(
		Ground.X - Origin.X, Ground.Y - Origin.Y, GroundZ + EagleSightHeightCm - Origin.Z);

	CameraBoom->TargetArmLength = 0.f;
	CameraBoom->TargetOffset = FVector::ZeroVector;
	CameraBoom->SocketOffset = FRotationMatrix(Look).InverseTransformVector(WorldOffset);
	CameraBoom->bEnableCameraLag = false;
	CameraBoom->bDoCollisionTest = false;
	CameraTerrainLiftCm = 0.f;
	ShowEagleSea(WorldMin, WorldMax);
}

void ASolidCharacter::ShowEagleSea(const FVector2D& WorldMin, const FVector2D& WorldMax)
{
	if (!EagleSea)
	{
		return;
	}

	if (!EagleSea->GetStaticMesh())
	{
		if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			EagleSea->SetStaticMesh(Plane);
		}
		const FLinearColor Sea = SolidBiome::Of(ESolidBiome::Sea).Color();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Sea, TEXT("EagleSea")))
		{
			EagleSea->SetMaterial(0, Material);
		}
	}

	const SolidSight::FEagleSea Sheet = SolidSight::EagleSeaSheet(WorldMin, WorldMax);
	EagleSea->SetWorldLocation(Sheet.Location);
	EagleSea->SetWorldRotation(FRotator::ZeroRotator);
	EagleSea->SetWorldScale3D(FVector(Sheet.Scale, Sheet.Scale, 1.f));
	EagleSea->SetVisibility(true);
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

void ASolidCharacter::UpdateSquadCameraFraming(float DeltaTime)
{
	if (!IsEagleSight())
	{
		EagleSightHeightCm = 0.f;
		if (EagleSea)
		{
			EagleSea->SetVisibility(false);
		}
	}

	if (IsTrueSight())
	{
		ApplyTrueSightCamera();
		return;
	}

	if (IsFalconSight())
	{
		AimFalconAtCaptain();
		return;
	}

	if (IsEagleSight())
	{
		ApplyEagleSightCamera(DeltaTime);
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
	if (IsTrueSight() || IsFalconSight() || IsEagleSight() || !CameraBoom || !IsLocallyControlled())
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
