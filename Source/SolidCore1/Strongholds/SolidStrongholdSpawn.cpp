#include "SolidStrongholdSpawn.h"
#include "SolidCore1.h"
#include "SolidStronghold.h"
#include "SolidStrongholdExterior.h"
#include "SolidStrongholdStone.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace SolidStronghold
{
	void SpawnOutposts(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors,
		TArray<TObjectPtr<ASolidStrongholdStone>>& Stones,
		bool& bStonesSpawned)
	{
		if (!World || !Streamer || !Streamer->GetTerrainMap() || !Streamer->GetTerrainMap()->IsBuilt())
		{
			return;
		}

		Exteriors.RemoveAll([](const TObjectPtr<ASolidStrongholdExterior>& Exterior)
		{
			return !IsValid(Exterior);
		});
		Stones.RemoveAll([](const TObjectPtr<ASolidStrongholdStone>& Stone)
		{
			return !IsValid(Stone);
		});

		if (Exteriors.Num() == 0)
		{
			for (TActorIterator<ASolidStrongholdExterior> It(World); It; ++It)
			{
				Exteriors.Add(*It);
			}
		}

		USolidTerrainMap* Terrain = Streamer->GetTerrainMap();
		USolidWorldMap* Map = Terrain->GetWorldMap();
		if (Exteriors.Num() == 0 && Map && Map->IsLoaded())
		{
			int32 Spawned = 0;
			for (const FSolidStronghold* Stronghold : Company)
			{
				if (!Stronghold)
				{
					continue;
				}
				UClass* ExteriorClass = Stronghold->ExteriorClass();
				if (!ExteriorClass)
				{
					continue;
				}

				FVector SpawnLoc(Stronghold->Location.X, Stronghold->Location.Y, 0.f);
				const float LandZ = Streamer->GetHeightAt(SpawnLoc) + Streamer->CollisionHeightBias;
				SpawnLoc.Z = LandZ + Streamer->SnapHeightPadding;
				const FRotator Rotation(0.f, Stronghold->ExteriorYawDeg(Map), 0.f);
				AActor* SpawnedActor = World->SpawnActor(ExteriorClass, &SpawnLoc, &Rotation, SpawnParams);
				ASolidStrongholdExterior* Exterior = Cast<ASolidStrongholdExterior>(SpawnedActor);
				if (!Exterior)
				{
					continue;
				}

				Stronghold->ConfigureExterior(Exterior);
				Exteriors.Add(Exterior);
				++Spawned;
			}
			if (Spawned > 0)
			{
				UE_LOG(LogSolid, Warning, TEXT("Spawned %d stronghold exteriors"), Spawned);
			}
		}

		if (bStonesSpawned || !Map || !Map->IsLoaded())
		{
			return;
		}

		const FVector2D WorldMin = Terrain->GetWorldMinXY();
		const FVector2D WorldMax = Terrain->GetWorldMaxXY();
		bStonesSpawned = true;
		int32 SpawnedStones = 0;
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
				ASolidStrongholdStone* Stone = World->SpawnActor<ASolidStrongholdStone>(
					ASolidStrongholdStone::StaticClass(),
					SpawnLoc,
					FRotator::ZeroRotator,
					SpawnParams);
				if (!Stone)
				{
					continue;
				}
				Stone->SetSite(SiteCell, SiteSize);
				Stone->BuildVisuals();
				Stones.Add(Stone);
				++SpawnedStones;
			}
		}
		UE_LOG(LogSolid, Warning, TEXT("Spawned %d stronghold stones"), SpawnedStones);
	}
}
