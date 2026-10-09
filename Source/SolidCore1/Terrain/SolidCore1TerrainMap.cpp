#include "SolidCore1TerrainMap.h"
#include "SolidCore1TerrainNoise.h"
#include "SolidCore1.h"

void USolidCore1TerrainMap::Build(
	int32 InSeed,
	float FrequencyScale,
	float Amplitude,
	float BaseHeight,
	int32 InGridWidth,
	int32 InGridHeight,
	float InPointSpacing,
	bool bForceRebuild)
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
	OriginXY = FVector2D(
		-0.5f * static_cast<float>(GridWidth - 1) * PointSpacing,
		-0.5f * static_cast<float>(GridHeight - 1) * PointSpacing);

	Points.SetNum(GridWidth * GridHeight);

	const float SafeAmp = FMath::Max(Amplitude, 1.f);
	for (int32 IY = 0; IY < GridHeight; ++IY)
	{
		for (int32 IX = 0; IX < GridWidth; ++IX)
		{
			FSolidCore1TerrainPoint& Point = Points[IY * GridWidth + IX];
			Point.X = OriginXY.X + static_cast<float>(IX) * PointSpacing;
			Point.Y = OriginXY.Y + static_cast<float>(IY) * PointSpacing;
			Point.Height = SolidCore1TerrainNoise::SampleHeight(
				Point.X, Point.Y, Seed, FrequencyScale, Amplitude, BaseHeight);

			const float HeightNorm = FMath::Clamp((Point.Height - BaseHeight) / SafeAmp, 0.f, 1.f);
			Point.Biome = ChooseBiome(Point.X, Point.Y, HeightNorm, Seed);
			FillThreatAndFog(Point, HeightNorm, Seed);
		}
	}

	bIsBuilt = true;
	UE_LOG(LogSolidCore1, Warning,
		TEXT("TerrainMap built: %dx%d spacing=%.0f cm seed=%d origin=(%.0f,%.0f) extent=(%.0f,%.0f)"),
		GridWidth, GridHeight, PointSpacing, Seed,
		OriginXY.X, OriginXY.Y, GetWorldMaxXY().X, GetWorldMaxXY().Y);
}

void USolidCore1TerrainMap::WorldToIndex(float WorldX, float WorldY, int32& OutX, int32& OutY) const
{
	const float FX = (WorldX - OriginXY.X) / PointSpacing;
	const float FY = (WorldY - OriginXY.Y) / PointSpacing;
	OutX = FMath::Clamp(FMath::RoundToInt(FX), 0, GridWidth - 1);
	OutY = FMath::Clamp(FMath::RoundToInt(FY), 0, GridHeight - 1);
}

const FSolidCore1TerrainPoint& USolidCore1TerrainMap::GetPoint(int32 IndexX, int32 IndexY) const
{
	check(Points.Num() > 0);
	IndexX = FMath::Clamp(IndexX, 0, GridWidth - 1);
	IndexY = FMath::Clamp(IndexY, 0, GridHeight - 1);
	return Points[IndexY * GridWidth + IndexX];
}

const FSolidCore1TerrainPoint& USolidCore1TerrainMap::GetNearestPoint(float WorldX, float WorldY) const
{
	int32 IX = 0;
	int32 IY = 0;
	WorldToIndex(WorldX, WorldY, IX, IY);
	return GetPoint(IX, IY);
}

float USolidCore1TerrainMap::SampleHeight(float WorldX, float WorldY) const
{
	if (!IsBuilt())
	{
		return 0.f;
	}

	const float FX = (WorldX - OriginXY.X) / PointSpacing;
	const float FY = (WorldY - OriginXY.Y) / PointSpacing;
	const int32 X0 = FMath::Clamp(FMath::FloorToInt(FX), 0, GridWidth - 1);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(FY), 0, GridHeight - 1);
	const int32 X1 = FMath::Min(X0 + 1, GridWidth - 1);
	const int32 Y1 = FMath::Min(Y0 + 1, GridHeight - 1);
	const float Tx = FMath::Clamp(FX - static_cast<float>(X0), 0.f, 1.f);
	const float Ty = FMath::Clamp(FY - static_cast<float>(Y0), 0.f, 1.f);

	const float H00 = GetPoint(X0, Y0).Height;
	const float H10 = GetPoint(X1, Y0).Height;
	const float H01 = GetPoint(X0, Y1).Height;
	const float H11 = GetPoint(X1, Y1).Height;
	return FMath::Lerp(FMath::Lerp(H00, H10, Tx), FMath::Lerp(H01, H11, Tx), Ty);
}

