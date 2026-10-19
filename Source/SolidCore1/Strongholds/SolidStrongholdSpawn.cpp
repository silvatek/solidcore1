#include "SolidStrongholdSpawn.h"
#include "SolidCore1.h"
#include "SolidStronghold.h"
#include "SolidStrongholdExterior.h"
#include "SolidStrongholdSite.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace SolidStronghold
{
	void SpawnStrongholds(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<const FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors)
	{
		if (!World || !Streamer || !Streamer->GetTerrainMap() || !Streamer->GetTerrainMap()->IsBuilt())
		{
			return;
		}

		Exteriors.RemoveAll([](const TObjectPtr<ASolidStrongholdExterior>& Exterior)
		{
			return !IsValid(Exterior);
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
	}

	void SpawnStrongholdSites(
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
}
