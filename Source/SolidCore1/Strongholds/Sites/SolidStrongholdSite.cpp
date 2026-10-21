#include "SolidStrongholdSite.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "Terrain/SolidTerrainSeat.h"
#include "Strongholds/SolidStronghold.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

ASolidStrongholdSite::ASolidStrongholdSite()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Block"));
	BlockMesh->SetupAttachment(SceneRoot);
	BlockMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void ASolidStrongholdSite::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidStrongholdSite::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidStrongholdSite::SetSite(const FIntPoint InCell, const ESolidStrongholdSize InSize)
{
	Cell = InCell;
	Size = InSize;
}

void ASolidStrongholdSite::BuildVisuals()
{
	if (!BlockMesh)
	{
		return;
	}

	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Cube)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidStrongholdSite: missing Engine BasicShapes Cube."));
		return;
	}

	constexpr float ShapeSize = 100.f;
	BlockMesh->SetStaticMesh(Cube);
	BlockMesh->SetRelativeLocation(FVector(0.f, 0.f, BlockCm * 0.5f));
	BlockMesh->SetRelativeRotation(FRotator::ZeroRotator);
	BlockMesh->SetRelativeScale3D(FVector(BlockCm / ShapeSize));
	SolidTerrainSeat::FFootprint Seat;
	Seat.HalfXCm = BlockCm * 0.5f;
	Seat.HalfYCm = BlockCm * 0.5f;
	const float Drop = SolidTerrainSeat::DropCm(this, Seat);
	SolidTerrainSeat::BuryCentered(BlockMesh, 0.f, BlockCm, Drop, ShapeSize);
	if (UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
		this, SolidStrongholds::RockColor(), TEXT("StrongholdSite"), 0.94f))
	{
		BlockMesh->SetMaterial(0, Material);
	}
}

void ASolidStrongholdSite::Spawn(
	UWorld* World,
	ASolidTerrainStreamer* Streamer,
	const TArray<const FSolidStronghold*>& Company,
	const FActorSpawnParameters& SpawnParams,
	TArray<TObjectPtr<ASolidStrongholdSite>>& Sites,
	bool& bSitesSpawned)
{
	if (!World || !Streamer || !Streamer->GetTerrainMap() || !Streamer->GetTerrainMap()->IsBuilt())
	{
		return;
	}

	Sites.RemoveAll([](const TObjectPtr<ASolidStrongholdSite>& Site)
	{
		return !IsValid(Site);
	});

	USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
	USolidWorldMap* Map = Terrain->GetWorldMap();
	if (bSitesSpawned || !Map || !Map->IsLoaded())
	{
		return;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	bSitesSpawned = true;
	int32 SpawnedSites = 0;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
			if (!Map->StrongholdSiteAtCell(Col, Row, SiteSize))
			{
				continue;
			}

			const FIntPoint SiteCell(Col, Row);
			FVector2D SiteXY = FVector2D::ZeroVector;
			if (!Map->CellToWorldXY(SiteCell, WorldMin, WorldMax, SiteXY))
			{
				continue;
			}

			const FSolidStronghold* Owned = nullptr;
			for (const FSolidStronghold* Stronghold : Company)
			{
				if (Stronghold && Stronghold->Location.Equals(SiteXY, 1.f))
				{
					Owned = Stronghold;
					break;
				}
			}
			if (Owned && Owned->ExteriorClass())
			{
				continue;
			}

			FVector SpawnLoc(SiteXY.X, SiteXY.Y, 0.f);
			const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
			SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;
			ASolidStrongholdSite* Site = World->SpawnActor<ASolidStrongholdSite>(
				ASolidStrongholdSite::StaticClass(),
				SpawnLoc,
				FRotator::ZeroRotator,
				SpawnParams);
			if (!Site)
			{
				continue;
			}
			Site->SetSite(SiteCell, SiteSize);
			Site->BuildVisuals();
			Sites.Add(Site);
			++SpawnedSites;
		}
	}
	UE_LOG(LogSolid, Warning, TEXT("Spawned %d stronghold sites"), SpawnedSites);
}
