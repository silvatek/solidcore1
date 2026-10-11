#pragma once

#include "CoreMinimal.h"

/**
 * Company bases. The cave is the first. Later upgrades append after Cave;
 * only the cave is placed on the world map today.
 */
enum class ESolidStronghold : uint8
{
	Cave,
};

namespace SolidStronghold
{
	struct FInfo
	{
		ESolidStronghold Kind = ESolidStronghold::Cave;
		const TCHAR* Name = TEXT("");
		/** WorldMap.txt marker. The cave is `C`. */
		TCHAR Marker = TEXT('C');
	};

	/** The company starts here. Later kinds replace it. */
	inline constexpr ESolidStronghold First = ESolidStronghold::Cave;

	inline const FInfo Definitions[] = {
		{ ESolidStronghold::Cave, TEXT("Cave"), TEXT('C') },
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const FInfo* Find(ESolidStronghold Kind)
	{
		for (const FInfo& Info : Definitions)
		{
			if (Info.Kind == Kind)
			{
				return &Info;
			}
		}
		return nullptr;
	}

	inline const TCHAR* NameFor(ESolidStronghold Kind)
	{
		if (const FInfo* Info = Find(Kind))
		{
			return Info->Name;
		}
		return TEXT("");
	}

	inline TCHAR MarkerFor(ESolidStronghold Kind)
	{
		if (const FInfo* Info = Find(Kind))
		{
			return Info->Marker;
		}
		return 0;
	}

	inline bool IsStrongholdMarker(TCHAR Marker)
	{
		const TCHAR Upper = FChar::ToUpper(Marker);
		for (const FInfo& Info : Definitions)
		{
			if (Info.Marker == Upper)
			{
				return true;
			}
		}
		return false;
	}
}
