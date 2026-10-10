#pragma once

#include "CoreMinimal.h"

/**
 * Camera boom limit against exploration fog.
 * Fog amount itself stays on the pawn trail. This only shortens the boom so the
 * camera XY is not over a terrain point with Fog > 0.
 */
namespace SolidCameraFog
{
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
