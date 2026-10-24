#pragma once

#include "SolidEncounter.h"

/**
 * The shipped encounters.
 * Add another FStaticEncounter to StaticEncounters, the way SolidEvents::Definitions lists events.
 */
namespace SolidEncounter
{
	inline const FStaticEncounter StaticEncounters[] = {
		// Three Small raptors. A chart mark supplies the focus. No event, so startup places them.
		FStaticEncounter{ TEXT("Iglin Bridge") }
			.WithGroup(EMonsterKind::Raptor, ESolidMonsterSize::Small, 3, TEXT("Attack")),
	};

	/** The catalog row of this name, ignoring case. Null when the catalog has none. */
	inline const FStaticEncounter* FindByName(const FString& Name)
	{
		const FString Want = Name.TrimStartAndEnd();
		for (const FStaticEncounter& Row : StaticEncounters)
		{
			if (Want.Equals(Row.Name, ESearchCase::IgnoreCase))
			{
				return &Row;
			}
		}
		return nullptr;
	}

	static_assert(UE_ARRAY_COUNT(StaticEncounters) <= MaxEncounters, "Raise SolidEncounter::MaxEncounters");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(StaticEncounters);
}
