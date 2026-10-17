#include "SolidStrongholdStone.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidStrongholdStone::ASolidStrongholdStone()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Block"));
	BlockMesh->SetupAttachment(SceneRoot);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ASolidStrongholdStone::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidStrongholdStone::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidStrongholdStone::SetSite(const FIntPoint InCell, const ESolidStrongholdSize InSize)
{
	Cell = InCell;
	Size = InSize;
}

void ASolidStrongholdStone::BuildVisuals()
{
	if (!BlockMesh)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidStrongholdStone: missing Engine BasicShapes Cube."));
		return;
	}

	constexpr float ShapeSize = 100.f;
	BlockMesh->SetStaticMesh(Cube);
	BlockMesh->SetRelativeLocation(FVector(0.f, 0.f, BlockCm * 0.5f));
	BlockMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BlockMesh->SetRelativeScale3D(FVector(BlockCm / ShapeSize));
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
		this, SolidStrongholds::RockColor(), TEXT("StrongholdStone"), 0.94f))
	{
		BlockMesh->SetMaterial(0, Material);
	}
}
