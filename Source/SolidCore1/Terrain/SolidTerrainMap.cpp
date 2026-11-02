#include "SolidTerrainMap.h"
#include "SolidBiome.h"
#include "SolidBridge.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainNoise.h"
#include "SolidWorldChart.h"
#include "SolidCore1.h"

void USolidTerrainMap::Build(
	int32 InSeed,
	float FrequencyScale,
	float Amplitude,
	float BaseHeight,
	int32 InGridWidth,
	int32 InGridHeight,
	float InPointSpacing,
	bool bForceRebuild,
	USolidWorldChart* InWorldMap)
{
	if (bIsBuilt && !bForceRebuild && Points.Num() == GridWidth * GridHeight)
	{
		return;
	}

	GridWidth = FMath::Clamp(InGridWidth, 2, 2049);
	GridHeight = FMath::Clamp(InGridHeight, 2, 2049);
	PointSpacing = FMath::Max(InPointSpacing, 50.f);
	Seed = InSeed;

	// Center the grid on world origin so the default spawn sits mid-map.
	WorldMinCm = FVector2D(
		-0.5f * static_cast<float>(GridWidth - 1) * PointSpacing,
		-0.5f * static_cast<float>(GridHeight - 1) * PointSpacing);

	if (InWorldMap)
	{
		WorldMap = InWorldMap;
	}
	else
	{
		if (!WorldMap)
		{
			WorldMap = NewObject<USolidWorldChart>(this, TEXT("WorldMap"));
		}
		WorldMap->LoadDefault();
	}

	// Fully fogged until the player is placed. The Z cell is the spawn, not the fog center.
	FogOriginWorldCm = FVector2D::ZeroVector;

	Points.SetNum(GridWidth * GridHeight);

	const bool bUseWorldMap = WorldMap && WorldMap->IsLoaded();
	const FVector2D WorldMin = GetWorldMinXY();
	const FVector2D WorldMax = GetWorldMaxXY();

	for (int32 TerrainY = 0; TerrainY < GridHeight; ++TerrainY)
	{
		for (int32 TerrainX = 0; TerrainX < GridWidth; ++TerrainX)
		{
			FSolidTerrainPoint& Point = Points[TerrainY * GridWidth + TerrainX];
			Point.WorldX = WorldMinCm.X + static_cast<float>(TerrainX) * PointSpacing;
			Point.WorldY = WorldMinCm.Y + static_cast<float>(TerrainY) * PointSpacing;
			if (bUseWorldMap)
			{
				Point.Biome = WorldMap->SampleBiome(Point.WorldX, Point.WorldY, WorldMin, WorldMax);
				const float Variance = SolidBiome::Of(Point.Biome).HeightVarianceCm();
				Point.Height = SolidTerrainNoise::SampleHeight(
					Point.WorldX, Point.WorldY, Seed, FrequencyScale, Variance, BaseHeight);
			}
			else
			{
				Point.Height = SolidTerrainNoise::SampleHeight(
					Point.WorldX, Point.WorldY, Seed, FrequencyScale, Amplitude, BaseHeight);
				const float HeightNorm = FMath::Clamp(
					(Point.Height - BaseHeight) / FMath::Max(Amplitude, 1.f), 0.f, 1.f);
				Point.Biome = ChooseBiome(Point.WorldX, Point.WorldY, HeightNorm, Seed);
			}
			FillThreatAndFog(Point);
			Point.Height += SolidBiome::Of(Point.Biome).HeightOffsetCm();
		}
	}

	bIsBuilt = true;
	if (bUseWorldMap && WorldMap)
	{
		TArray<SolidBridge::FBridge> Bridges;
		const TFunction<float(float, float)> HeightAt = [this](const float X, const float Y)
		{
			return SampleHeight(X, Y);
		};
		SolidBridge::FromMarkers(*WorldMap, HeightAt, Bridges, WorldMin, WorldMax);
		for (const SolidBridge::FBridge& Bridge : Bridges)
		{
			SolidBridge::PaintBuriedRamp(*WorldMap, Bridge, HeightAt, WorldMin, WorldMax);
		}
		const float RoadShelf = SolidBiome::Of(ESolidBiome::Road).HeightOffsetCm();
		for (FSolidTerrainPoint& Point : Points)
		{
			if (SolidBiome::Of(Point.Biome).HeightOffsetCm() != RoadShelf)
			{
				continue;
			}
			if (WorldMap->SampleBiome(Point.WorldX, Point.WorldY, WorldMin, WorldMax) == ESolidBiome::Road)
			{
				Point.Biome = ESolidBiome::Road;
			}
		}
	}
	TrackClearFogBounds();
	UE_LOG(LogSolid, Warning,
		TEXT("TerrainMap built: %dx%d spacing=%.0f cm seed=%d origin=(%.0f,%.0f) extent=(%.0f,%.0f) worldMap=%s fogOrigin=(%.0f,%.0f)"),
		GridWidth, GridHeight, PointSpacing, Seed,
		WorldMinCm.X, WorldMinCm.Y, GetWorldMaxXY().X, GetWorldMaxXY().Y,
		bUseWorldMap ? TEXT("yes") : TEXT("no"),
		FogOriginWorldCm.X, FogOriginWorldCm.Y);
}

