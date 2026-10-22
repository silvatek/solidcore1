#pragma once

#include "Monster/SolidEncounter.h"

/**
 * The shipped encounters.
 * Add another FDefinition to Definitions, the way SolidEvents::Definitions lists events.
 */
namespace SolidEncounter
{
	inline const FDefinition Definitions[] = {
		// Three Small raptors beside the Iglin ramp.
		FDefinition{ TEXT("Iglin Bridge"), ESite::IglinBridgeRamp }
			.WithGroup(EMonsterKind::Raptor, ESolidMonsterSize::Small, 3),
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEncounters, "Raise SolidEncounter::MaxEncounters");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);
}
