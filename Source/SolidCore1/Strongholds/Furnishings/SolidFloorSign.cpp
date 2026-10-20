#include "SolidFloorSign.h"

namespace SolidStronghold
{
	int32 FSolidFloorSign::DigitMask(const int32 Digit)
	{
		switch (Digit)
		{
		case 0: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5);
		case 1: return (1 << 1) | (1 << 2);
		case 2: return (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 6);
		case 3: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 6);
		case 4: return (1 << 1) | (1 << 2) | (1 << 5) | (1 << 6);
		case 5: return (1 << 0) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6);
		case 6: return (1 << 0) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
		case 7: return (1 << 0) | (1 << 1) | (1 << 2);
		case 8: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
		case 9: return (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 5) | (1 << 6);
		default: return 0;
		}
	}

	void FSolidFloorSign::CollectStrokes(TArray<FStroke>& Out) const
	{
		Out.Reset();
		TArray<int32, TInlineAllocator<4>> Digits;
		int32 Remaining = FMath::Max(ShownNumber, 0);
		if (Remaining == 0)
		{
			Digits.Add(0);
		}
		while (Remaining > 0)
		{
			Digits.Add(Remaining % 10);
			Remaining /= 10;
		}

		const float Pitch = DigitWidthCm + DigitGapCm;
		const int32 Count = Digits.Num();
		const float LeftU = -0.5f * static_cast<float>(Count - 1) * Pitch;
		const float HalfBar = DigitWidthCm * 0.5f - DigitStrokeCm * 0.5f;
		const float Outer = DigitHeightCm * 0.5f - DigitStrokeCm * 0.5f;
		const float Rise = DigitHeightCm * 0.25f;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const int32 Digit = Digits[Count - 1 - Index];
			const int32 Mask = DigitMask(Digit);
			const float DigitY = -(LeftU + static_cast<float>(Index) * Pitch);
			auto Add = [&](const int32 Bit, const float VisualRightCm, const float UpCm, const float WidthCm, const float HeightCm)
			{
				if ((Mask & Bit) == 0)
				{
					return;
				}
				FStroke Stroke;
				Stroke.CenterCm = FVector(0.f, DigitY - VisualRightCm, UpCm);
				Stroke.SizeCm = FVector(DigitDepthCm, WidthCm, HeightCm);
				Out.Add(Stroke);
			};
			Add(1 << 0, 0.f, Outer, DigitWidthCm, DigitStrokeCm);
			Add(1 << 3, 0.f, -Outer, DigitWidthCm, DigitStrokeCm);
			Add(1 << 6, 0.f, 0.f, DigitWidthCm, DigitStrokeCm);
			Add(1 << 1, HalfBar, Rise, DigitStrokeCm, DigitHeightCm * 0.5f);
			Add(1 << 2, HalfBar, -Rise, DigitStrokeCm, DigitHeightCm * 0.5f);
			Add(1 << 5, -HalfBar, Rise, DigitStrokeCm, DigitHeightCm * 0.5f);
			Add(1 << 4, -HalfBar, -Rise, DigitStrokeCm, DigitHeightCm * 0.5f);
		}
	}

	FVector FSolidFloorSign::PlateLocation() const
	{
		const FVector Inward = FRotator(0.f, FacingYawDeg, 0.f).Vector();
		const float TowardWall = DigitDepthCm * 0.5f + PlateGapCm + PlateDepthCm * 0.5f;
		return StandLocation - Inward * TowardWall;
	}
}
