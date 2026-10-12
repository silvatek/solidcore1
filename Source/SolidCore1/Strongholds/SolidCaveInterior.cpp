#include "SolidCaveInterior.h"
#include "SolidCaveLevel.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidStrongholds.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace
{
	void PlaceInteriorPart(
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
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.95f))
		{
			Part->SetMaterial(0, Material);
		}
	}
}

ASolidCaveInterior::ASolidCaveInterior()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	FloorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FloorMesh"));
	CeilingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CeilingMesh"));
	WallBack = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallBack"));
	WallLeft = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallLeft"));
	WallRight = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallRight"));
	WallFront = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallFront"));
	MouthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthMesh"));
	RoomLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RoomLight"));

	UStaticMeshComponent* Parts[] = {
		FloorMesh.Get(), CeilingMesh.Get(), WallBack.Get(), WallLeft.Get(),
		WallRight.Get(), WallFront.Get(), MouthMesh.Get() };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}

	RoomLight->SetupAttachment(SceneRoot);
	RoomLight->SetMobility(EComponentMobility::Movable);
	RoomLight->SetCastShadows(true);
	RoomLight->SetIntensity(6000.f);
	RoomLight->SetAttenuationRadius(2200.f);
	RoomLight->SetLightColor(FLinearColor(1.f, 0.82f, 0.55f));
}

void ASolidCaveInterior::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidCaveInterior::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidCaveInterior::BuildVisuals()
{
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidCaveInterior: missing Engine BasicShapes Cube."));
		return;
	}

	const float Depth = SolidCaveLevel::RoomDepthCm;
	const float Width = SolidCaveLevel::RoomWidthCm;
	const float Height = SolidCaveLevel::RoomHeightCm;
	const float Shell = SolidCaveLevel::ShellCm;
	const float SpanX = Depth + Shell * 2.f;
	const float SpanY = Width + Shell * 2.f;
	const FLinearColor Rock = SolidStrongholds::RockColor();

	PlaceInteriorPart(FloorMesh, Cube, this, FVector(Depth * 0.5f, 0.f, -Shell * 0.5f), FVector(SpanX, SpanY, Shell), Rock, TEXT("CaveFloor"));
	PlaceInteriorPart(CeilingMesh, Cube, this, FVector(Depth * 0.5f, 0.f, Height + Shell * 0.5f), FVector(SpanX, SpanY, Shell), Rock, TEXT("CaveCeiling"));
	PlaceInteriorPart(WallBack, Cube, this, FVector(Depth + Shell * 0.5f, 0.f, Height * 0.5f), FVector(Shell, SpanY, Height), Rock, TEXT("CaveWallBack"));
	PlaceInteriorPart(WallLeft, Cube, this, FVector(Depth * 0.5f, -(Width * 0.5f + Shell * 0.5f), Height * 0.5f), FVector(Depth, Shell, Height), Rock, TEXT("CaveWallLeft"));
	PlaceInteriorPart(WallRight, Cube, this, FVector(Depth * 0.5f, Width * 0.5f + Shell * 0.5f, Height * 0.5f), FVector(Depth, Shell, Height), Rock, TEXT("CaveWallRight"));
	PlaceInteriorPart(WallFront, Cube, this, FVector(-Shell * 0.5f, 0.f, Height * 0.5f), FVector(Shell, SpanY, Height), Rock, TEXT("CaveWallFront"));
	PlaceInteriorPart(
		MouthMesh,
		Cube,
		this,
		FVector(4.f, 0.f, SolidStrongholds::OpeningHeightCm * 0.5f),
		FVector(12.f, SolidStrongholds::OpeningWidthCm, SolidStrongholds::OpeningHeightCm),
		SolidStrongholds::MouthColor(),
		TEXT("CaveInteriorMouth"));

	if (RoomLight)
	{
		RoomLight->SetRelativeLocation(FVector(Depth * 0.5f, 0.f, Height * 0.72f));
	}
}

FVector ASolidCaveInterior::GetCaptainSpawnWorld() const
{
	return GetActorTransform().TransformPosition(SolidCaveLevel::CaptainSpawnLocal());
}

float ASolidCaveInterior::GetCaptainYaw() const
{
	return SolidCaveLevel::CaptainYawDeg;
}
