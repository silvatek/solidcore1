#pragma once

#include "CoreMinimal.h"

namespace SolidStronghold
{
	/**
	 * The number on a stronghold floor, drawn as dark bars on a light plate.
	 * A runtime text component on the wall did not draw, so the number is these bars.
	 * Where the sign stands is the visit's own placement. On a tower that is beside the landing.
	 * A sign is absent until Place is called.
	 */
	class FSolidFloorSign
	{
	public:
		/**
		 * One dark bar, in the sign's own frame.
		 * +X points into the room, +Y is to the viewer's left, +Z is up.
		 * The number is centered on the origin. The viewer's right is -Y.
		 */
		struct FStroke
		{
			FVector CenterCm = FVector::ZeroVector;
			FVector SizeCm = FVector::OneVector;
		};

		/** Show this number at this point. YawDeg points the text into the room. */
		void Place(int32 InNumber, const FVector& InLocation, float InYawDeg)
		{
			ShownNumber = InNumber;
			StandLocation = InLocation;
			FacingYawDeg = InYawDeg;
			bPresent = true;
		}

		bool IsPresent() const { return bPresent; }
		int32 Number() const { return ShownNumber; }
		const FVector& Location() const { return StandLocation; }
		float YawDeg() const { return FacingYawDeg; }

		/** Depth toward the room, width across the view, height up. */
		static FVector PlateSizeCm()
		{
			return FVector(PlateDepthCm, PlateWidthCm, PlateHeightCm);
		}

		static FLinearColor PlateColor()
		{
			return FLinearColor(0.90f, 0.86f, 0.74f);
		}

		static FColor TextColor()
		{
			return FColor(24, 18, 12);
		}

		/** Dark strokes for the number, centered on the sign. */
		void CollectStrokes(TArray<FStroke>& Out) const;

		/** Where the light plate sits, toward the wall from the strokes. */
		FVector PlateLocation() const;

	private:
		int32 ShownNumber = 0;
		FVector StandLocation = FVector::ZeroVector;
		/** Yaw that points the text's forward axis into the room. */
		float FacingYawDeg = 0.f;
		bool bPresent = false;

		/** One digit. A two-digit number, such as 10, is two of these with a gap between. */
		static constexpr float DigitWidthCm = 44.f;
		static constexpr float DigitHeightCm = 72.f;
		static constexpr float DigitStrokeCm = 11.f;
		static constexpr float DigitGapCm = 12.f;
		/** Thickness of a stroke, toward the room from the plate. */
		static constexpr float DigitDepthCm = 5.f;
		/** Light plate behind the strokes. The number reads on this, not on the bare stone. */
		static constexpr float PlateWidthCm = 140.f;
		static constexpr float PlateHeightCm = 110.f;
		static constexpr float PlateDepthCm = 8.f;
		/** Clear space between the plate's room face and the back of the strokes. */
		static constexpr float PlateGapCm = 2.f;

		/** Which bars a digit uses. Bit 0 is the top bar, then upper right, lower right, bottom, lower left, upper left, middle. */
		static int32 DigitMask(int32 Digit);
	};
}