FSolidCore1TerrainPoint USolidCore1TerrainMap::SamplePoint(float WorldX, float WorldY) const
{
	FSolidCore1TerrainPoint Result;
	Result.X = WorldX;
	Result.Y = WorldY;

	if (!IsBuilt())
	{
		return Result;
	}

	const float FX = (WorldX - OriginXY.X) / PointSpacing;
	const float FY = (WorldY - OriginXY.Y) / PointSpacing;
	const int32 X0 = FMath::Clamp(FMath::FloorToInt(FX), 0, GridWidth - 1);
	const int32 Y0 = FMath::Clamp(FMath::FloorToInt(FY), 0, GridHeight - 1);
	const int32 X1 = FMath::Min(X0 + 1, GridWidth - 1);
	const int32 Y1 = FMath::Min(Y0 + 1, GridHeight - 1);
	const float Tx = FMath::Clamp(FX - static_cast<float>(X0), 0.f, 1.f);
	const float Ty = FMath::Clamp(FY - static_cast<float>(Y0), 0.f, 1.f);

	const FSolidCore1TerrainPoint& P00 = GetPoint(X0, Y0);
	const FSolidCore1TerrainPoint& P10 = GetPoint(X1, Y0);
	const FSolidCore1TerrainPoint& P01 = GetPoint(X0, Y1);
	const FSolidCore1TerrainPoint& P11 = GetPoint(X1, Y1);

	Result.Height = FMath::Lerp(FMath::Lerp(P00.Height, P10.Height, Tx), FMath::Lerp(P01.Height, P11.Height, Tx), Ty);
	Result.Threat = FMath::Lerp(FMath::Lerp(P00.Threat, P10.Threat, Tx), FMath::Lerp(P01.Threat, P11.Threat, Tx), Ty);
	Result.Fog = FMath::Lerp(FMath::Lerp(P00.Fog, P10.Fog, Tx), FMath::Lerp(P01.Fog, P11.Fog, Tx), Ty);

	// Biome: nearest neighbor (keeps enum discrete).
	Result.Biome = GetNearestPoint(WorldX, WorldY).Biome;
	return Result;
}

ESolidCore1Biome USolidCore1TerrainMap::ChooseBiome(
	float WorldX,
	float WorldY,
	float HeightNorm,
	int32 InSeed) const
{
	const float DistFromOrigin = FVector2D(WorldX, WorldY).Size();
	const float TownRadius = PointSpacing * 12.f; // ~small starter town footprint
	if (DistFromOrigin < TownRadius && HeightNorm < 0.55f)
	{
		return ESolidCore1Biome::Town;
	}

	const float Moisture = SolidCore1TerrainNoise::Fbm2D(
		WorldX * 0.00035f, WorldY * 0.00035f, InSeed + 44027, 4);
	const float Heat = SolidCore1TerrainNoise::Fbm2D(
		WorldX * 0.00028f, WorldY * 0.00028f, InSeed + 99191, 3);

	if (HeightNorm > 0.72f)
	{
		return ESolidCore1Biome::Mountain;
	}
	if (Moisture > 0.62f && HeightNorm < 0.45f)
	{
		return ESolidCore1Biome::Swamp;
	}
	if (Heat > 0.68f && Moisture < 0.40f && HeightNorm < 0.55f)
	{
		return ESolidCore1Biome::Desert;
	}
	if (Moisture > 0.48f && HeightNorm > 0.25f && HeightNorm < 0.70f)
	{
		return ESolidCore1Biome::Forest;
	}
	return ESolidCore1Biome::Grassland;
}

void USolidCore1TerrainMap::FillThreatAndFog(
	FSolidCore1TerrainPoint& Point,
	float HeightNorm,
	int32 InSeed) const
{
	float ThreatBase = 0.2f;
	float FogBase = 0.55f;

	switch (Point.Biome)
	{
	case ESolidCore1Biome::Town:
		ThreatBase = 0.05f;
		FogBase = 0.0f; // starter area revealed
		break;
	case ESolidCore1Biome::Grassland:
		ThreatBase = 0.20f;
		FogBase = 0.45f;
		break;
	case ESolidCore1Biome::Forest:
		ThreatBase = 0.45f;
		FogBase = 0.70f;
		break;
	case ESolidCore1Biome::Mountain:
		ThreatBase = 0.65f;
		FogBase = 0.50f;
		break;
	case ESolidCore1Biome::Desert:
		ThreatBase = 0.40f;
		FogBase = 0.35f;
		break;
	case ESolidCore1Biome::Swamp:
		ThreatBase = 0.55f;
		FogBase = 0.85f;
		break;
	default:
		break;
	}

	const float Jitter = SolidCore1TerrainNoise::HashToFloat(
		FMath::FloorToInt(Point.X * 0.01f),
		FMath::FloorToInt(Point.Y * 0.01f),
		InSeed + 3037);

	Point.Threat = FMath::Clamp(ThreatBase + (Jitter - 0.5f) * 0.15f + HeightNorm * 0.1f, 0.f, 1.f);
	Point.Fog = FMath::Clamp(FogBase + (1.f - Jitter) * 0.1f, 0.f, 1.f);

	// Keep the immediate town clear.
	if (Point.Biome == ESolidCore1Biome::Town)
	{
		Point.Threat = FMath::Min(Point.Threat, 0.08f);
		Point.Fog = 0.f;
	}
}
