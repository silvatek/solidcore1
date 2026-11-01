#include "SolidSight.h"
#include "SolidCharacter.h"
#include "Terrain/SolidTerrainMap.h"

float SolidSight::CameraGroundFog(const USolidTerrainMap& Map, const float WorldX, const float WorldY)
{
	if (!Map.IsBuilt() || Map.PointSpacing <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}

	const int32 Width = Map.GridWidth;
	const int32 Height = Map.GridHeight;
	const int32 TerrainX = FMath::Clamp(
		FMath::RoundToInt((WorldX - Map.WorldMinCm.X) / Map.PointSpacing), 0, Width - 1);
	const int32 TerrainY = FMath::Clamp(
		FMath::RoundToInt((WorldY - Map.WorldMinCm.Y) / Map.PointSpacing), 0, Height - 1);

	float Fog = 0.f;
	for (int32 OffsetY = -1; OffsetY <= 1; ++OffsetY)
	{
		const int32 NeighbourTerrainY = TerrainY + OffsetY;
		if (NeighbourTerrainY < 0 || NeighbourTerrainY >= Height)
		{
			continue;
		}
		for (int32 OffsetX = -1; OffsetX <= 1; ++OffsetX)
		{
			const int32 NeighbourTerrainX = TerrainX + OffsetX;
			if (NeighbourTerrainX < 0 || NeighbourTerrainX >= Width)
			{
				continue;
			}
			Fog = FMath::Max(Fog, Map.GetPoint(NeighbourTerrainX, NeighbourTerrainY).Fog);
		}
	}
	return Fog;
}

void ASolidCharacter::UpdateSquadCameraFraming(float DeltaTime)
{
	if (!IsEagleSight())
	{
		HideEagleSight();
	}

	if (IsTrueSight())
	{
		ApplyTrueSightCamera();
		return;
	}

	if (IsFalconSight())
	{
		AimFalconAtCaptain();
		return;
	}

	if (IsEagleSight())
	{
		ApplyEagleSightCamera(DeltaTime);
		return;
	}

	FrameRavenSquad(DeltaTime);
}
