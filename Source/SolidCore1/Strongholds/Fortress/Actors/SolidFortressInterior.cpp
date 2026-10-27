#include "SolidFortressInterior.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Strongholds/SolidStronghold.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidFortressInteriorPrivate
{
	constexpr float CubeSizeCm = 100.f;

	/** Plan +X is east and plan +Y is north. Actor -X is that north, the gate side. */
	FVector ActorCenter(const float Width, const float Depth, const float PlanX, const float PlanY, const float Z)
	{
		return FVector(Depth * 0.5f - PlanY, PlanX - Width * 0.5f, Z);
	}
}

ASolidFortressInterior::ASolidFortressInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidFortressInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidFortressInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidFortressInterior::DiscardParts()
{
	for (TObjectPtr<UStaticMeshComponent>& Part : Curtain)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
		}
	}
	Curtain.Reset();
}

UStaticMeshComponent* ASolidFortressInterior::AddPart(
	UStaticMesh* Mesh,
	const FVector& Location,
	const FVector& Scale,
	const FLinearColor& Color,
	const TCHAR* DebugName)
{
	if (!Mesh || !SceneRoot)
	{
		return nullptr;
	}

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	if (!Part)
	{
		return nullptr;
	}

	Part->SetupAttachment(SceneRoot);
	Part->SetStaticMesh(Mesh);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(FRotator::ZeroRotator);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Part->SetCollisionResponseToAllChannels(ECR_Block);
	Part->SetCastShadow(true);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(this, Color, DebugName, 0.94f))
	{
		Part->SetMaterial(0, Material);
	}
	if (GetWorld())
	{
		Part->RegisterComponent();
	}
	return Part;
}

void ASolidFortressInterior::BuildVisuals()
{
	DiscardParts();
	if (!Fortress)
	{
		return;
	}

	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float Height = Fortress->CurtainHeightCm();
	const float Wall = FSolidStronghold::WallCm;
	if (Width <= Wall || Depth <= Wall || Height <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidFortressInterior: missing Engine BasicShapes Cube."));
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();

	auto AddWall = [&](
		const float PlanCenterX,
		const float PlanCenterY,
		const float PlanSizeX,
		const float PlanSizeY)
	{
		if (UStaticMeshComponent* Part = AddPart(
			Cube,
			SolidFortressInteriorPrivate::ActorCenter(Width, Depth, PlanCenterX, PlanCenterY, Height * 0.5f),
			FVector(
				PlanSizeY / SolidFortressInteriorPrivate::CubeSizeCm,
				PlanSizeX / SolidFortressInteriorPrivate::CubeSizeCm,
				Height / SolidFortressInteriorPrivate::CubeSizeCm),
			Rock,
			TEXT("FortressInteriorCurtain")))
		{
			Curtain.Add(Part);
		}
	};

	AddWall(Width * 0.5f, Wall * 0.5f, Width, Wall);
	AddWall(Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall);
	AddWall(Width - Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall);
	AddWall(Width * 0.5f, Depth - Wall * 0.5f, Width, Wall);
}
