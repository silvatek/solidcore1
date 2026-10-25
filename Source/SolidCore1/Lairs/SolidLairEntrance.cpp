#include "SolidLairEntrance.h"
#include "Strongholds/Dungeon/SolidDungeon.h"
#include "Strongholds/Dungeon/SolidDungeonExterior.h"
#include "Strongholds/SolidStronghold.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"

namespace SolidLairEntrancePrivate
{
	void SetPart(
		UStaticMeshComponent* Part,
		UStaticMesh* Mesh,
		AActor* Outer,
		const FVector& Location,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!Part || !Mesh)
		{
			return;
		}

		Part->SetStaticMesh(Mesh);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(FRotator::ZeroRotator);
		Part->SetRelativeScale3D(FVector::OneVector);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
		if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(Outer, Color, DebugName, 0.94f))
		{
			Part->SetMaterial(0, Material);
		}
	}
}

ASolidLairEntrance::ASolidLairEntrance()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	RockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RockMesh"));
	MouthMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MouthMesh"));

	UStaticMeshComponent* Parts[] = { RockMesh.Get(), MouthMesh.Get() };
	for (UStaticMeshComponent* Part : Parts)
	{
		Part->SetupAttachment(SceneRoot);
		Part->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Part->SetCollisionResponseToAllChannels(ECR_Block);
		Part->SetCastShadow(true);
	}
}

void ASolidLairEntrance::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidLairEntrance::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidLairEntrance::BuildVisuals()
{
	const SolidStronghold::FSolidDungeon& Dungeon = SolidStronghold::FSolidDungeon::Instance();
	float Drop = 0.f;
	if (UWorld* World = GetWorld())
	{
		const USolidTerrainMap* Terrain = nullptr;
		float SurfaceBias = 0.f;
		for (TActorIterator<ASolidTerrainStreamer> It(World); It; ++It)
		{
			Terrain = It->GetTerrainMap();
			SurfaceBias = It->CollisionHeightBias;
			break;
		}
		Drop = ASolidDungeonExterior::GroundDropCm(Terrain, SurfaceBias, GetActorLocation());
	}

	UStaticMesh* Rock = SolidDungeonExteriorPrivate::BuildRock(
		this,
		Dungeon.FaceWidthCm(),
		Dungeon.FaceHeightCm(),
		Dungeon.SlopeRunCm(),
		SolidStrongholds::OpeningWidthCm,
		SolidStrongholds::OpeningHeightCm,
		Drop);
	UStaticMesh* Mouth = SolidDungeonExteriorPrivate::BuildMouth(
		this,
		SolidStrongholds::OpeningWidthCm,
		SolidStrongholds::OpeningHeightCm,
		SolidStrongholds::FrameThicknessCm,
		Drop);
	if (!Rock || !Mouth)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidLairEntrance: the dungeon rock did not build."));
		return;
	}

	// The actor origin is the grass where the captain stands. The opening sits
	// toward the cliff, so that spot is in front of the stone.
	const FVector Face(-Dungeon.LeaveStandCm(), 0.f, 0.f);
	SolidLairEntrancePrivate::SetPart(
		RockMesh,
		Rock,
		this,
		Face,
		SolidStrongholds::RockColor(),
		TEXT("LairRock"));
	SolidLairEntrancePrivate::SetPart(
		MouthMesh,
		Mouth,
		this,
		Face,
		SolidStrongholds::MouthColor(),
		TEXT("LairMouth"));
	if (MouthMesh)
	{
		MouthMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}
}
