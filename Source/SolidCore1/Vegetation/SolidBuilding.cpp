#include "SolidBuilding.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidBuilding::ASolidBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	BodyMesh->SetCastShadow(true);

	RoofMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RoofMesh"));
	RoofMesh->SetupAttachment(SceneRoot);
	RoofMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RoofMesh->SetCollisionResponseToAllChannels(ECR_Block);
	RoofMesh->SetCastShadow(true);
}

void ASolidBuilding::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidBuilding::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidBuilding::ApplyRandomVariation(FRandomStream& Rng)
{
	FootprintXCm = Rng.FRandRange(180.f, 420.f);
	FootprintYCm = Rng.FRandRange(160.f, 380.f);
	BodyHeightCm = Rng.FRandRange(220.f, 520.f);
	RoofHeightCm = Rng.FRandRange(90.f, 220.f);

	const float Grey = Rng.FRandRange(-0.04f, 0.05f);
	BodyColor = FLinearColor(
		FMath::Clamp(0.40f + Grey, 0.28f, 0.52f),
		FMath::Clamp(0.40f + Grey, 0.28f, 0.52f),
		FMath::Clamp(0.42f + Grey, 0.30f, 0.55f));

	const float RedTint = Rng.FRandRange(-0.06f, 0.08f);
	RoofColor = FLinearColor(
		FMath::Clamp(0.55f + RedTint, 0.40f, 0.70f),
		FMath::Clamp(0.10f + RedTint * 0.2f, 0.04f, 0.18f),
		FMath::Clamp(0.08f + RedTint * 0.1f, 0.03f, 0.14f));
}

void ASolidBuilding::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (!Cube || !Cone)
	{
		UE_LOG(LogSolid, Error,
			TEXT("SolidBuilding: missing Engine BasicShapes (Cube=%s Cone=%s)."),
			Cube ? TEXT("ok") : TEXT("null"),
			Cone ? TEXT("ok") : TEXT("null"));
		return;
	}

	// Engine cube/cone are ~100cm tall with ~50cm half-width, pivot at center.
	constexpr float ShapeSize = 100.f;
	constexpr float ShapeHalfWidth = 50.f;

	BodyMesh->SetStaticMesh(Cube);
	BodyMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyHeightCm * 0.5f));
	BodyMesh->SetRelativeScale3D(FVector(
		FootprintXCm / ShapeSize,
		FootprintYCm / ShapeSize,
		BodyHeightCm / ShapeSize));
	if (UMaterialInterface* BodyMat =
			SolidMaterials::CreateSolidColor(this, BodyColor, TEXT("BuildingBody"), 0.92f))
	{
		BodyMesh->SetMaterial(0, BodyMat);
	}

	// Cone as a simple prismatic/pyramidal roof sitting on the body.
	RoofMesh->SetStaticMesh(Cone);
	RoofMesh->SetRelativeLocation(FVector(0.f, 0.f, BodyHeightCm + RoofHeightCm * 0.5f));
	const float RoofRadius = 0.5f * FMath::Max(FootprintXCm, FootprintYCm) * 1.08f;
	RoofMesh->SetRelativeScale3D(FVector(
		RoofRadius / ShapeHalfWidth,
		RoofRadius / ShapeHalfWidth,
		RoofHeightCm / ShapeSize));
	if (UMaterialInterface* RoofMat =
			SolidMaterials::CreateSolidColor(this, RoofColor, TEXT("BuildingRoof"), 0.90f))
	{
		RoofMesh->SetMaterial(0, RoofMat);
	}
}
