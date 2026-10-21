#include "SolidStronghold.h"
#include "SolidCore1.h"
#include "SolidGameState.h"
#include "Squad/SolidCompany.h"
#include "Strongholds/Actors/SolidStrongholdExterior.h"
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
		Stronghold->BindPlan();
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

namespace
{
	/** Map step (column, row) toward grass. Row +1 is south; column +1 is west. */
	const FIntPoint NeighborSteps[] = {
		FIntPoint(0, -1),
		FIntPoint(-1, 0),
		FIntPoint(0, 1),
		FIntPoint(1, 0),
	};

	bool InBounds(const int32 X, const int32 Y)
	{
		return X >= 0 && X < USolidWorldMap::MapWidth && Y >= 0 && Y < USolidWorldMap::MapHeight;
	}

	float YawFacingGrass(const FIntPoint& Step)
	{
		// Column +1 is world -X. Row +1 is world -Y.
		const float WorldX = -static_cast<float>(Step.X);
		const float WorldY = -static_cast<float>(Step.Y);
		return FMath::RadiansToDegrees(FMath::Atan2(WorldY, WorldX));
	}
}

bool SolidStrongholds::MakeEntrance(
	const USolidWorldMap* WorldMap,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	const FIntPoint Cell,
	FEntrance& OutEntrance)
{
	OutEntrance = FEntrance();
	if (!WorldMap || !WorldMap->IsLoaded() || !InBounds(Cell.X, Cell.Y))
	{
		return false;
	}

	const FIntPoint* ChosenStep = nullptr;
	FIntPoint GrassCell = FIntPoint::ZeroValue;
	for (const FIntPoint& Step : NeighborSteps)
	{
		const int32 NextX = Cell.X + Step.X;
		const int32 NextY = Cell.Y + Step.Y;
		if (!InBounds(NextX, NextY))
		{
			continue;
		}
		if (WorldMap->GetBiomeAtCell(NextX, NextY) != ESolidBiome::Grassland)
		{
			continue;
		}
		ChosenStep = &Step;
		GrassCell = FIntPoint(NextX, NextY);
		break;
	}
	if (!ChosenStep)
	{
		return false;
	}

	FVector2D CaveXY = FVector2D::ZeroVector;
	FVector2D GrassXY = FVector2D::ZeroVector;
	if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, CaveXY)
		|| !WorldMap->CellToWorldXY(GrassCell, WorldMinXY, WorldMaxXY, GrassXY))
	{
		return false;
	}

	OutEntrance.Cell = Cell;
	OutEntrance.GrassCell = GrassCell;
	OutEntrance.EdgeXY = (CaveXY + GrassXY) * 0.5f;
	const FVector2D TowardGrass = (GrassXY - CaveXY).GetSafeNormal();
	OutEntrance.WorldXY = OutEntrance.EdgeXY + TowardGrass * FaceInsetCm;
	OutEntrance.FacingYawDeg = YawFacingGrass(*ChosenStep);
	return true;
}

bool SolidStrongholds::FindGrassFace(
	const USolidWorldMap* WorldMap,
	const FIntPoint Cell,
	FGrassFace& Out)
{
	Out = FGrassFace();
	if (!WorldMap || !WorldMap->IsLoaded())
	{
		return false;
	}

	for (const FIntPoint& Step : NeighborSteps)
	{
		const int32 NextX = Cell.X + Step.X;
		const int32 NextY = Cell.Y + Step.Y;
		if (!InBounds(NextX, NextY))
		{
			continue;
		}
		if (WorldMap->GetBiomeAtCell(NextX, NextY) != ESolidBiome::Grassland)
		{
			continue;
		}
		Out.GrassCell = FIntPoint(NextX, NextY);
		Out.FacingYawDeg = YawFacingGrass(Step);
		return true;
	}
	return false;
}

bool SolidStrongholdTransition::IsFinalPair(const FSolidStronghold& Stronghold)
{
	if (Stronghold.Size == ESolidStrongholdSize::Meagre)
	{
		return Stronghold.Type == ESolidStrongholdType::Dungeon;
	}
	if (Stronghold.Size == ESolidStrongholdSize::Humble)
	{
		return Stronghold.Type == ESolidStrongholdType::Dungeon
			|| Stronghold.Type == ESolidStrongholdType::Fortress;
	}
	return true;
}

bool SolidStrongholdTransition::IsBuilt(const FSolidStronghold& Stronghold)
{
	return IsFinalPair(Stronghold)
		&& SolidStronghold::TypeOf(Stronghold.Type).Supported(Stronghold.Size);
}

SolidStrongholdTransition::EOffer SolidStrongholdTransition::Consider(
	const USolidGameState& State,
	const FSolidStronghold* Active,
	TArray<FSolidStrongholdChoice>& OutChoices)
{
	OutChoices.Reset();
	if (!Active)
	{
		return EOffer::None;
	}

	const int32 ActiveIndex = static_cast<int32>(Active->Size);
	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		if (!SolidEvents::IsStrongholdSizeUnlocked(State, Size))
		{
			continue;
		}
		if (State.bStrongholdSiteRestrictions && static_cast<int32>(Size) <= ActiveIndex)
		{
			continue;
		}
		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			if (!SolidStronghold::TypeOf(Type).Supported(Size))
			{
				continue;
			}
			FSolidStrongholdChoice Choice;
			Choice.Size = Size;
			Choice.Type = Type;
			OutChoices.Add(Choice);
		}
	}
	return OutChoices.Num() > 0 ? EOffer::Choose : EOffer::None;
}

