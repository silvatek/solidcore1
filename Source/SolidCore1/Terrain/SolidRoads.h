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

	/** Copy the chart mark's position onto this end when the chart has that mark. */
	inline void LocateMark(const TArray<FSolidWorldChartMark>& ChartMarks, FSolidWorldChartMark& End)
	{
		for (const FSolidWorldChartMark& ChartMark : ChartMarks)
		{
			if (ChartMark.Kind != End.Kind)
			{
				continue;
			}
			if (End.Kind == ESolidWorldChartMarkKind::Stronghold)
			{
				if (ChartMark.StrongholdSize != End.StrongholdSize
					|| ChartMark.StrongholdType != End.StrongholdType)
				{
					continue;
				}
			}
			else
			{
				FString EndName = End.Name;
				FString ChartName = ChartMark.Name;
				EndName.ReplaceInline(TEXT(" "), TEXT(""));
				ChartName.ReplaceInline(TEXT(" "), TEXT(""));
				if (!EndName.Equals(ChartName, ESearchCase::IgnoreCase))
				{
					continue;
				}
			}
			End.PositionCm = ChartMark.PositionCm;
			return;
		}
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

	/** Town name, or the stronghold name, for this end. */
	FString ChartRoadEndName(const FEndpoint& End);

	/** Write the stronghold cell onto this end when the loaded marks name it. */
	void LocateStrongholdEnd(const TMap<int32, FSolidWorldBang>& Marks, FEndpoint& End);
}
