#pragma once

#include "CoreMinimal.h"

/** What the company base is. Dungeon is the first; Fortress and Tower come later. */
enum class ESolidStrongholdType : uint8
{
	Dungeon,
	Fortress,
	Tower,
};

/**
 * How grand the company base is.
 * Meagre is the start. Later upgrades move up this list.
 */
enum class ESolidStrongholdSize : uint8
{
	Meagre,
	Humble,
	Average,
	Impressive,
	Grand,
	Stately,
	Imperious,
};

/** A company base: one type and one size. */
struct FSolidStronghold
{
	ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
};

namespace SolidStronghold
{
	/** A stronghold that has a WorldMap.txt cell. Only the starting cave is placed today. */
	struct FOnMap
	{
		TCHAR Marker = TEXT('C');
		FSolidStronghold Attributes;
	};

	inline const ESolidStrongholdType Types[] = {
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdType::Fortress,
		ESolidStrongholdType::Tower,
	};

	inline const ESolidStrongholdSize Sizes[] = {
		ESolidStrongholdSize::Meagre,
		ESolidStrongholdSize::Humble,
		ESolidStrongholdSize::Average,
		ESolidStrongholdSize::Impressive,
		ESolidStrongholdSize::Grand,
		ESolidStrongholdSize::Stately,
		ESolidStrongholdSize::Imperious,
	};

	inline constexpr int32 TypeCount = UE_ARRAY_COUNT(Types);
	inline constexpr int32 SizeCount = UE_ARRAY_COUNT(Sizes);

	/** The company starts here: a Meagre Dungeon on the `C` cell. */
	inline const FOnMap Starting = { TEXT('C'), { ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre } };

	inline const FOnMap OnMap[] = {
		Starting,
	};

	inline constexpr int32 OnMapCount = UE_ARRAY_COUNT(OnMap);

	inline const TCHAR* TypeName(ESolidStrongholdType Type)
	{
		switch (Type)
		{
		case ESolidStrongholdType::Dungeon: return TEXT("Dungeon");
		case ESolidStrongholdType::Fortress: return TEXT("Fortress");
		case ESolidStrongholdType::Tower: return TEXT("Tower");
		default: return TEXT("");
		}
	}

	inline const TCHAR* SizeName(ESolidStrongholdSize Size)
	{
		switch (Size)
		{
		case ESolidStrongholdSize::Meagre: return TEXT("Meagre");
		case ESolidStrongholdSize::Humble: return TEXT("Humble");
		case ESolidStrongholdSize::Average: return TEXT("Average");
		case ESolidStrongholdSize::Impressive: return TEXT("Impressive");
		case ESolidStrongholdSize::Grand: return TEXT("Grand");
		case ESolidStrongholdSize::Stately: return TEXT("Stately");
		case ESolidStrongholdSize::Imperious: return TEXT("Imperious");
		default: return TEXT("");
		}
	}

	/** "Meagre Dungeon". */
	inline FString Label(const FSolidStronghold& Stronghold)
	{
		return FString::Printf(TEXT("%s %s"), SizeName(Stronghold.Size), TypeName(Stronghold.Type));
	}

	inline int32 IndexOfType(ESolidStrongholdType Type)
	{
		for (int32 Index = 0; Index < TypeCount; ++Index)
		{
			if (Types[Index] == Type)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	inline int32 IndexOfSize(ESolidStrongholdSize Size)
	{
		for (int32 Index = 0; Index < SizeCount; ++Index)
		{
			if (Sizes[Index] == Size)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	inline const FOnMap* FindOnMap(TCHAR Marker)
	{
		const TCHAR Upper = FChar::ToUpper(Marker);
		for (const FOnMap& Placed : OnMap)
		{
			if (Placed.Marker == Upper)
			{
				return &Placed;
			}
		}
		return nullptr;
	}

	inline bool IsStrongholdMarker(TCHAR Marker)
	{
		return FindOnMap(Marker) != nullptr;
	}

	inline bool AttributesForMarker(TCHAR Marker, FSolidStronghold& OutAttributes)
	{
		if (const FOnMap* Placed = FindOnMap(Marker))
		{
			OutAttributes = Placed->Attributes;
			return true;
		}
		return false;
	}
}
