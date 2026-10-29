#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

/** Raven sight is the third-person squad camera. True sight is first person. */
enum class ESolidSight : uint8
{
	Raven,
	True,
	Falcon,
	Eagle,
};

/** F9 view toggle. Pure helpers — no actor spawning (safe for automation). */
namespace SolidSight
{
	/** Eye height above the capsule center (cm). */
	inline constexpr float TrueSightEyeHeightCm = 64.f;

	/** Camera sits this far along the look direction so it clears the near clip (cm). */
	inline constexpr float TrueSightForwardCm = 12.f;

	/** Right stick click. The same toggle as F9. */
	inline FKey ControllerToggleKey()
	{
		return EKeys::Gamepad_RightThumbstick;
	}

	inline bool IsTrueSight(ESolidSight Sight)
	{
		return Sight == ESolidSight::True;
	}

	inline ESolidSight Toggle(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? ESolidSight::Raven : ESolidSight::True;
	}

	/** Which sights F9 may land on. True sight starts available. */
	struct FAvailable
	{
		bool bTrue = true;
		bool bRaven = false;
		bool bFalcon = false;
		bool bEagle = false;
	};

	inline bool IsAvailable(const ESolidSight Sight, const FAvailable& Available)
	{
		switch (Sight)
		{
		case ESolidSight::True: return Available.bTrue;
		case ESolidSight::Raven: return Available.bRaven;
		case ESolidSight::Falcon: return Available.bFalcon;
		case ESolidSight::Eagle: return Available.bEagle;
		default: return false;
		}
	}

	/** The next mode in F9 order: true, raven, falcon, eagle. */
	inline ESolidSight Next(const ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::True: return ESolidSight::Raven;
		case ESolidSight::Raven: return ESolidSight::Falcon;
		case ESolidSight::Falcon: return ESolidSight::Eagle;
		case ESolidSight::Eagle: return ESolidSight::True;
		default: return ESolidSight::True;
		}
	}

	/**
	 * F9. Walks that order and skips a mode that is locked.
	 * Stays on the current mode when no other mode is available.
	 */
	inline ESolidSight Cycle(const ESolidSight Sight, const FAvailable& Available)
	{
		ESolidSight Candidate = Sight;
		for (int32 Step = 0; Step < 4; ++Step)
		{
			Candidate = Next(Candidate);
			if (IsAvailable(Candidate, Available))
			{
				return Candidate;
			}
		}
		return Sight;
	}

	/**
	 * Look used after a sight change. True sight is horizontal: pitch and roll are 0.
	 * Yaw stays, so the captain still faces the same way. Raven sight keeps the tilt.
	 */
	inline FRotator ViewForSight(const ESolidSight Sight, const FRotator& View)
	{
		if (!IsTrueSight(Sight))
		{
			return View;
		}
		return FRotator(0.f, View.Yaw, 0.f);
	}

	inline const TCHAR* Label(ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::True: return TEXT("True sight");
		case ESolidSight::Falcon: return TEXT("Falcon sight");
		case ESolidSight::Eagle: return TEXT("Eagle sight");
		case ESolidSight::Raven:
		default: return TEXT("Raven sight");
		}
	}

	/** Top-right bird or eye. Falcon and eagle use the same eagle as raven. */
	inline const TCHAR* Marker(ESolidSight Sight)
	{
		return IsTrueSight(Sight) ? TEXT("\U0001F441") : TEXT("\U0001F985");
	}

	/** Glyph behind the bird. Empty for true sight and raven sight. */
	inline const TCHAR* Mark(ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::Falcon: return TEXT("+");
		case ESolidSight::Eagle: return TEXT("\U0001F310");
		default: return TEXT("");
		}
	}

	inline bool HasMark(const ESolidSight Sight)
	{
		const TCHAR* Glyph = Mark(Sight);
		return Glyph != nullptr && Glyph[0] != 0;
	}

	/** Oval width in pixels. Wider when a mark sits behind the bird. 72 matches the raven chip. */
	inline float ChipWidth(const ESolidSight Sight)
	{
		return HasMark(Sight) ? 112.f : 72.f;
	}

	/** The globe is larger than the plus. The bird stays at 28. */
	inline float MarkFontSize(const ESolidSight Sight)
	{
		return Sight == ESolidSight::Eagle ? 28.f : 20.f;
	}

	/**
	 * Top-right oval. Hidden until raven sight unlocks, and while the character editor is open.
	 * The editor hides it even when raven sight is unlocked.
	 */
	inline bool ShowsIndicator(const bool bRavenSightEnabled, const bool bCharacterEditorOpen)
	{
		return bRavenSightEnabled && !bCharacterEditorOpen;
	}
}
