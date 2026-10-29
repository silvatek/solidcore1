#pragma once

#include "CoreMinimal.h"
#include "Terrain/SolidTerrainMap.h"

/**
 * Camera position against exploration fog.
 * Fog amount itself stays on the pawn trail. A camera may stand on a terrain
 * point only when that point and each neighbour have Fog of 0.
 */
namespace SolidCameraFog
{
	/**
	 * Fog that blocks a camera at this world XY.
	 * The nearest terrain point and each neighbour that exists. 0 only when every
	 * one of those is 0. Missing map is blocked.
	 */
	inline float CameraGroundFog(const USolidTerrainMap& Map, const float WorldX, const float WorldY)
	{
		if (!Map.IsBuilt() || Map.PointSpacing <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}

		const int32 Width = Map.GridWidth;
		const int32 Height = Map.GridHeight;
		const int32 X = FMath::Clamp(
			FMath::RoundToInt((WorldX - Map.OriginXY.X) / Map.PointSpacing), 0, Width - 1);
		const int32 Y = FMath::Clamp(
			FMath::RoundToInt((WorldY - Map.OriginXY.Y) / Map.PointSpacing), 0, Height - 1);

		float Fog = 0.f;
		for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
		{
			const int32 NeighbourY = Y + OffsetY;
			if (NeighbourY < 0 || NeighbourY >= Height)
			{
				continue;
			}
			for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
			{
				const int32 NeighbourX = X + OffsetX;
				if (NeighbourX < 0 || NeighbourX >= Width)
				{
					continue;
				}
				Fog = FMath::Max(Fog, Map.GetPoint(NeighbourX, NeighbourY).Fog);
			}
		}
		return Fog;
	}

	/**
	 * Largest scale in [0, 1] along a boom where FogAtScale is still clear.
	 * FogAtScale(0) is the boom anchor. FogAtScale(1) is the desired camera.
	 * Returns 1 when the desired camera is already clear, or when the anchor is
	 * already over fog (do not collapse the boom).
	 */
	template <typename FogAtScaleFunc>
	float MaxClearScale(FogAtScaleFunc&& FogAtScale)
	{
		if (FogAtScale(1.f) <= 0.f)
		{
			return 1.f;
		}
		if (FogAtScale(0.f) > 0.f)
		{
			return 1.f;
		}

		float Lo = 0.f;
		float Hi = 1.f;
		for (int32 Step = 0; Step < 12; ++Step)
		{
			const float Mid = (Lo + Hi) * 0.5f;
			if (FogAtScale(Mid) > 0.f)
			{
				Hi = Mid;
			}
			else
			{
				Lo = Mid;
			}
		}
		return Lo;
	}
}