void USolidTerrainMap::WorldToIndex(float WorldX, float WorldY, int32& OutTerrainX, int32& OutTerrainY) const
{
	const float FX = (WorldX - WorldMinCm.X) / PointSpacing;
	const float FY = (WorldY - WorldMinCm.Y) / PointSpacing;
	OutTerrainX = FMath::Clamp(FMath::RoundToInt(FX), 0, GridWidth - 1);
	OutTerrainY = FMath::Clamp(FMath::RoundToInt(FY), 0, GridHeight - 1);
}

const FSolidTerrainPoint& USolidTerrainMap::GetPoint(int32 TerrainX, int32 TerrainY) const
{
	check(Points.Num() > 0);
	TerrainX = FMath::Clamp(TerrainX, 0, GridWidth - 1);
	TerrainY = FMath::Clamp(TerrainY, 0, GridHeight - 1);
	return Points[TerrainY * GridWidth + TerrainX];
}

const FSolidTerrainPoint& USolidTerrainMap::GetNearestPoint(float WorldX, float WorldY) const
{
	int32 TerrainX = 0;
	int32 TerrainY = 0;
	WorldToIndex(WorldX, WorldY, TerrainX, TerrainY);
	return GetPoint(TerrainX, TerrainY);
}

float USolidTerrainMap::SampleHeight(float WorldX, float WorldY) const
{
	if (!IsBuilt())
	{
		return 0.f;
	}

	const float FX = (WorldX - WorldMinCm.X) / PointSpacing;
	const float FY = (WorldY - WorldMinCm.Y) / PointSpacing;
	const int32 TerrainX0 = FMath::Clamp(FMath::FloorToInt(FX), 0, GridWidth - 1);
	const int32 TerrainY0 = FMath::Clamp(FMath::FloorToInt(FY), 0, GridHeight - 1);
	const int32 TerrainX1 = FMath::Min(TerrainX0 + 1, GridWidth - 1);
	const int32 TerrainY1 = FMath::Min(TerrainY0 + 1, GridHeight - 1);
	const float Tx = FMath::Clamp(FX - static_cast<float>(TerrainX0), 0.f, 1.f);
	const float Ty = FMath::Clamp(FY - static_cast<float>(TerrainY0), 0.f, 1.f);

	const float H00 = GetPoint(TerrainX0, TerrainY0).Height;
	const float H10 = GetPoint(TerrainX1, TerrainY0).Height;
	const float H01 = GetPoint(TerrainX0, TerrainY1).Height;
	const float H11 = GetPoint(TerrainX1, TerrainY1).Height;
	return FMath::Lerp(FMath::Lerp(H00, H10, Tx), FMath::Lerp(H01, H11, Tx), Ty);
}

FSolidTerrainPoint USolidTerrainMap::SamplePoint(float WorldX, float WorldY) const
{
	FSolidTerrainPoint Result;
	Result.WorldX = WorldX;
	Result.WorldY = WorldY;

	if (!IsBuilt())
	{
		return Result;
	}

	const float FX = (WorldX - WorldMinCm.X) / PointSpacing;
	const float FY = (WorldY - WorldMinCm.Y) / PointSpacing;
	const int32 TerrainX0 = FMath::Clamp(FMath::FloorToInt(FX), 0, GridWidth - 1);
	const int32 TerrainY0 = FMath::Clamp(FMath::FloorToInt(FY), 0, GridHeight - 1);
	const int32 TerrainX1 = FMath::Min(TerrainX0 + 1, GridWidth - 1);
	const int32 TerrainY1 = FMath::Min(TerrainY0 + 1, GridHeight - 1);
	const float Tx = FMath::Clamp(FX - static_cast<float>(TerrainX0), 0.f, 1.f);
	const float Ty = FMath::Clamp(FY - static_cast<float>(TerrainY0), 0.f, 1.f);

	const FSolidTerrainPoint& P00 = GetPoint(TerrainX0, TerrainY0);
	const FSolidTerrainPoint& P10 = GetPoint(TerrainX1, TerrainY0);
	const FSolidTerrainPoint& P01 = GetPoint(TerrainX0, TerrainY1);
	const FSolidTerrainPoint& P11 = GetPoint(TerrainX1, TerrainY1);

	Result.Height = FMath::Lerp(FMath::Lerp(P00.Height, P10.Height, Tx), FMath::Lerp(P01.Height, P11.Height, Tx), Ty);
	Result.Threat = FMath::Lerp(FMath::Lerp(P00.Threat, P10.Threat, Tx), FMath::Lerp(P01.Threat, P11.Threat, Tx), Ty);
	Result.Fog = FMath::Lerp(FMath::Lerp(P00.Fog, P10.Fog, Tx), FMath::Lerp(P01.Fog, P11.Fog, Tx), Ty);

	// Biome: nearest neighbor (keeps enum discrete).
	Result.Biome = GetNearestPoint(WorldX, WorldY).Biome;
	return Result;
}

