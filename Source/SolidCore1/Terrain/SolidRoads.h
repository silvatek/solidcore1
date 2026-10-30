#pragma once

#include "SolidRoad.h"
#include "SolidWorldChart.h"
#include "Towns/SolidTowns.h"

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

	/** A town mark. The chart places it. */
	inline FSolidWorldChartMark TownMark(const int32 TownId)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::Town;
		Mark.Name = SolidTowns::Definitions[TownId].Name;
		return Mark;
	}

	inline const FRoad Roads[] = {
		{
			TownMark(SolidTowns::Relion),
			TownMark(SolidTowns::Kanfold),
			EType::Highway,
		},
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Roads);
}
