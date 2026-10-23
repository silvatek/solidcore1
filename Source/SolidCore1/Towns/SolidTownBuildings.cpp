#include "SolidTownBuildings.h"
#include "SolidRoad.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTypes.h"
#include "SolidTowns.h"
#include "SolidWorldChart.h"

namespace
{
	/** A little grass between the wall and the strip, in centimetres. */
	constexpr float RoadVergeCm = 40.f;
	constexpr float RoadSlideStepCm = 50.f;

	template<typename Func>
	void ForEachFootprintPoint(const SolidTownBuildings::FPlacement& Placement, Func&& Visitor)
	{
		const FRotator Yaw(0.f, Placement.YawDeg, 0.f);
		const float HalfX = Placement.FootprintXCm * 0.5f + RoadVergeCm;
		const float HalfY = Placement.FootprintYCm * 0.5f + RoadVergeCm;
		constexpr int32 Steps = 4;
		for (int32 IndexY = 0; IndexY <= Steps; ++IndexY)
		{
			for (int32 IndexX = 0; IndexX <= Steps; ++IndexX)
			{
				const float LocalX = FMath::Lerp(-HalfX, HalfX, static_cast<float>(IndexX) / static_cast<float>(Steps));
				const float LocalY = FMath::Lerp(-HalfY, HalfY, static_cast<float>(IndexY) / static_cast<float>(Steps));
				const FVector Local = Yaw.RotateVector(FVector(LocalX, LocalY, 0.f));
				Visitor(Placement.CenterXY + FVector2D(Local.X, Local.Y));
			}
		}
	}
}

bool SolidTownBuildings::CoversRoad(
	const USolidWorldChart& Chart,
	const FPlacement& Placement,
	const FVector2D WorldMin,
	const FVector2D WorldMax,
	FVector2D* OutOnRoad)
{
	const FVector2D Shift = SolidRoad::StripPaintOffset();
	FVector2D Sum = FVector2D::ZeroVector;
	int32 Count = 0;
	ForEachFootprintPoint(Placement, [&](const FVector2D& Point)
	{
		const FVector2D Sample = Point - Shift;
		if (Chart.IsOnRoad(Sample.X, Sample.Y, WorldMin, WorldMax))
		{
			Sum += Point;
			++Count;
		}
	});
	if (OutOnRoad && Count > 0)
	{
		*OutOnRoad = Sum / static_cast<float>(Count);
	}
	return Count > 0;
}

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

	const USolidWorldChart* WorldMap = TerrainMap->GetWorldMap();
	const FVector2D WorldMin = TerrainMap->GetWorldMinXY();
	const FVector2D WorldMax = TerrainMap->GetWorldMaxXY();
	const bool bMapLoaded = WorldMap && WorldMap->IsLoaded();

	const TArray<FSolidTerrainPoint>& Points = TerrainMap->GetPoints();
	TArray<int32> TownIndices;
	TownIndices.Reserve(Points.Num() / 16);
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FSolidTerrainPoint& Point = Points[Index];
		if (Point.Biome != ESolidBiome::Town)
		{
			continue;
		}
		// Roads share a brown with the town, but the marker on the map is the town.
		if (bMapLoaded && !WorldMap->IsMarkedTown(Point.X, Point.Y, WorldMin, WorldMax))
		{
			continue;
		}
		TownIndices.Add(Index);
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
		if (bMapLoaded
			&& SolidTowns::IsWallGate(WorldMap->SampleLocationId(Center.X, Center.Y, WorldMin, WorldMax)))
		{
			continue;
		}

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

		auto Blocked = [&](const FVector2D& At) -> bool
		{
			if (bHasTownCenter && FVector2D::DistSquared(At, TownCenter) < ClearRadiusSq)
			{
				return true;
			}
			for (const FVector2D& ClearCenter : ExtraClearCenters)
			{
				if (FVector2D::DistSquared(At, ClearCenter) < ClearRadiusSq)
				{
					return true;
				}
			}
			if (!bMapLoaded)
			{
				return false;
			}
			if (!WorldMap->IsMarkedTown(At.X, At.Y, WorldMin, WorldMax)
				|| WorldMap->IsOnRoad(At.X, At.Y, WorldMin, WorldMax))
			{
				return true;
			}
			const ESolidBiome Ground = WorldMap->SampleBiome(At.X, At.Y, WorldMin, WorldMax);
			if (Ground == ESolidBiome::Sea || Ground == ESolidBiome::Mountain || Ground == ESolidBiome::River)
			{
				return true;
			}
			FPlacement Trial = Placement;
			Trial.CenterXY = At;
			if (CoversRoad(*WorldMap, Trial, WorldMin, WorldMax))
			{
				return true;
			}
			const float Radius = Trial.PackingRadiusCm() + Separation * 0.5f;
			for (const FPlacement& Existing : OutPlacements)
			{
				const float MinDist = Radius + Existing.PackingRadiusCm() + Separation * 0.5f;
				if (FVector2D::DistSquared(At, Existing.CenterXY) < MinDist * MinDist)
				{
					return true;
				}
			}
			return false;
		};

		if (bMapLoaded && CoversRoad(*WorldMap, Placement, WorldMin, WorldMax))
		{
			FVector2D OnRoad = Center;
			CoversRoad(*WorldMap, Placement, WorldMin, WorldMax, &OnRoad);
			FVector2D Away = Center - OnRoad;
			if (!Away.Normalize())
			{
				Away = FVector2D(1.f, 0.f);
			}
			const FVector2D Perp(-Away.Y, Away.X);
			const FVector2D Directions[] = { Away, -Away, Perp, -Perp };
			float BestDistance = RoadSlideCm + RoadSlideStepCm;
			FVector2D BestCenter = Center;
			bool bFound = false;
			for (const FVector2D& Direction : Directions)
			{
				for (float Distance = RoadSlideStepCm; Distance <= RoadSlideCm; Distance += RoadSlideStepCm)
				{
					const FVector2D Candidate = Center + Direction * Distance;
					if (Blocked(Candidate))
					{
						continue;
					}
					if (Distance < BestDistance)
					{
						BestDistance = Distance;
						BestCenter = Candidate;
						bFound = true;
					}
					break;
				}
			}
			if (!bFound)
			{
				continue;
			}
			Placement.CenterXY = BestCenter;
		}
		else if (Blocked(Center))
		{
			continue;
		}

		OutPlacements.Add(Placement);
	}

	return OutPlacements.Num();
}
