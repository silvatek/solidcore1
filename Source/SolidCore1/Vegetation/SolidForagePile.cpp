#include "SolidForagePile.h"
#include "SolidCore1.h"
#include "SolidForage.h"
#include "SolidMaterials.h"
#include "Terrain/SolidTerrainSeat.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

ASolidForagePile::ASolidForagePile()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PileMesh"));
	PileMesh->SetupAttachment(SceneRoot);
	PileMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PileMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	PileMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PileMesh->SetGenerateOverlapEvents(false);
	PileMesh->SetCastShadow(false);
}

void ASolidForagePile::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidForagePile::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidForagePile::SetChoice(const int32 InChoiceIndex, const TCHAR* InItemId)
{
	ChoiceIndex = InChoiceIndex;
	ItemId = InItemId;
}

void ASolidForagePile::BuildVisuals()
{
	if (!PileMesh)
	{
		return;
	}

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, SolidForage::MeshPath(ItemId));
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidForagePile: missing %s."), SolidForage::CubeMesh);
		return;
	}

	// Engine cube is 100 cm on a side, pivot at the center.
	const float Scale = SolidForage::CubeCm / 100.f;
	PileMesh->SetStaticMesh(Mesh);
	PileMesh->SetRelativeScale3D(FVector(Scale));
	PileMesh->SetRelativeLocation(FVector(0.f, 0.f, SolidForage::CubeCm * 0.5f));
	SolidTerrainSeat::FFootprint Seat;
	Seat.HalfXCm = SolidForage::CubeCm * 0.5f;
	Seat.HalfYCm = SolidForage::CubeCm * 0.5f;
	const float Drop = SolidTerrainSeat::DropCm(this, Seat);
	SolidTerrainSeat::BuryCentered(PileMesh, 0.f, SolidForage::CubeCm, Drop, 100.f);
	if (UMaterialInterface* Mat = SolidMaterials::CreateSolidColor(this, PileColor, TEXT("ForagePile"), 1.f))
	{
		PileMesh->SetMaterial(0, Mat);
	}
}
