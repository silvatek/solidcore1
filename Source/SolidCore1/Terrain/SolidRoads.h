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
		{ TownMark(SolidTowns::Iglin), TownMark(SolidTowns::Relion), EType::Track },
		{ TownMark(SolidTowns::Relion), TownMark(SolidTowns::Kanfold), EType::Highway },
		{ TownMark(SolidTowns::Relion), TownMark(SolidTowns::Dragongate), EType::Road },
		{ TownMark(SolidTowns::Dragongate), TownMark(SolidTowns::Jethan), EType::Track },
		{ TownMark(SolidTowns::Dragongate), TownMark(SolidTowns::Rathen), EType::Track },
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Roads);
}
