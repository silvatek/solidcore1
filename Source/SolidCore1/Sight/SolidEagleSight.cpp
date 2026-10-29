#include "SolidCharacter.h"
#include "Camera/PlayerCameraManager.h"
#include "Content/SolidMaterials.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Math/RotationMatrix.h"
#include "Terrain/SolidBiome.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"

void ASolidCharacter::HideEagleSight()
{
	EagleSightHeightCm = 0.f;
	bEagleGroundBegun = false;
	bEagleGroundSettled = false;
	if (EagleSea)
	{
		EagleSea->SetVisibility(false);
	}
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
	if (!bEagleGroundBegun)
	{
		EagleKeptXY = Captain;
		EagleCommandXY = Captain;
		bEagleGroundBegun = true;
		bEagleGroundSettled = false;
	}

	if (!bEagleGroundSettled && BuiltMap && PC && PC->PlayerCameraManager)
	{
		const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		const FVector2D Camera(CamLoc.X, CamLoc.Y);
		if (FVector2D::Distance(Camera, EagleCommandXY) <= 100.f)
		{
			const float Fog = SolidSight::CameraGroundFog(*BuiltMap, Camera.X, Camera.Y);
			const SolidSight::FEagleCameraGround Move = SolidSight::EagleCameraGround(
				Camera, EagleKeptXY, Centre, 200.f, Fog);
			EagleKeptXY = Move.Kept;
			EagleCommandXY = Move.Next;
			bEagleGroundSettled = Move.bSettled;
		}
	}
	const FVector2D Ground = EagleCommandXY;
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
		FVector2D ClearMin = WorldMin;
		FVector2D ClearMax = WorldMax;
		const bool bClear = BuiltMap && BuiltMap->GetClearFogBounds(ClearMin, ClearMax);
		FVector TopOrigin;
		FVector TopDir;
		FVector BottomOrigin;
		FVector BottomDir;
		FVector LeftOrigin;
		FVector LeftDir;
		FVector RightOrigin;
		FVector RightDir;
		FVector TopHit;
		FVector BottomHit;
		FVector LeftHit;
		FVector RightHit;
		const bool bClearRays = bEagleView && bClear
			&& PC->DeprojectScreenPositionToWorld(SizeX * 0.5f, 0.f, TopOrigin, TopDir)
			&& PC->DeprojectScreenPositionToWorld(SizeX * 0.5f, static_cast<float>(SizeY), BottomOrigin, BottomDir)
			&& PC->DeprojectScreenPositionToWorld(0.f, SizeY * 0.5f, LeftOrigin, LeftDir)
			&& PC->DeprojectScreenPositionToWorld(static_cast<float>(SizeX), SizeY * 0.5f, RightOrigin, RightDir)
			&& SolidSight::EagleGroundHit(TopOrigin, TopDir, GroundZ, TopHit)
			&& SolidSight::EagleGroundHit(BottomOrigin, BottomDir, GroundZ, BottomHit)
			&& SolidSight::EagleGroundHit(LeftOrigin, LeftDir, GroundZ, LeftHit)
			&& SolidSight::EagleGroundHit(RightOrigin, RightDir, GroundZ, RightHit);
		FVector OriginA;
		FVector DirA;
		FVector OriginB;
		FVector DirB;
		const bool bMapRays = bEagleView && !bClear
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
		float Target = EagleSightHeightCm;
		bool bHaveTarget = false;
		if (bClearRays)
		{
			Target = SolidSight::EagleClearHeightCm(
				Cam.Z - GroundZ,
				FMath::Abs(RightHit.X - LeftHit.X),
				FMath::Abs(TopHit.Y - BottomHit.Y),
				FVector2D(Cam.X, Cam.Y),
				ClearMin,
				ClearMax);
			bHaveTarget = true;
		}
		else if (bMapRays
			&& SolidSight::EagleGroundHit(OriginA, DirA, GroundZ, HitA)
			&& SolidSight::EagleGroundHit(OriginB, DirB, GroundZ, HitB))
		{
			const float SeenA = bFillHeight ? HitA.Y : HitA.X;
			const float SeenB = bFillHeight ? HitB.Y : HitB.X;
			const float MapA = bFillHeight ? WorldMin.Y : WorldMin.X;
			const float MapB = bFillHeight ? WorldMax.Y : WorldMax.X;
			Target = SolidSight::EagleSpanHeightCm(
				Cam.Z - GroundZ, FMath::Abs(SeenB - SeenA), FMath::Abs(MapB - MapA));
			bHaveTarget = true;
		}
		if (bHaveTarget)
		{
			Target = FMath::Clamp(Target, 1000.f, SolidSight::EagleMaxHeightCm);
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
