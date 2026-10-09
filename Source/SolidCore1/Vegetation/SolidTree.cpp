#include "SolidTree.h"
#include "SolidCore1.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ASolidTree::ASolidTree()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TrunkMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrunkMesh"));
	TrunkMesh->SetupAttachment(SceneRoot);
	TrunkMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	TrunkMesh->SetCollisionResponseToAllChannels(ECR_Block);
	TrunkMesh->SetCastShadow(true);

	CanopyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CanopyMesh"));
	CanopyMesh->SetupAttachment(SceneRoot);
	CanopyMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CanopyMesh->SetCollisionResponseToAllChannels(ECR_Block);
	CanopyMesh->SetCastShadow(true);
}

void ASolidTree::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidTree::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidTree::ApplyRandomVariation(FRandomStream& Rng)
{
	const float Overall = Rng.FRandRange(0.62f, 1.55f);
	const float TrunkFat = Rng.FRandRange(0.70f, 1.45f);
	const float TrunkTall = Rng.FRandRange(0.75f, 1.40f);
	const float CanopyWide = Rng.FRandRange(0.65f, 1.50f);
	const float CanopyTall = Rng.FRandRange(0.70f, 1.40f);

	TrunkHeightCm = 280.f * Overall * TrunkTall;
	TrunkRadiusCm = FMath::Max(8.f, 28.f * Overall * TrunkFat);
	CanopyHeightCm = 320.f * Overall * CanopyTall;
	CanopyRadiusCm = FMath::Max(40.f, 180.f * Overall * CanopyWide);

	const float TrunkTint = Rng.FRandRange(-0.04f, 0.05f);
	TrunkColor = FLinearColor(
		FMath::Clamp(0.28f + TrunkTint, 0.12f, 0.40f),
		FMath::Clamp(0.16f + TrunkTint * 0.5f, 0.06f, 0.24f),
		FMath::Clamp(0.07f + TrunkTint * 0.25f, 0.03f, 0.14f));

	const float LeafTint = Rng.FRandRange(-0.05f, 0.08f);
	CanopyColor = FLinearColor(
		FMath::Clamp(0.10f + LeafTint * 0.3f, 0.04f, 0.18f),
		FMath::Clamp(0.32f + LeafTint, 0.16f, 0.48f),
		FMath::Clamp(0.08f + LeafTint * 0.2f, 0.03f, 0.16f));
}

UMaterialInterface* ASolidTree::MakeSolidColor(const FLinearColor& Color, const TCHAR* DebugName) const
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
		UE_LOG(LogSolid, Error, TEXT("SolidTree %s: no FlatCol parent."), DebugName);
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(
		Parent, const_cast<ASolidTree*>(this));
	if (!MID)
	{
		return Parent;
	}

	MID->SetVectorParameterValue(TEXT("Base Color"), Color);
	MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
	MID->SetScalarParameterValue(TEXT("Roughness"), 0.95f);
	return MID;
}

void ASolidTree::BuildVisuals()
{
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* Cone = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));

	if (!Cylinder || !Cone)
	{
		UE_LOG(LogSolid, Error,
			TEXT("SolidTree: missing Engine BasicShapes (Cylinder=%s Cone=%s)."),
			Cylinder ? TEXT("ok") : TEXT("null"),
			Cone ? TEXT("ok") : TEXT("null"));
		return;
	}

	// Engine cylinder/cone are ~100cm tall with ~50cm radius, pivot at center.
	constexpr float ShapeHalfWidth = 50.f;
	constexpr float ShapeHeight = 100.f;

	TrunkMesh->SetStaticMesh(Cylinder);
	TrunkMesh->SetRelativeLocation(FVector(0.f, 0.f, TrunkHeightCm * 0.5f));
	TrunkMesh->SetRelativeScale3D(FVector(
		TrunkRadiusCm / ShapeHalfWidth,
		TrunkRadiusCm / ShapeHalfWidth,
		TrunkHeightCm / ShapeHeight));
	if (UMaterialInterface* TrunkMat = MakeSolidColor(TrunkColor, TEXT("Trunk")))
	{
		TrunkMesh->SetMaterial(0, TrunkMat);
	}

	CanopyMesh->SetStaticMesh(Cone);
	CanopyMesh->SetRelativeLocation(FVector(0.f, 0.f, TrunkHeightCm + CanopyHeightCm * 0.5f));
	CanopyMesh->SetRelativeScale3D(FVector(
		CanopyRadiusCm / ShapeHalfWidth,
		CanopyRadiusCm / ShapeHalfWidth,
		CanopyHeightCm / ShapeHeight));
	if (UMaterialInterface* CanopyMat = MakeSolidColor(CanopyColor, TEXT("Canopy")))
	{
		CanopyMesh->SetMaterial(0, CanopyMat);
	}
}
