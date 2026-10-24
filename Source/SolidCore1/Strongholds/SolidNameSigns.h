#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Strongholds/SolidStronghold.h"

/**
 * The company stronghold's name sign.
 * The board is one line, the stronghold's name. The pole and the board match a town sign.
 * Pure logic — no actor spawning (safe for automation).
 */
namespace SolidNameSigns
{
	inline constexpr int32 MaxCharacters = 24;
	/** Past the frame edge, before half the board. The board's near edge stays this far clear. */
	inline constexpr float StepCm = 80.f;

	/**
	 * The typed name, trimmed, at most MaxCharacters.
	 * Empty when nothing is left. That result is not kept.
	 */
	FString Keep(const FString& Typed);

	/**
	 * Letters, digits, space, and Backspace change the line.
	 * A letter past MaxCharacters is dropped. Enter and Esc are not edits.
	 * @return True when Line changed.
	 */
	bool ApplyKey(FString& Line, const FKey& Key, bool bUpper);

	/** Width of the one-line board. The same measure a town sign uses for a line. */
	float BoardWidthCm(const FString& Name);

	/** One line of glyphs, plus the padding a town board uses around its text. */
	float BoardHeightCm();

	/**
	 * Pole position in the exterior actor's centimetres.
	 * A step past the frame edge, then half the board, so the board clears the opening.
	 */
	FVector PoleLocalCm(const FSolidNameSignPlace& Place, const FString& Name);

	/** Where the pole stands in the world, and the yaw that turns the board toward the grass. */
	struct FWorldPlace
	{
		FVector2D XY = FVector2D::ZeroVector;
		float YawDeg = 0.f;
	};

	/**
	 * The name sign for this stronghold.
	 * The exterior yaw turns the type's place into world centimetres.
	 */
	FWorldPlace WorldPlace(const FSolidStronghold& Stronghold, const USolidWorldChart* Map);

	/** The sign stands at the company stronghold. One the company has left does not get one. */
	inline bool StandsFor(const FSolidStronghold* Active, const FSolidStronghold* Candidate)
	{
		return Active && Candidate && Active == Candidate;
	}
}
