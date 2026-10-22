#pragma once

#include "Monster/SolidEncounter.h"

/**
 * The shipped encounters.
 * Add a factory result to Definitions, the way SolidEvents::Definitions lists events.
 */
namespace SolidEncounter
{
	inline const FDefinition Definitions[] = {
		IglinBridge(),
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEncounters, "Raise SolidEncounter::MaxEncounters");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);
}