bool SolidStrongholdTransition::Offers(
	const TArray<FSolidStrongholdChoice>& Choices,
	const ESolidStrongholdSize Size,
	const ESolidStrongholdType Type)
{
	for (const FSolidStrongholdChoice& Choice : Choices)
	{
		if (Choice.Size == Size && Choice.Type == Type)
		{
			return true;
		}
	}
	return false;
}

bool SolidStrongholdTransition::AreNeighbors(const SolidStrongholdGrid::FHex& A, const SolidStrongholdGrid::FHex& B)
{
	return FMath::IsNearlyEqual(
		FVector2D::Distance(A.Center, B.Center),
		SolidStrongholdGrid::FlatToFlatCm,
		15.f);
}

bool SolidStrongholdTransition::PlaceOnFloor(
	TArray<SolidStrongholdFurniture::FUsed>& Pieces,
	const TArray<SolidStrongholdGrid::FHex>& Hexes)
{
	TArray<SolidStrongholdFurniture::FUsed> Next = Pieces;
	TArray<int32> Taken;
	for (SolidStrongholdFurniture::FUsed& Piece : Next)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Piece.ItemIndex);
		if (!Item)
		{
			return false;
		}

		if (SolidStrongholdFurniture::HexCount(Item->Kind) >= 2)
		{
			int32 Anchor = INDEX_NONE;
			int32 Span = INDEX_NONE;
			for (int32 Index = 0; Index < Hexes.Num() && Anchor == INDEX_NONE; ++Index)
			{
				if (Taken.Contains(Index))
				{
					continue;
				}
				for (int32 Other = 0; Other < Hexes.Num(); ++Other)
				{
					if (Other == Index || Taken.Contains(Other) || !AreNeighbors(Hexes[Index], Hexes[Other]))
					{
						continue;
					}
					Anchor = Index;
					Span = Other;
					break;
				}
			}
			if (Anchor == INDEX_NONE)
			{
				return false;
			}
			Taken.Add(Anchor);
			Taken.Add(Span);
			const FVector2D Delta = Hexes[Span].Center - Hexes[Anchor].Center;
			Piece.Level = 0;
			Piece.X = Hexes[Anchor].Col;
			Piece.Y = Hexes[Anchor].Row;
			Piece.RotationDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
		}
		else
		{
			int32 Anchor = INDEX_NONE;
			for (int32 Index = 0; Index < Hexes.Num(); ++Index)
			{
				if (!Taken.Contains(Index))
				{
					Anchor = Index;
					break;
				}
			}
			if (Anchor == INDEX_NONE)
			{
				return false;
			}
			Taken.Add(Anchor);
			Piece.Level = 0;
			Piece.X = Hexes[Anchor].Col;
			Piece.Y = Hexes[Anchor].Row;
			Piece.RotationDeg = 0.f;
		}
	}

	Pieces = Next;
	return true;
}

bool SolidStrongholdTransition::Commit(
	USolidCompany& Company,
	USolidGameState& State,
	const FIntPoint SiteCell,
	const ESolidStrongholdSize Size,
	const ESolidStrongholdType Type)
{
	TArray<FSolidStrongholdChoice> Choices;
	if (Consider(State, Company.GetStronghold(), Choices) != EOffer::Choose || !Offers(Choices, Size, Type))
	{
		return false;
	}

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidStronghold::TypeOf(Type).CollectGroundHexes(Size, Hexes);
	if (Hexes.Num() == 0)
	{
		return false;
	}
	TArray<SolidStrongholdFurniture::FUsed> Moved = State.Furnishings;
	if (Moved.Num() == 0)
	{
		Moved = SolidStrongholdFurniture::StartingFurnishings();
	}
	if (!PlaceOnFloor(Moved, Hexes))
	{
		return false;
	}

	State.PreviousStrongholds.Add(Company.TakeStronghold());
	Company.SetStronghold(SolidStronghold::Make(
		Type,
		Size,
		SiteCell,
		SolidStronghold::WorldXY(SiteCell)));
	State.Furnishings = Moved;
	return true;
}

bool SolidStrongholdTransition::CompanyChestOpen(
	const USolidGameState& State,
	const FSolidStronghold* Active,
	const bool bInStronghold)
{
	if (!bInStronghold || !Active)
	{
		return false;
	}
	bool bHasChest = false;
	for (const SolidStrongholdFurniture::FUsed& Piece : State.Furnishings)
	{
		if (SolidStrongholdFurniture::IsStorage(SolidStrongholdFurniture::ItemAt(Piece.ItemIndex)))
		{
			bHasChest = true;
			break;
		}
	}
	if (!bHasChest)
	{
		return false;
	}
	return Active->AcceptsEnter(State, Active, Active->Cell);
}
