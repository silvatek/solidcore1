#include "SolidForage.h"
#include "Inventory/SolidInventory.h"
#include "SolidGameState.h"
#include "Terrain/SolidWorldChart.h"

int32 SolidForage::CollectChoices(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const int32 ChoiceSeed,
	const FScatterParams& Params,
	TArray<FChoice>& OutChoices)
{
	OutChoices.Reset();
	const float UsedDensity = FMath::Clamp(Params.Density, 0.f, 1.f);
	const int32 MaxCount = FMath::Max(Params.MaxPiles, 0);
	if (MaxCount == 0 || UsedDensity <= 0.f)
	{
		return 0;
	}

	TArray<int32> OpenForest;
	OpenForest.Reserve(Points.Num() / 8);
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		if (!OccursInBiome(SolidItemType::IdStick, Points[Index].Biome))
		{
			continue;
		}
		if (TreePointIndices.Contains(Index))
		{
			continue;
		}
		OpenForest.Add(Index);
	}

	if (OpenForest.Num() == 0)
	{
		return 0;
	}

	FRandomStream Rng(ChoiceSeed != 0 ? ChoiceSeed : SolidForage::Seed);
	for (int32 I = OpenForest.Num() - 1; I > 0; --I)
	{
		const int32 J = Rng.RandRange(0, I);
		OpenForest.Swap(I, J);
	}

	const int32 Target = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(OpenForest.Num()) * UsedDensity),
		0,
		FMath::Min(MaxCount, OpenForest.Num()));
	if (Target <= 0)
	{
		return 0;
	}

	const float Jitter = FMath::Max(Params.JitterCm, 0.f);
	OutChoices.Reserve(Target);
	for (int32 I = 0; I < Target; ++I)
	{
		const int32 PointIndex = OpenForest[I];
		const FSolidTerrainPoint& Point = Points[PointIndex];
		FChoice Choice;
		Choice.PointIndex = PointIndex;
		Choice.XY = FVector2D(
			Point.X + Rng.FRandRange(-Jitter, Jitter),
			Point.Y + Rng.FRandRange(-Jitter, Jitter));
		Choice.ItemId = (I % 2 == 0) ? SolidItemType::IdStick : SolidItemType::IdPitch;
		OutChoices.Add(Choice);
	}
	return OutChoices.Num();
}

