#include "SolidMonolith.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidMonolith::ASolidMonolith()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SlabMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlabMesh"));
	SlabMesh->SetupAttachment(SceneRoot);
	SlabMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SlabMesh->SetCollisionResponseToAllChannels(ECR_Block);
	SlabMesh->SetCastShadow(true);
}

void ASolidMonolith::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidMonolith::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidMonolith::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidMonolith: missing /Engine/BasicShapes/Cube."));
		return;
	}

	// Engine cube is 100cm on each side, pivot at center.
	constexpr float ShapeSize = 100.f;

	SlabMesh->SetStaticMesh(Cube);
	SlabMesh->SetRelativeLocation(FVector(0.f, 0.f, HeightCm * 0.5f));
	SlabMesh->SetRelativeScale3D(FVector(
		WidthCm / ShapeSize,
		ThicknessCm / ShapeSize,
		HeightCm / ShapeSize));

	if (UMaterialInterface* Mat =
			SolidMaterials::CreateSolidColor(this, Color, TEXT("MonolithSlab"), 0.92f))
	{
		SlabMesh->SetMaterial(0, Mat);
	}
}
