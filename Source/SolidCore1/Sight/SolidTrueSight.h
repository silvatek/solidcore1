#pragma once

#include "CoreMinimal.h"

namespace SolidSight
{
	/** Eye height above the capsule center (cm). */
	inline constexpr float TrueSightEyeHeightCm = 64.f;

	/** Camera sits this far along the look direction so it clears the near clip (cm). */
	inline constexpr float TrueSightForwardCm = 12.f;
}
