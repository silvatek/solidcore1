#pragma once

#include "SolidRoad.h"
#include "SolidStronghold.h"
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

	/** A stronghold mark. The chart places it. */
	inline FSolidWorldChartMark StrongholdMark(const ESolidStrongholdSize Size, const ESolidStrongholdType Type)
	{
		FSolidWorldChartMark Mark;
		Mark.Kind = ESolidWorldChartMarkKind::Stronghold;
		Mark.StrongholdSize = Size;
		Mark.StrongholdType = Type;
		Mark.Name = SolidStronghold::Label(Size, Type);
		return Mark;
	}

	inline const FRoad Roads[] = {
		{ StrongholdMark(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon), TownMark(SolidTowns::Iglin), EType::Trail },
		{ TownMark(SolidTowns::Iglin), TownMark(SolidTowns::Relion), EType::Track },
		{ TownMark(SolidTowns::Relion), TownMark(SolidTowns::Kanfold), EType::Highway },
		{ TownMark(SolidTowns::Relion), TownMark(SolidTowns::Dragongate), EType::Road },
		{ TownMark(SolidTowns::Dragongate), TownMark(SolidTowns::Jethan), EType::Track },
		{ TownMark(SolidTowns::Dragongate), TownMark(SolidTowns::Rathen), EType::Track },
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Roads);
}
