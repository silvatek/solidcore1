#pragma once

#include "CoreMinimal.h"

class APlayerController;
class UWorld;

/** Yes / No after the player chooses Exit Game. */
namespace SolidExit
{
	inline constexpr int32 YesIndex = 0;
	inline constexpr int32 NoIndex = 1;
	inline constexpr int32 ChoiceCount = 2;

	inline const TCHAR* Prompt()
	{
		return TEXT("Exit the game?");
	}

	inline const TCHAR* Label(const int32 Index)
	{
		return Index == YesIndex ? TEXT("Yes") : TEXT("No");
	}

	struct FRequest
	{
		bool bQuitGame = false;
		bool bCloseEditor = false;
	};

	/** Yes. bInEditor is play in editor. bCanCloseEditor is whether that close can be requested. */
	inline FRequest RequestForYes(const bool bInEditor, const bool bCanCloseEditor)
	{
		FRequest Request;
		Request.bQuitGame = true;
		Request.bCloseEditor = bInEditor && bCanCloseEditor;
		return Request;
	}

	inline bool IsYes(const int32 Index)
	{
		return Index == YesIndex;
	}

	inline int32 WrapIndex(const int32 Index, const int32 Delta)
	{
		int32 Wrapped = (Index + Delta) % ChoiceCount;
		if (Wrapped < 0)
		{
			Wrapped += ChoiceCount;
		}
		return Wrapped;
	}

	/** True when this world is play in editor. */
	bool IsEditorPlay(const UWorld* World);

	/** True when the editor close can be requested. False in a non-editor build. */
	bool CanCloseEditor();

	/**
	 * End play, then close the editor when the request asks and the editor can close.
	 * A non-editor build does not compile the editor close.
	 */
	void Perform(const UObject* WorldContext, APlayerController* Player);
}
