#include "SolidTowerShell.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace SolidTowerShellPrivate
{
	constexpr float CylinderRadiusCm = 50.f;
	constexpr float CylinderHeightCm = 100.f;
	constexpr float CubeSizeCm = 100.f;
}

ASolidTowerShell::ASolidTowerShell()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASolidTowerShell::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTowerShell::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTowerShell::SetPlan(const SolidTowerStronghold::FPlan& InPlan)
{
	Plan = InPlan;
}

UStaticMeshComponent* ASolidTowerShell::GetCourse(const int32 Index) const
{
	if (!Courses.IsValidIndex(Index))
	{
		return nullptr;
	}
	return Courses[Index].Get();
}

void ASolidTowerShell::DiscardParts()
{
	auto Drop = [](TObjectPtr<UStaticMeshComponent>& Part)
	{
		if (UStaticMeshComponent* Mesh = Part.Get())
		{
			Mesh->DestroyComponent();
			Part = nullptr;
		}
	};

	for (TObjectPtr<UStaticMeshComponent>& Course : Courses)
	{
		Drop(Course);
	}
	Courses.Reset();
	Drop(CapMesh);
	Drop(DoorMesh);
}

UStaticMeshComponent* ASolidTowerShell::AddPart(
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

void ASolidTowerShell::BuildVisuals()
{
	DiscardParts();

	if (Plan.FloorCount <= 0)
	{
		return;
	}

	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cylinder || !Cube)
	{
		UE_LOG(LogSolid, Error,
			TEXT("SolidTowerShell: missing Engine BasicShapes (Cylinder=%s Cube=%s)."),
			Cylinder ? TEXT("ok") : TEXT("null"),
			Cube ? TEXT("ok") : TEXT("null"));
		return;
	}

	const FLinearColor Rock = SolidStrongholds::RockColor();
	Courses.Reserve(Plan.FloorCount);
	for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
	{
		const float BottomZ = SolidTowerStronghold::FloorZCm(Plan, Floor);
		const float Radius = SolidTowerStronghold::ExteriorRadiusCm(Plan, BottomZ);
		const bool bLast = Floor == Plan.FloorCount - 1;
		const float Height = bLast
			? FMath::Max(Plan.FloorPitchCm - Plan.SlabCm, 1.f)
			: Plan.FloorPitchCm;
		const FVector Scale(
			Radius / SolidTowerShellPrivate::CylinderRadiusCm,
			Radius / SolidTowerShellPrivate::CylinderRadiusCm,
			Height / SolidTowerShellPrivate::CylinderHeightCm);
		UStaticMeshComponent* Course = AddPart(
			Cylinder,
			FVector(0.f, 0.f, BottomZ + Height * 0.5f),
			Scale,
			Rock,
			TEXT("TowerCourse"));
		if (Course)
		{
			Courses.Add(Course);
		}
	}

	const float CapHeight = FMath::Max(Plan.SlabCm, 1.f);
	const float CapRadius = Plan.TopExteriorRadiusCm;
	CapMesh = AddPart(
		Cylinder,
		FVector(0.f, 0.f, Plan.HeightCm() - CapHeight * 0.5f),
		FVector(
			CapRadius / SolidTowerShellPrivate::CylinderRadiusCm,
			CapRadius / SolidTowerShellPrivate::CylinderRadiusCm,
			CapHeight / SolidTowerShellPrivate::CylinderHeightCm),
		Rock,
		TEXT("TowerCap"));

	if (SolidTowerStronghold::HasDoor(0) && Plan.DoorWidthCm > 0.f && Plan.DoorHeightCm > 0.f)
	{
		DoorMesh = AddPart(
			Cube,
			SolidTowerStronghold::DoorSlabCenterCm(Plan),
			FVector(
				SolidTowerStronghold::DoorSlabDepthCm / SolidTowerShellPrivate::CubeSizeCm,
				Plan.DoorWidthCm / SolidTowerShellPrivate::CubeSizeCm,
				Plan.DoorHeightCm / SolidTowerShellPrivate::CubeSizeCm),
			SolidStrongholds::MouthColor(),
			TEXT("TowerDoor"));
	}
}
