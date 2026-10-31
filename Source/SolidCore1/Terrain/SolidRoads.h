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
	inline const FSolidRoad Roads[] = {
		FSolidRoad(StrongholdMark(ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon), TownMark(SolidTowns::Iglin), EType::Trail),
		FSolidRoad(TownMark(SolidTowns::Iglin), BridgeMark(TEXT("Iglin Bridge")), EType::Track),
		FSolidRoad(BridgeMark(TEXT("Iglin Bridge")), TownMark(SolidTowns::Relion), EType::Track),
		FSolidRoad(TownMark(SolidTowns::Relion), TownMark(SolidTowns::Kanfold), EType::Highway),
		FSolidRoad(TownMark(SolidTowns::Relion), TownMark(SolidTowns::Dragongate), EType::Road),
		FSolidRoad(TownMark(SolidTowns::Dragongate), TownMark(SolidTowns::Jethan), EType::Track),
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Roads);

	/** Write the stronghold cell onto this end when the loaded marks name it. */
	void LocateStrongholdEnd(const TMap<int32, FSolidWorldBang>& Marks, FEndpoint& End);
}
