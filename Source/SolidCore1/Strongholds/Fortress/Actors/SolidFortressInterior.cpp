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
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};
	Drop(GateMesh);
	Drop(FloorMesh);
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
	const float Opening = FSolidStronghold::OpeningCm;
	const float Storey = FSolidStronghold::StoreyCm;
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
		const float PlanSizeY,
		const float CenterZ,
		const float SizeZ)
	{
		if (UStaticMeshComponent* Part = AddPart(
			Cube,
			SolidFortressInteriorPrivate::ActorCenter(Width, Depth, PlanCenterX, PlanCenterY, CenterZ),
			FVector(
				PlanSizeY / SolidFortressInteriorPrivate::CubeSizeCm,
				PlanSizeX / SolidFortressInteriorPrivate::CubeSizeCm,
				SizeZ / SolidFortressInteriorPrivate::CubeSizeCm),
			Rock,
			TEXT("FortressInteriorCurtain")))
		{
			Curtain.Add(Part);
		}
	};

	const float GateMinX = (Width - Opening) * 0.5f;
	const float GateMaxX = GateMinX + Opening;
	const float NorthY = Depth - Wall * 0.5f;
	AddWall(Width * 0.5f, Wall * 0.5f, Width, Wall, Height * 0.5f, Height);
	AddWall(Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall, Height * 0.5f, Height);
	AddWall(Width - Wall * 0.5f, Depth * 0.5f, Wall, Depth - 2.f * Wall, Height * 0.5f, Height);
	AddWall(GateMinX * 0.5f, NorthY, GateMinX, Wall, Height * 0.5f, Height);
	AddWall((GateMaxX + Width) * 0.5f, NorthY, Width - GateMaxX, Wall, Height * 0.5f, Height);
	AddWall(Width * 0.5f, NorthY, Opening, Wall, (Storey + Height) * 0.5f, Height - Storey);
	GateMesh = AddPart(
		Cube,
		SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Width * 0.5f, NorthY, Storey * 0.5f),
		FVector(
			Wall / SolidFortressInteriorPrivate::CubeSizeCm,
			Opening / SolidFortressInteriorPrivate::CubeSizeCm,
			Storey / SolidFortressInteriorPrivate::CubeSizeCm),
		SolidStrongholds::MouthColor(),
		TEXT("FortressInteriorGate"));
	const float FloorThickness = 20.f;
	FloorMesh = AddPart(
		Cube,
		SolidFortressInteriorPrivate::ActorCenter(
			Width,
			Depth,
			Width * 0.5f,
			Depth * 0.5f,
			-FloorThickness * 0.5f),
		FVector(
			(Depth - 2.f * Wall) / SolidFortressInteriorPrivate::CubeSizeCm,
			(Width - 2.f * Wall) / SolidFortressInteriorPrivate::CubeSizeCm,
			FloorThickness / SolidFortressInteriorPrivate::CubeSizeCm),
		Rock,
		TEXT("FortressInteriorFloor"));
}

FVector ASolidFortressInterior::GetCaptainSpawnWorld() const
{
	if (!Fortress)
	{
		return FVector::ZeroVector;
	}
	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float InsideY = Depth - FSolidStronghold::WallCm - SolidStrongholdGrid::FlatToFlatCm;
	return SolidFortressInteriorPrivate::ActorCenter(Width, Depth, Width * 0.5f, InsideY, 0.f);
}

FRotator ASolidFortressInterior::GetCaptainView() const
{
	return FRotator::ZeroRotator;
}
