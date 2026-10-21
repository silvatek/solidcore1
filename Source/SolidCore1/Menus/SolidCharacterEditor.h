#pragma once

#include "CoreMinimal.h"
#include "Company/SolidCompany.h"

/** One slider track in the character-editor panel. */
struct FSolidSliderBar
{
	float X = 0.f;
	float Y = 0.f;
	float W = 0.f;
	float H = 0.f;

	bool Contains(const float PointX, const float PointY) const
	{
		return PointX >= X && PointX <= X + W && PointY >= Y && PointY <= Y + H;
	}
};

/** Label on the left, track on the right. A label click selects. A track click sets the value. */
struct FSolidSliderRow
{
	int32 Slider = 0;
	float X = 0.f;
	float Y = 0.f;
	float H = 0.f;
	FSolidSliderBar Bar;
};

enum class ESolidSliderHit : uint8
{
	None,
	Label,
	Bar,
};

struct FSolidSliderHit
{
	int32 Slider = INDEX_NONE;
	ESolidSliderHit Kind = ESolidSliderHit::None;
};

/** Text and slider geometry for the F10 character editor. The stage itself is a world actor. */
namespace SolidCharacterEditor
{
	inline constexpr float SliderBarWidth = 160.f;
	inline constexpr float SliderBarGap = 16.f;

	/** Move Index by Delta and wrap inside Count. Count of 0 stays at 0. */
	int32 WrapIndex(int32 Index, int32 Delta, int32 Count);

	/**
	 * Name, vocation, height, build, gender, the other body channels, and shirt, trouser, and skin numbers.
	 * The closing lines are the left/right, up/down, page up/page down, and F10 hints.
	 */
	void Describe(const FSolidCompanion& Companion, TArray<FString>& OutLines);

	/** Space in the character editor. Skeleton mode asks for the body, and the body asks for the skeleton. */
	FString SkeletonHint(bool bSkeleton);

	/** Degrees added to the preview yaw for one Home or End press. Home is clockwise on screen. */
	inline constexpr float TurnStepDegrees = 15.f;

	/** Negative yaw is clockwise as you look at the figure. */
	inline float TurnYawDelta(const bool bClockwise)
	{
		return bClockwise ? -TurnStepDegrees : TurnStepDegrees;
	}

	FString TurnClockwiseHint();
	FString TurnAnticlockwiseHint();

	/** Height, Build, and Gender lines are the three sliders. Anything else is INDEX_NONE. */
	int32 SliderForLine(const FString& Line);

	/** Place a track to the right of the text column, the full height of that line. */
	FSolidSliderRow MakeSliderRow(int32 Slider, float LabelX, float LineY, float LineH, float TextRight);

	/** Bar hits win over the label. A point left of the track on that line selects the slider. */
	FSolidSliderHit HitSliders(const FSolidSliderRow* Rows, int32 Count, float X, float Y);

	/** 0 at the left of the track, 1 at the right. */
	float BarFraction(const FSolidSliderBar& Bar, float X);
}
