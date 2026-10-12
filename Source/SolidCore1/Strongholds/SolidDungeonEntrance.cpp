#include "SolidDungeonEntrance.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	void PlacePart(
		UStaticMeshComponent* Part,
		UStaticMesh* Cube,
		AActor* Outer,
		const FVector& Location,
		const FVector& SizeCm,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Part || !Cube)
		{
			return;
		}

		constexpr float ShapeSize = 100.f;
		Part->SetStaticMesh(Cube);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(FRotator::ZeroRotator);
		Part->SetRelativeScale3D(SizeCm / ShapeSize);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.94f))
		{
			Part->SetMaterial(0, Material);
		}
	}
}

ASolidDungeonEntrance::ASolidDungeonEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	MouthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthMesh"));
	LeftJamb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftJamb"));
	RightJamb = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightJamb"));
	Lintel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lintel"));

	UStaticMeshComponent* Parts[] = { MouthMesh.Get(), LeftJamb.Get(), RightJamb.Get(), Lintel.Get() };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}
}

void ASolidDungeonEntrance::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidDungeonEntrance::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidDungeonEntrance::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidDungeonEntrance: missing Engine BasicShapes Cube."));
		return;
	}

	const float Width = SolidStrongholds::OpeningWidthCm;
	const float Height = SolidStrongholds::OpeningHeightCm;
	const float Depth = SolidStrongholds::OpeningDepthCm;
	const float Frame = SolidStrongholds::FrameThicknessCm;
	const float Recess = SolidStrongholds::MouthRecessCm;
	const float Gap = SolidStrongholds::MouthGapCm;
	// The actor origin is the grass where the captain stands. Shift the whole
	// frame toward the cliff so that spot is in front of the stone.
	const float ShiftX = SolidStrongholds::FrameFrontLocalX - Depth * 0.5f;

	PlacePart(
		MouthMesh,
		Cube,
		this,
		FVector(-Recess * 0.5f + ShiftX, 0.f, Height * 0.5f),
		FVector(Depth - Recess, Width - Gap, Height),
		SolidStrongholds::MouthColor(),
		TEXT("CaveMouth"));
	if (MouthMesh)
	{
		MouthMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		MouthMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	const float JambY = (Width + Frame) * 0.5f;
	PlacePart(
		LeftJamb,
		Cube,
		this,
		FVector(ShiftX, -JambY, Height * 0.5f),
		FVector(Depth, Frame, Height),
		SolidStrongholds::RockColor(),
		TEXT("CaveLeftJamb"));
	PlacePart(
		RightJamb,
		Cube,
		this,
		FVector(ShiftX, JambY, Height * 0.5f),
		FVector(Depth, Frame, Height),
		SolidStrongholds::RockColor(),
		TEXT("CaveRightJamb"));
	PlacePart(
		Lintel,
		Cube,
		this,
		FVector(ShiftX, 0.f, Height + Frame * 0.5f),
		FVector(Depth, Width + Frame * 2.f, Frame),
		SolidStrongholds::RockColor(),
		TEXT("CaveLintel"));
}
