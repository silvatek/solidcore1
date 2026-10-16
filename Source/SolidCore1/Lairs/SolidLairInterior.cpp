#include "SolidLairInterior.h"
#include "SolidCore1.h"
#include "SolidLair.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidLairInteriorPrivate
{
	void PlaceMouth(
		UStaticMeshComponent* Mouth,
		UStaticMesh* Cube,
		AActor* Outer,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& SizeCm,
		const TCHAR* DebugName)
	{
		if (!Mouth || !Cube)
		{
			return;
		}

		constexpr float ShapeSize = 100.f;
		Mouth->SetStaticMesh(Cube);
		Mouth->SetRelativeLocation(Location);
		Mouth->SetRelativeRotation(Rotation);
		Mouth->SetRelativeScale3D(SizeCm / ShapeSize);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			Outer, SolidStrongholds::MouthColor(), DebugName, 0.95f))
		{
			Mouth->SetMaterial(0, Material);
		}
		Mouth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mouth->SetCollisionResponseToAllChannels(ECR_Block);
		Mouth->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	void AddRock(
		UInstancedStaticMeshComponent* Walls,
		const FVector2D& A,
		const FVector2D& B,
		const FVector2D& Outward,
		const float Z0,
		const float Z1,
		const float Shell,
		const float ShapeSize)
	{
		if (!Walls || Z1 <= Z0)
		{
			return;
		}
		const FVector2D Delta = B - A;
		const float Length = Delta.Size();
		if (Length < 1.f)
		{
			return;
		}
		const FVector2D Mid = (A + B) * 0.5f;
		const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		const float Overlap = 8.f;
		const float Span = Z1 - Z0;
		const FTransform Wall(
			FRotator(0.f, Yaw, 0.f),
			FVector(Mid.X, Mid.Y, Z0 + Span * 0.5f) + FVector(Outward.X, Outward.Y, 0.f) * (Shell * 0.5f),
			FVector((Length + Overlap) / ShapeSize, Shell / ShapeSize, Span / ShapeSize));
		Walls->AddInstance(Wall);
	}
}

ASolidLairInterior::ASolidLairInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorTiles"));
	CeilingTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("CeilingTiles"));
	WallShell = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallShell"));
	MouthX = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthX"));
	MouthY = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthY"));
	LightNearX = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightNearX"));
	LightMid = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightMid"));
	LightNearY = CreateDefaultSubobject<UPointLightComponent>(TEXT("LightNearY"));

	UInstancedStaticMeshComponent* Shells[] = { FloorTiles.Get(), CeilingTiles.Get(), WallShell.Get() };
	for (UInstancedStaticMeshComponent* Shell : Shells)
	{
		Shell->SetupAttachment(SceneRoot);
		Shell->SetMobility(EComponentMobility::Movable);
		Shell->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Shell->SetCollisionResponseToAllChannels(ECR_Block);
		Shell->SetCastShadow(true);
	}

	MouthX->SetupAttachment(SceneRoot);
	MouthY->SetupAttachment(SceneRoot);
	UStaticMeshComponent* Mouths[] = { MouthX.Get(), MouthY.Get() };
	for (UStaticMeshComponent* Mouth : Mouths)
	{
		Mouth->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mouth->SetCollisionResponseToAllChannels(ECR_Block);
		Mouth->SetCastShadow(true);
	}

	UPointLightComponent* Lights[] = { LightNearX.Get(), LightMid.Get(), LightNearY.Get() };
	for (UPointLightComponent* Light : Lights)
	{
		Light->SetupAttachment(SceneRoot);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(true);
		Light->SetIntensity(4500.f);
		Light->SetAttenuationRadius(900.f);
		Light->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
	}
}

void ASolidLairInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidLairInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

FVector ASolidLairInterior::GetCaptainSpawnWorld(const TCHAR End) const
{
	return GetActorTransform().TransformPosition(SolidLair::SpawnLocal(End));
}

FRotator ASolidLairInterior::GetCaptainView(const TCHAR End) const
{
	const FRotator Local(0.f, SolidLair::SpawnYawDeg(End), 0.f);
	return (GetActorQuat() * Local.Quaternion()).Rotator();
}

UStaticMeshComponent* ASolidLairInterior::GetEndMouth(const TCHAR End) const
{
	if (FChar::ToUpper(End) == USolidWorldMap::XMarker)
	{
		return MouthX;
	}
	if (FChar::ToUpper(End) == USolidWorldMap::YMarker)
	{
		return MouthY;
	}
	return nullptr;
}

