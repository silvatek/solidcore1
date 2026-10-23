#pragma once

#include "Monster/SolidEncounter.h"

/**
 * The shipped encounters.
 * Add another FDefinition to Definitions, the way SolidEvents::Definitions lists events.
 */
namespace SolidEncounter
{
	inline const FDefinition Definitions[] = {
		// Three Small raptors. A chart mark supplies the focus.
		FDefinition{ TEXT("Iglin Bridge") }
			.WithGroup(EMonsterKind::Raptor, ESolidMonsterSize::Small, 3, TEXT("Attack")),
	};

	/** The catalog row of this name, ignoring case. Null when the catalog has none. */
	inline const FDefinition* FindByName(const FString& Name)
	{
		const FString Want = Name.TrimStartAndEnd();
		for (const FDefinition& Def : Definitions)
		{
			if (Want.Equals(Def.Name, ESearchCase::IgnoreCase))
			{
				return &Def;
			}
		}
		return nullptr;
	}

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEncounters, "Raise SolidEncounter::MaxEncounters");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);
}
