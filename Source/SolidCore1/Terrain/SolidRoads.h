#pragma once

#include "SolidRoad.h"
#include "SolidWorldChart.h"

/**
 * The shipped roads. Each one joins two marks on the world chart.
 */
namespace SolidRoad
{
	struct FRoad
	{
		FSolidWorldChartMark From;
		FSolidWorldChartMark To;
		EType Type = EType::Trail;
	};

	inline constexpr int32 DefinitionCount = 0;
}