int32 SolidForage::GuaranteeForestCells(
	const TArray<FSolidTerrainPoint>& Points,
	const USolidWorldChart* WorldMap,
	const FVector2D& WorldMinXY,
	const FVector2D& WorldMaxXY,
	const FVector2D& TownXY,
	const float RadiusCm,
	const TArray<int32>& TreePointIndices,
	TArray<FChoice>& InOutChoices)
{
	if (!WorldMap || !WorldMap->IsLoaded() || RadiusCm <= 0.f || Points.Num() == 0)
	{
		return 0;
	}

	const int32 Before = InOutChoices.Num();

	TSet<int32> TreeIndexes;
	TreeIndexes.Reserve(TreePointIndices.Num());
	for (const int32 TreeIndex : TreePointIndices)
	{
		TreeIndexes.Add(TreeIndex);
	}

	TArray<uint8> PointUsed;
	PointUsed.Init(0, Points.Num());

	const int32 CellCount = USolidWorldChart::MapWidth * USolidWorldChart::MapHeight;
	TArray<uint8> HasStick;
	TArray<uint8> HasPitch;
	HasStick.Init(0, CellCount);
	HasPitch.Init(0, CellCount);

	for (const FChoice& Choice : InOutChoices)
	{
		if (!Points.IsValidIndex(Choice.PointIndex) || !Choice.ItemId)
		{
			continue;
		}
		PointUsed[Choice.PointIndex] = 1;

		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!WorldMap->CellAtWorld(Points[Choice.PointIndex].X, Points[Choice.PointIndex].Y, WorldMinXY, WorldMaxXY, Cell))
		{
			continue;
		}
		const int32 Key = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
		if (!HasStick.IsValidIndex(Key))
		{
			continue;
		}
		if (FCString::Strcmp(Choice.ItemId, SolidItemType::IdStick) == 0)
		{
			HasStick[Key] = 1;
		}
		else if (FCString::Strcmp(Choice.ItemId, SolidItemType::IdPitch) == 0)
		{
			HasPitch[Key] = 1;
		}
	}

	struct FOpenPoints
	{
		TArray<int32> Indexes;
		int32 Next = 0;
	};
	TMap<int32, FOpenPoints> OpenByCell;

	for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
	{
		if (PointUsed[PointIndex] || TreeIndexes.Contains(PointIndex))
		{
			continue;
		}
		if (!OccursInBiome(SolidItemType::IdStick, Points[PointIndex].Biome))
		{
			continue;
		}

		FIntPoint Cell = FIntPoint::ZeroValue;
		if (!WorldMap->CellAtWorld(Points[PointIndex].X, Points[PointIndex].Y, WorldMinXY, WorldMaxXY, Cell))
		{
			continue;
		}
		if (WorldMap->GetMarkerAtCell(Cell.X, Cell.Y) != USolidWorldChart::ForestMarker)
		{
			continue;
		}

		FVector2D Center = FVector2D::ZeroVector;
		if (!WorldMap->CellToWorldXY(Cell, WorldMinXY, WorldMaxXY, Center))
		{
			continue;
		}
		if (FVector2D::Distance(Center, TownXY) > RadiusCm)
		{
			continue;
		}

		const int32 Key = Cell.Y * USolidWorldChart::MapWidth + Cell.X;
		OpenByCell.FindOrAdd(Key).Indexes.Add(PointIndex);
	}

	auto TakeOpenPoint = [&PointUsed, &OpenByCell](const int32 Key) -> int32
	{
		FOpenPoints* Open = OpenByCell.Find(Key);
		if (!Open)
		{
			return INDEX_NONE;
		}
		while (Open->Next < Open->Indexes.Num())
		{
			const int32 PointIndex = Open->Indexes[Open->Next];
			++Open->Next;
			if (PointUsed.IsValidIndex(PointIndex) && PointUsed[PointIndex] == 0)
			{
				PointUsed[PointIndex] = 1;
				return PointIndex;
			}
		}
		return INDEX_NONE;
	};

	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			if (WorldMap->GetMarkerAtCell(Col, Row) != USolidWorldChart::ForestMarker)
			{
				continue;
			}

			FVector2D Center = FVector2D::ZeroVector;
			if (!WorldMap->CellToWorldXY(FIntPoint(Col, Row), WorldMinXY, WorldMaxXY, Center))
			{
				continue;
			}
			if (FVector2D::Distance(Center, TownXY) > RadiusCm)
			{
				continue;
			}

			const int32 Key = Row * USolidWorldChart::MapWidth + Col;
			if (!HasStick.IsValidIndex(Key))
			{
				continue;
			}

			if (HasStick[Key] == 0)
			{
				const int32 PointIndex = TakeOpenPoint(Key);
				if (PointIndex != INDEX_NONE)
				{
					FChoice Choice;
					Choice.PointIndex = PointIndex;
					Choice.XY = FVector2D(Points[PointIndex].X, Points[PointIndex].Y);
					Choice.ItemId = SolidItemType::IdStick;
					InOutChoices.Add(Choice);
					HasStick[Key] = 1;
				}
			}
			if (HasPitch[Key] == 0)
			{
				const int32 PointIndex = TakeOpenPoint(Key);
				if (PointIndex != INDEX_NONE)
				{
					FChoice Choice;
					Choice.PointIndex = PointIndex;
					Choice.XY = FVector2D(Points[PointIndex].X, Points[PointIndex].Y);
					Choice.ItemId = SolidItemType::IdPitch;
					InOutChoices.Add(Choice);
					HasPitch[Key] = 1;
				}
			}
		}
	}

	return InOutChoices.Num() - Before;
}

void SolidForage::CollectReadyToSeed(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<FChoice>& Choices,
	const TArray<int32>& Taken,
	const TArray<int32>& Seeded,
	TArray<int32>& OutReady)
{
	OutReady.Reset();
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		if (Taken.Contains(Index) || Seeded.Contains(Index))
		{
			continue;
		}
		const int32 PointIndex = Choices[Index].PointIndex;
		if (!Points.IsValidIndex(PointIndex))
		{
			continue;
		}
		if (!(Points[PointIndex].Fog < 1.f))
		{
			continue;
		}
		OutReady.Add(Index);
	}
}

bool SolidForage::TakePile(USolidGameState& State, const int32 ChoiceIndex, const TCHAR* ItemId)
{
	if (ChoiceIndex < 0 || State.TakenForagePiles.Contains(ChoiceIndex))
	{
		return false;
	}
	if (!SolidInventory::InsertPack(State.Inventory, ItemId))
	{
		return false;
	}
	State.TakenForagePiles.Add(ChoiceIndex);
	return true;
}
