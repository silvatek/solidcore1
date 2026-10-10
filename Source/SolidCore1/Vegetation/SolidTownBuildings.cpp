#include "SolidTownBuildings.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTypes.h"

int32 SolidTownBuildings::CollectPlacements(
	const USolidTerrainMap* TerrainMap,
	const int32 Seed,
	const FScatterParams& Params,
	TArray<FPlacement>& OutPlacements,
	const TArray<FVector2D>& ExtraClearCenters)
{
	OutPlacements.Reset();
	if (!TerrainMap || !TerrainMap->IsBuilt())
	{
		return 0;
	}

	const float Density = FMath::Clamp(Params.Density, 0.f, 1.f);
	const int32 MaxBuildings = FMath::Max(Params.MaxBuildings, 0);
	if (MaxBuildings == 0 || Density <= 0.f)
	{
		return 0;
	}

	const TArray<FSolidTerrainPoint>& Points = TerrainMap->GetPoints();
	TArray<int32> TownIndices;
	TownIndices.Reserve(Points.Num() / 16);
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		if (Points[Index].Biome == ESolidBiome::Town)
		{
			TownIndices.Add(Index);
		}
	}
	if (TownIndices.Num() == 0)
	{
		return 0;
	}

	FRandomStream Rng(Seed != 0 ? Seed : 1337);
	for (int32 I = TownIndices.Num() - 1; I > 0; --I)
	{
		const int32 J = Rng.RandRange(0, I);
		TownIndices.Swap(I, J);
	}

	const int32 CandidateCount = FMath::Clamp(
		FMath::RoundToInt(static_cast<float>(TownIndices.Num()) * Density),
		0,
		TownIndices.Num());
	if (CandidateCount <= 0)
	{
		return 0;
	}

	FVector2D TownCenter = FVector2D::ZeroVector;
	const bool bHasTownCenter = TerrainMap->GetStartTownWorldXY(TownCenter);
	const float ClearRadius = FMath::Max(Params.ClearRadiusAroundTownCenterCm, 0.f);
	const float ClearRadiusSq = ClearRadius * ClearRadius;
	const float Separation = FMath::Max(Params.MinSeparationCm, 0.f);

	OutPlacements.Reserve(FMath::Min(MaxBuildings, CandidateCount));
	for (int32 I = 0; I < CandidateCount && OutPlacements.Num() < MaxBuildings; ++I)
	{
		const FSolidTerrainPoint& Point = Points[TownIndices[I]];
		const FVector2D Center(Point.X, Point.Y);

		if (bHasTownCenter && FVector2D::DistSquared(Center, TownCenter) < ClearRadiusSq)
		{
			continue;
		}
		bool bInsideExtraClear = false;
		for (const FVector2D& ClearCenter : ExtraClearCenters)
		{
			if (FVector2D::DistSquared(Center, ClearCenter) < ClearRadiusSq)
			{
				bInsideExtraClear = true;
				break;
			}
		}
		if (bInsideExtraClear)
		{
			continue;
		}

		FPlacement Placement;
		Placement.CenterXY = Center;
		Placement.FootprintXCm = Rng.FRandRange(Params.MinFootprintCm, Params.MaxFootprintCm);
		Placement.FootprintYCm = Rng.FRandRange(Params.MinFootprintCm, Params.MaxFootprintCm);
		Placement.BodyHeightCm = Rng.FRandRange(Params.MinBodyHeightCm, Params.MaxBodyHeightCm);
		Placement.RoofHeightCm = Rng.FRandRange(Params.MinRoofHeightCm, Params.MaxRoofHeightCm);
		Placement.YawDeg = Rng.FRandRange(0.f, 360.f);

		const float Radius = Placement.PackingRadiusCm() + Separation * 0.5f;
		bool bOverlaps = false;
		for (const FPlacement& Existing : OutPlacements)
		{
			const float MinDist = Radius + Existing.PackingRadiusCm() + Separation * 0.5f;
			if (FVector2D::DistSquared(Center, Existing.CenterXY) < MinDist * MinDist)
			{
				bOverlaps = true;
				break;
			}
		}
		if (bOverlaps)
		{
			continue;
		}

		OutPlacements.Add(Placement);
	}

	return OutPlacements.Num();
}