void ASolidLairInterior::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidLairInterior: missing Engine BasicShapes Cube."));
		return;
	}

	const float Height = SolidLair::RoomHeightCm;
	const float Shell = SolidLair::ShellCm;
	const float ShapeSize = 100.f;
	const FLinearColor Rock = SolidStrongholds::RockColor();

	auto PaintShell = [this, Cube, Rock](UInstancedStaticMeshComponent* Shell, const TCHAR* Name)
	{
		if (!Shell)
		{
			return;
		}
		Shell->SetStaticMesh(Cube);
		Shell->ClearInstances();
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Rock, Name, 0.95f))
		{
			Shell->SetMaterial(0, Material);
		}
	};
	PaintShell(FloorTiles, TEXT("TunnelFloor"));
	PaintShell(CeilingTiles, TEXT("TunnelCeiling"));
	PaintShell(WallShell, TEXT("TunnelWall"));

	const FIntPoint Steps[] = {
		FIntPoint(-1, 0), FIntPoint(1, 0), FIntPoint(0, -1), FIntPoint(0, 1),
	};

	for (int32 Row = 0; Row < SolidLair::MazeRows; ++Row)
	{
		for (int32 Col = 0; Col < SolidLair::MazeCols; ++Col)
		{
			if (!SolidLair::IsOpenCell(Col, Row))
			{
				continue;
			}

			const FVector2D Center = SolidLair::CellCenter(Col, Row);
			const FTransform Floor(
				FRotator::ZeroRotator,
				FVector(Center.X, Center.Y, -Shell * 0.5f),
				FVector(SolidLair::CellCm / ShapeSize, SolidLair::CellCm / ShapeSize, Shell / ShapeSize));
			const FTransform Ceiling(
				FRotator::ZeroRotator,
				FVector(Center.X, Center.Y, Height + Shell * 0.5f),
				Floor.GetScale3D());
			if (FloorTiles)
			{
				FloorTiles->AddInstance(Floor);
			}
			if (CeilingTiles)
			{
				CeilingTiles->AddInstance(Ceiling);
			}

			const float X0 = static_cast<float>(Col) * SolidLair::CellCm;
			const float Y0 = static_cast<float>(Row) * SolidLair::CellCm;
			const float X1 = X0 + SolidLair::CellCm;
			const float Y1 = Y0 + SolidLair::CellCm;
			const FVector2D Corners[4][2] = {
				{ FVector2D(X0, Y0), FVector2D(X0, Y1) },
				{ FVector2D(X1, Y0), FVector2D(X1, Y1) },
				{ FVector2D(X0, Y0), FVector2D(X1, Y0) },
				{ FVector2D(X0, Y1), FVector2D(X1, Y1) },
			};
			const FVector2D Outward[4] = {
				FVector2D(-1.f, 0.f), FVector2D(1.f, 0.f), FVector2D(0.f, -1.f), FVector2D(0.f, 1.f),
			};

			const TCHAR Marker = SolidLair::MazeAt(Col, Row);
			for (int32 Side = 0; Side < 4; ++Side)
			{
				if (SolidLair::IsOpenCell(Col + Steps[Side].X, Row + Steps[Side].Y))
				{
					continue;
				}

				const bool bWestDoor = Marker == USolidWorldMap::XMarker && Side == 0;
				const bool bEastDoor = Marker == USolidWorldMap::YMarker && Side == 1;
				const FVector2D A = Corners[Side][0];
				const FVector2D B = Corners[Side][1];
				const FVector2D Delta = B - A;
				const float Length = Delta.Size();
				const FVector2D Dir = Length > 1.f ? Delta / Length : FVector2D(0.f, 1.f);
				if (bWestDoor || bEastDoor)
				{
					const float Door = FMath::Min(SolidLair::DoorwayWidthCm, Length);
					const float Stub = (Length - Door) * 0.5f;
					const FVector2D DoorA = A + Dir * Stub;
					const FVector2D DoorB = B - Dir * Stub;
					if (Stub > 1.f)
					{
						SolidLairInteriorPrivate::AddRock(WallShell, A, DoorA, Outward[Side], 0.f, Height, Shell, ShapeSize);
						SolidLairInteriorPrivate::AddRock(WallShell, DoorB, B, Outward[Side], 0.f, Height, Shell, ShapeSize);
					}
					SolidLairInteriorPrivate::AddRock(WallShell, DoorA, DoorB, Outward[Side], SolidLair::MouthHeightCm, Height, Shell, ShapeSize);
					const FVector2D Mid = (DoorA + DoorB) * 0.5f;
					const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
					const float Lip = 16.f;
					const FVector MouthCenter = FVector(Mid.X, Mid.Y, SolidLair::MouthHeightCm * 0.5f)
						+ FVector(Outward[Side].X, Outward[Side].Y, 0.f) * (Lip * 0.5f);
					SolidLairInteriorPrivate::PlaceMouth(
						bWestDoor ? MouthX.Get() : MouthY.Get(),
						Cube,
						this,
						MouthCenter,
						FRotator(0.f, Yaw, 0.f),
						FVector(Door, Lip, SolidLair::MouthHeightCm),
						bWestDoor ? TEXT("TunnelMouthX") : TEXT("TunnelMouthY"));
					continue;
				}

				SolidLairInteriorPrivate::AddRock(WallShell, A, B, Outward[Side], 0.f, Height, Shell, ShapeSize);
			}
		}
	}

	auto PlaceLight = [](UPointLightComponent* Light, const TCHAR EndOrMid)
	{
		if (!Light)
		{
			return;
		}
		FVector2D Plan = FVector2D::ZeroVector;
		if (EndOrMid == USolidWorldMap::XMarker || EndOrMid == USolidWorldMap::YMarker)
		{
			const FIntPoint Cell = SolidLair::CellOf(EndOrMid);
			Plan = SolidLair::CellCenter(Cell.X, Cell.Y);
		}
		else
		{
			Plan = SolidLair::CellCenter(5, 5);
		}
		Light->SetRelativeLocation(FVector(Plan.X, Plan.Y, SolidLair::RoomHeightCm * 0.62f));
	};
	PlaceLight(LightNearX, USolidWorldMap::XMarker);
	PlaceLight(LightNearY, USolidWorldMap::YMarker);
	PlaceLight(LightMid, TEXT('M'));
}