ESolidBiome USolidTerrainMap::ChooseBiome(
	float WorldX,
	float WorldY,
	float HeightNorm,
	int32 InSeed) const
{
	const float DistFromOrigin = FVector2D(WorldX, WorldY).Size();
	const float TownRadius = PointSpacing * 12.f; // ~small starter town footprint
	if (DistFromOrigin < TownRadius && HeightNorm < 0.55f)
	{
		return ESolidBiome::Town;
	}

	const float Moisture = SolidTerrainNoise::Fbm2D(
		WorldX * 0.00035f, WorldY * 0.00035f, InSeed + 44027, 4);
	const float Heat = SolidTerrainNoise::Fbm2D(
		WorldX * 0.00028f, WorldY * 0.00028f, InSeed + 99191, 3);

	if (HeightNorm > 0.72f)
	{
		return ESolidBiome::Mountain;
	}
	if (Moisture > 0.62f && HeightNorm < 0.45f)
	{
		return ESolidBiome::Swamp;
	}
	if (Heat > 0.68f && Moisture < 0.40f && HeightNorm < 0.55f)
	{
		return ESolidBiome::Desert;
	}
	if (Moisture > 0.48f && HeightNorm > 0.25f && HeightNorm < 0.70f)
	{
		return ESolidBiome::Forest;
	}
	return ESolidBiome::Grassland;
}

void USolidTerrainMap::FillThreatAndFog(FSolidTerrainPoint& Point) const
{
	Point.Threat = 0.f;
	// Shroud the map. CenterExplorationFogOn opens the bands around the player.
	Point.Fog = 1.f;
}

void USolidTerrainMap::CenterExplorationFogOn(const float WorldX, const float WorldY)
{
	if (!IsBuilt())
	{
		return;
	}

	FogOriginWorldCm = FVector2D(WorldX, WorldY);
	for (FSolidTerrainPoint& Point : Points)
	{
		const float DistM = (FVector2D(Point.WorldX, Point.WorldY) - FogOriginWorldCm).Size() * 0.01f;
		Point.Fog = SolidTerrainFog::FogFromDistanceMeters(DistM);
	}
	TrackClearFogBounds();
}

void USolidTerrainMap::CopyExplorationFog(TArray<float>& OutFog) const
{
	OutFog.Reset(Points.Num());
	for (const FSolidTerrainPoint& Point : Points)
	{
		OutFog.Add(Point.Fog);
	}
}

bool USolidTerrainMap::ApplyExplorationFog(const TArray<float>& InFog)
{
	if (!IsBuilt() || InFog.Num() != Points.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		Points[Index].Fog = InFog[Index];
	}
	TrackClearFogBounds();
	return true;
}

void USolidTerrainMap::ClearAllExplorationFog()
{
	for (FSolidTerrainPoint& Point : Points)
	{
		Point.Fog = 0.f;
	}
	TrackClearFogBounds();
}

