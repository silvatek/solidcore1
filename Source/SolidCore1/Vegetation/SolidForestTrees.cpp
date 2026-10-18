#include "SolidForestTrees.h"
#include "SolidTerrainMap.h"

namespace
{
	int32 ChoosePointIndices(
		const TArray<FSolidTerrainPoint>& Points,
		FRandomStream& Rng,
		const SolidForestTrees::FScatterParams& Params,
		TArray<int32>& OutIndices)
	{
		OutIndices.Reset();
		const float Density = FMath::Clamp(Params.Density, 0.f, 1.f);
		const int32 MaxTrees = FMath::Max(Params.MaxTrees, 0);
		if (MaxTrees == 0 || Density <= 0.f)
		{
			return 0;
		}

		TArray<int32> ForestIndices;
		ForestIndices.Reserve(Points.Num() / 8);
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			if (Points[Index].Biome == ESolidBiome::Forest)
			{
				ForestIndices.Add(Index);
			}
		}

		if (ForestIndices.Num() == 0)
		{
			return 0;
		}

		// Fisher–Yates so the cap does not bias toward map origin.
		for (int32 I = ForestIndices.Num() - 1; I > 0; --I)
		{
			const int32 J = Rng.RandRange(0, I);
			ForestIndices.Swap(I, J);
		}

		const int32 Target = FMath::Clamp(
			FMath::RoundToInt(static_cast<float>(ForestIndices.Num()) * Density),
			0,
			FMath::Min(MaxTrees, ForestIndices.Num()));
		if (Target <= 0)
		{
			return 0;
		}

		OutIndices.Reserve(Target);
		for (int32 I = 0; I < Target; ++I)
		{
			OutIndices.Add(ForestIndices[I]);
		}
		return OutIndices.Num();
	}
}

int32 SolidForestTrees::CollectSpawnPositions(
	const USolidTerrainMap* TerrainMap,
	const int32 Seed,
	const FScatterParams& Params,
	TArray<FVector2D>& OutXY)
{
	OutXY.Reset();
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return 0;
	}

	FRandomStream Rng(Seed != 0 ? Seed : 1337);
	TArray<int32> Indices;
	if (ChoosePointIndices(TerrainMap->GetPoints(), Rng, Params, Indices) <= 0)
	{
		return 0;
	}

	const float Jitter = FMath::Max(Params.JitterCm, 0.f);
	const TArray<FSolidTerrainPoint>& Points = TerrainMap->GetPoints();
	OutXY.Reserve(Indices.Num());
	for (const int32 Index : Indices)
	{
		const FSolidTerrainPoint& Point = Points[Index];
		const float Jx = Rng.FRandRange(-Jitter, Jitter);
		const float Jy = Rng.FRandRange(-Jitter, Jitter);
		OutXY.Add(FVector2D(Point.X + Jx, Point.Y + Jy));
	}

	return OutXY.Num();
}

int32 SolidForestTrees::CollectChosenPointIndices(
	const TArray<FSolidTerrainPoint>& Points,
	const int32 Seed,
	const FScatterParams& Params,
	TArray<int32>& OutIndices)
{
	FRandomStream Rng(Seed != 0 ? Seed : 1337);
	return ChoosePointIndices(Points, Rng, Params, OutIndices);
}
