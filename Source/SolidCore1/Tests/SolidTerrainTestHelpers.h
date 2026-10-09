#pragma once

#include "SolidTerrainMap.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Shared fixtures for SolidCore1.Map.* / Fog map-backed automation tests.
 * Keep identical Build args so Map and MapMore suites stay comparable.
 */
namespace SolidTerrainTestHelpers
{
	/** ~128m half-extent so the grid includes full-fog cells beyond 50m. */
	inline USolidTerrainMap* MakeSmallMap()
	{
		USolidTerrainMap* Map = NewObject<USolidTerrainMap>();
		Map->Build(
			/*InSeed=*/1337,
			/*FrequencyScale=*/0.00012f,
			/*Amplitude=*/3000.f,
			/*BaseHeight=*/0.f,
			/*InGridWidth=*/65,
			/*InGridHeight=*/65,
			/*InPointSpacing=*/200.f,
			/*bForceRebuild=*/true);
		return Map;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
