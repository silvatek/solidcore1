#include "SolidForage.h"
#include "Inventory/SolidInventory.h"
#include "SolidGameState.h"

int32 SolidForage::CollectChoices(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const int32 Seed,
	const FScatterParams& Params,
	TArray<FChoice>& OutChoices)
{
	OutChoices.Reset();
	const float Density = FMath::Clamp(Params.Density, 0.f, 1.f);
	const int32 MaxCount = FMath::Max(Params.MaxPiles, 0);
	if (MaxCount == 0 || Density <= 0.f)
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

	FRandomStream Rng(Seed != 0 ? Seed : SolidForage::Seed);
	for (int32 I = OpenForest.Num() - 1; I > 0; --I)
	{
		const int32 J = Rng.RandRange(0, I);
		OpenForest.Swap(I, J);
	}

	const int32 Target = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(OpenForest.Num()) * Density),
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

bool SolidForage::TakePile(FGameState& State, const int32 ChoiceIndex, const TCHAR* ItemId)
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
