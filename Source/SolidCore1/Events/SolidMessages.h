#pragma once

#include "CoreMinimal.h"

/**
 * Lines an event can show. Each message has a stable id.
 * A ShowMessage change and a MessageClosed trigger name that id.
 */
namespace SolidMessages
{
	inline constexpr int32 MaxMessages = 16;
	inline constexpr int32 IdLength = 8;

	/** Stable ids. Eight hex digits. Event changes and triggers name these. */
	inline constexpr const TCHAR* IdKilledByMonsters = TEXT("7c2e9a41");
	inline constexpr const TCHAR* IdDrivenFromVillage = TEXT("4b91d0e6");

	struct FMessage
	{
		/** Eight hex digits. */
		const TCHAR* Id = TEXT("");
		const TCHAR* Text = TEXT("");
	};

	inline const FMessage Definitions[] = {
		{
			IdKilledByMonsters,
			TEXT("You are killed by monsters"),
		},
		{
			IdDrivenFromVillage,
			TEXT("You are driven from the village"),
		},
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxMessages, "Raise SolidMessages::MaxMessages");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const FMessage* FindById(const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return nullptr;
		}
		for (int32 Index = 0; Index < DefinitionCount; ++Index)
		{
			if (FCString::Strcmp(Definitions[Index].Id, Id) == 0)
			{
				return &Definitions[Index];
			}
		}
		return nullptr;
	}

	inline const TCHAR* TextFor(const TCHAR* Id)
	{
		const FMessage* Message = FindById(Id);
		return Message ? Message->Text : nullptr;
	}
}
