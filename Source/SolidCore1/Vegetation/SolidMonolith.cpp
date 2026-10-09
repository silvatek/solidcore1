#include "SolidMonolith.h"
#include "SolidCore1.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
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

UMaterialInterface* ASolidMonolith::MakeSolidColor(const FLinearColor& InColor, const TCHAR* DebugName) const
{
	UMaterialInterface* Parent = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"));
	if (!Parent)
	{
		Parent = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/LevelPrototyping/Materials/MI_DefaultColorway.MI_DefaultColorway"));
	}
	if (!Parent)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidMonolith %s: no FlatCol parent."), DebugName);
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(
		Parent, const_cast<ASolidMonolith*>(this));
	if (!MID)
	{
		return Parent;
	}

	MID->SetVectorParameterValue(TEXT("Base Color"), InColor);
	MID->SetVectorParameterValue(TEXT("BaseColor"), InColor);
	MID->SetScalarParameterValue(TEXT("Roughness"), 0.92f);
	return MID;
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

	if (UMaterialInterface* Mat = MakeSolidColor(Color, TEXT("Slab")))
	{
		SlabMesh->SetMaterial(0, Mat);
	}
}