int32 USolidTerrainMap::ApplyExplorationFogAround(
	float WorldX,
	float WorldY,
	bool bHavePrev,
	float PrevWorldX,
	float PrevWorldY,
	TArray<FVector2D>* OutChangedXY)
{
	if (!IsBuilt())
	{
		return 0;
	}

	const float ClearRadiusCm = SolidTerrainFog::MetersToCm(SolidTerrainFog::ClearRadiusMeters);
	const float HalfOuterCm = SolidTerrainFog::MetersToCm(SolidTerrainFog::FullFogStartMeters);
	const float ClearRadiusSq = ClearRadiusCm * ClearRadiusCm;
	const float HalfOuterSq = HalfOuterCm * HalfOuterCm;
	const float MoveCm = bHavePrev
		? FVector2D::Distance(FVector2D(WorldX, WorldY), FVector2D(PrevWorldX, PrevWorldY))
		: HalfOuterCm;
	const bool bFullDisk = !bHavePrev || MoveCm > ClearRadiusCm;

	auto WritePoint = [&](FSolidTerrainPoint& Point, float DistSq) -> bool
	{
		const float OldFog = Point.Fog;
		if (DistSq <= ClearRadiusSq)
		{
			Point.Fog = 0.f;
		}
		else if (DistSq <= HalfOuterSq)
		{
			Point.Fog = FMath::Min(Point.Fog, 0.5f);
		}
		else
		{
			return false;
		}
		if (FMath::IsNearlyEqual(OldFog, Point.Fog))
		{
			return false;
		}
		if (OutChangedXY)
		{
			OutChangedXY->Add(FVector2D(Point.WorldX, Point.WorldY));
		}
		return true;
	};

	auto AlreadyCovered = [&](float DistSq, float PrevDistSq) -> bool
	{
		if (DistSq <= ClearRadiusSq)
		{
			return PrevDistSq <= ClearRadiusSq;
		}
		if (DistSq <= HalfOuterSq)
		{
			return PrevDistSq <= HalfOuterSq;
		}
		return true;
	};

	int32 Changed = 0;
	auto VisitBox = [&](float MinX, float MaxX, float MinY, float MaxY, bool bRingsOnly, float InnerSq, float OuterSq)
	{
		const int32 TerrainX0 = FMath::Clamp(
			FMath::FloorToInt((MinX - WorldMinCm.X) / PointSpacing), 0, GridWidth - 1);
		const int32 TerrainX1 = FMath::Clamp(
			FMath::CeilToInt((MaxX - WorldMinCm.X) / PointSpacing), 0, GridWidth - 1);
		const int32 TerrainY0 = FMath::Clamp(
			FMath::FloorToInt((MinY - WorldMinCm.Y) / PointSpacing), 0, GridHeight - 1);
		const int32 TerrainY1 = FMath::Clamp(
			FMath::CeilToInt((MaxY - WorldMinCm.Y) / PointSpacing), 0, GridHeight - 1);
		for (int32 TerrainY = TerrainY0; TerrainY <= TerrainY1; ++TerrainY)
		{
			for (int32 TerrainX = TerrainX0; TerrainX <= TerrainX1; ++TerrainX)
			{
				FSolidTerrainPoint& Point = Points[TerrainY * GridWidth + TerrainX];
				const float DistSq = FVector2D::DistSquared(FVector2D(Point.WorldX, Point.WorldY), FVector2D(WorldX, WorldY));
				if (bRingsOnly && (DistSq < InnerSq || DistSq > OuterSq))
				{
					continue;
				}
				if (bHavePrev)
				{
					const float PrevDistSq = FVector2D::DistSquared(
						FVector2D(Point.WorldX, Point.WorldY), FVector2D(PrevWorldX, PrevWorldY));
					if (AlreadyCovered(DistSq, PrevDistSq))
					{
						continue;
					}
				}
				if (WritePoint(Point, DistSq))
				{
					++Changed;
				}
			}
		}
	};

	if (bFullDisk)
	{
		VisitBox(
			WorldX - HalfOuterCm, WorldX + HalfOuterCm,
			WorldY - HalfOuterCm, WorldY + HalfOuterCm,
			false, 0.f, HalfOuterSq);
	}
	else
	{
		const float Band = MoveCm + PointSpacing;
		auto VisitRing = [&](float RadiusCm)
		{
			const float Inner = FMath::Max(RadiusCm - Band, 0.f);
			const float Outer = RadiusCm + Band;
			VisitBox(
				WorldX - Outer, WorldX + Outer,
				WorldY - Outer, WorldY + Outer,
				true, Inner * Inner, Outer * Outer);
		};
		VisitRing(ClearRadiusCm);
		VisitRing(HalfOuterCm);
	}

	TrackClearFogBounds();
	return Changed;
}

bool USolidTerrainMap::GetClearFogBounds(FVector2D& OutMin, FVector2D& OutMax) const
{
	OutMin = ClearFogMinWorldCm;
	OutMax = ClearFogMaxWorldCm;
	return bHasClearFog;
}

void USolidTerrainMap::TrackClearFogBounds()
{
	bHasClearFog = false;
	float MinX = 0.f;
	float MaxX = 0.f;
	float MinY = 0.f;
	float MaxY = 0.f;
	for (const FSolidTerrainPoint& Point : Points)
	{
		if (Point.Fog != 0.f)
		{
			continue;
		}
		if (!bHasClearFog)
		{
			MinX = MaxX = Point.WorldX;
			MinY = MaxY = Point.WorldY;
			bHasClearFog = true;
			continue;
		}
		MinX = FMath::Min(MinX, Point.WorldX);
		MaxX = FMath::Max(MaxX, Point.WorldX);
		MinY = FMath::Min(MinY, Point.WorldY);
		MaxY = FMath::Max(MaxY, Point.WorldY);
	}
	ClearFogMinWorldCm = FVector2D(MinX, MinY);
	ClearFogMaxWorldCm = FVector2D(MaxX, MaxY);
}
