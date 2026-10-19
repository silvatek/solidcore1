#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"

/** The open-world map, and the URL that asks for a closed visit on it. */
namespace SolidTravel
{
	inline const TCHAR* WorldMapPath = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");
	inline const TCHAR* OptionKey = TEXT("SolidLevel");
	inline const TCHAR* StrongholdValue = TEXT("Stronghold");

	/** True when the travel URL's SolidLevel value is Expected. A later ? is not part of the value. */
	inline bool OptionEquals(const FString& Options, const TCHAR* Expected)
	{
		FString Value;
		if (!FParse::Value(*Options, *FString::Printf(TEXT("%s="), OptionKey), Value))
		{
			return false;
		}
		int32 Query = INDEX_NONE;
		if (Value.FindChar(TEXT('?'), Query))
		{
			Value.LeftInline(Query);
		}
		return Value.Equals(Expected, ESearchCase::IgnoreCase);
	}

	inline FString EnterStrongholdOptions()
	{
		return FString::Printf(TEXT("?%s=%s"), OptionKey, StrongholdValue);
	}

	/** Empty options load the open world. */
	inline FString LeaveOptions()
	{
		return FString();
	}

	/** True when a travel URL asked for the company stronghold interior. */
	inline bool RequestsStronghold(const FString& Options)
	{
		return OptionEquals(Options, StrongholdValue);
	}
}
