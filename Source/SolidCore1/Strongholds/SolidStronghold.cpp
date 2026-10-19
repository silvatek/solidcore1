#include "SolidStronghold.h"
#include "SolidCore1.h"
#include "SolidStrongholdExterior.h"
#include "Strongholds/Dungeon/SolidDungeon.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FSolidStronghold::FSolidStronghold(
	const ESolidStrongholdType InType,
	const ESolidStrongholdSize InSize,
	const FIntPoint InCell,
	const FVector2D InLocation)
	: Type(InType)
	, Size(InSize)
	, Cell(InCell)
	, Location(InLocation)
{
	Name = SolidStronghold::Label(*this);
}

TUniquePtr<FSolidStronghold> FSolidStronghold::Clone() const
{
	return MakeUnique<FSolidStronghold>(*this);
}

float FSolidStronghold::ExteriorYawDeg(const USolidWorldMap*) const
{
	return SolidStronghold::OrientationYawDeg(Orientation);
}

FVector2D FSolidStronghold::ExitStandXY() const
{
	const float YawRad = FMath::DegreesToRadians(SolidStronghold::OrientationYawDeg(Orientation));
	const FVector2D Forward(FMath::Cos(YawRad), FMath::Sin(YawRad));
	const FVector2D Left(-Forward.Y, Forward.X);
	return Location + Forward * ExitReachCm() + Left * ExitLateralCm();
}

void FSolidStronghold::RememberExit(const USolidWorldMap*)
{
}

namespace SolidStronghold
{
	FVector2D WorldXY(const FIntPoint Cell)
	{
		constexpr float PointSpacing = 200.f;
		constexpr int32 Grid = 513;
		const float Extent = static_cast<float>(Grid - 1) * PointSpacing;
		const float Half = Extent * 0.5f;
		const float CellW = Extent / static_cast<float>(USolidWorldMap::MapWidth);
		const float CellH = Extent / static_cast<float>(USolidWorldMap::MapHeight);
		return FVector2D(
			Half - (static_cast<float>(Cell.X) + 0.5f) * CellW,
			Half - (static_cast<float>(Cell.Y) + 0.5f) * CellH);
	}

	float CellExtentAlongExitCm(const ESolidStrongholdOrientation Orientation)
	{
		constexpr float PointSpacing = 200.f;
		constexpr int32 Grid = 513;
		const float Extent = static_cast<float>(Grid - 1) * PointSpacing;
		const float EastCm = Extent / static_cast<float>(USolidWorldMap::MapWidth);
		const float NorthCm = Extent / static_cast<float>(USolidWorldMap::MapHeight);
		switch (Orientation)
		{
		case ESolidStrongholdOrientation::East:
		case ESolidStrongholdOrientation::West:
			return EastCm;
		case ESolidStrongholdOrientation::North:
		case ESolidStrongholdOrientation::South:
		default:
			return NorthCm;
		}
	}

	TUniquePtr<FSolidStronghold> Make(
		const ESolidStrongholdType Type,
		const ESolidStrongholdSize Size,
		const FIntPoint Cell,
		const FVector2D InWorldXY)
	{
		TUniquePtr<FSolidStronghold> Stronghold = TypeOf(Type).Clone();
		Stronghold->Type = Type;
		Stronghold->Size = Size;
		Stronghold->Cell = Cell;
		Stronghold->Location = InWorldXY;
		Stronghold->Name = Label(*Stronghold);
		return Stronghold;
	}

	const FSolidStronghold& TypeOf(const ESolidStrongholdType Type)
	{
		switch (Type)
		{
		case ESolidStrongholdType::Dungeon:
			return FSolidDungeon::Instance();
		case ESolidStrongholdType::Fortress:
			return FSolidFortress::Instance();
		case ESolidStrongholdType::Tower:
			return FSolidTower::Instance();
		default:
			checkNoEntry();
			return FSolidDungeon::Instance();
		}
	}

	void SpawnStrongholds(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<FSolidStronghold*>& Company,
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
			for (FSolidStronghold* Stronghold : Company)
			{
				if (!Stronghold)
				{
					continue;
				}
				Stronghold->RememberExit(Map);
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
}
