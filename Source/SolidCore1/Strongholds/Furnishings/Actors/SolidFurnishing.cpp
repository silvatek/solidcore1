#include "SolidFurnishing.h"

#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidFurnishing::ASolidFurnishing()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidFurnishing::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidFurnishing::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

int32 ASolidFurnishing::HexCount() const
{
	return 1;
}

void ASolidFurnishing::SetPlacement(const SolidStrongholdFurniture::FPlacedFurnishing& InPlacement)
{
	Placement = InPlacement;
	bHasPlacement = true;
}

void ASolidFurnishing::SetGhost(const bool bInGhost)
{
	if (bInGhost == bGhostLook)
	{
		return;
	}
	bGhostLook = bInGhost;
	BuildVisuals();
}

void ASolidFurnishing::BuildVisuals()
{
}

float ASolidFurnishing::FloorLiftCm() const
{
	return SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm;
}

FLinearColor ASolidFurnishing::BeigeColor() const
{
	return FLinearColor(0.78f, 0.66f, 0.45f);
}

FLinearColor ASolidFurnishing::Shown(const FLinearColor& Color) const
{
	if (!bGhostLook)
	{
		return Color;
	}
	return FMath::Lerp(Color, FLinearColor::White, 0.62f);
}

UStaticMeshComponent* ASolidFurnishing::MakePart(const TCHAR* Name)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetupAttachment(SceneRoot);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Part->SetCollisionResponseToAllChannels(ECR_Block);
	Part->SetCastShadow(true);
	return Part;
}

UStaticMesh* ASolidFurnishing::BasicCube() const
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFurnishing: missing Engine BasicShapes Cube."));
	}
	return Cube;
}

UStaticMesh* ASolidFurnishing::BasicCylinder() const
{
	return LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

void ASolidFurnishing::PlacePart(
	UStaticMeshComponent* Part,
	UStaticMesh* Mesh,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& SizeCm,
	const FVector& MeshSizeCm,
	const FLinearColor& Color,
	const TCHAR* DebugName)
{
	if (!Part || !Mesh)
	{
		return;
	}

	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(FVector(
		SizeCm.X / MeshSizeCm.X,
		SizeCm.Y / MeshSizeCm.Y,
		SizeCm.Z / MeshSizeCm.Z));
	Part->SetVisibility(true);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Shown(Color), DebugName, 0.85f))
	{
		Part->SetMaterial(0, Material);
	}
}
