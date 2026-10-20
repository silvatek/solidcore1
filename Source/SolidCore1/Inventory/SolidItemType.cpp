#include "SolidItemType.h"

namespace SolidItemTypePrivate
{
	/** Grade index for the rising-spread roll. NA is not a grade. */
	int32 GradeIndex(const ESolidItemQuality Quality)
	{
		switch (Quality)
		{
		case ESolidItemQuality::Inferior:
			return 0;
		case ESolidItemQuality::Common:
			return 1;
		case ESolidItemQuality::Superior:
			return 2;
		case ESolidItemQuality::Exquisite:
			return 3;
		case ESolidItemQuality::Legendary:
			return 4;
		case ESolidItemQuality::NA:
		default:
			return INDEX_NONE;
		}
	}

	/**
	 * Log term that eases a spread from the low side to the high side.
	 * A large draw uses the limit, so the exponential stays finite.
	 */
	double GlideLog(const double StandardNormal, const double Width)
	{
		const double X = StandardNormal / Width;
		if (X > 40.0)
		{
			return FMath::Loge(2.0) - X;
		}
		if (X < -40.0)
		{
			return FMath::Loge(2.0);
		}
		return FMath::Loge(2.0 / (1.0 + FMath::Exp(X)));
	}

	/** How far log power moves for one draw. Equal spreads stay linear. */
	double LogStep(
		const double StandardNormal,
		const double Downside,
		const double Upside,
		const double Width)
	{
		if (Upside == Downside)
		{
			return Downside * StandardNormal;
		}
		return Downside * StandardNormal + (Downside - Upside) * Width * GlideLog(StandardNormal, Width);
	}
}

int32 SolidItemType::PowerFor(const ESolidItemQuality Quality, const double StandardNormal)
{
	const int32 Grade = SolidItemTypePrivate::GradeIndex(Quality);
	if (Grade == INDEX_NONE || !FMath::IsFinite(StandardNormal))
	{
		return 0;
	}

	double Median = PowerMedian;
	for (int32 Index = 0; Index < Grade; ++Index)
	{
		Median *= PowerMedianRatio;
	}

	const double Upside = (Quality == ESolidItemQuality::Legendary) ? LegendaryUpside : PowerSpread;
	const double Step = SolidItemTypePrivate::LogStep(StandardNormal, PowerSpread, Upside, PowerGlide);
	const double Value = Median * FMath::Exp(Step);
	if (!FMath::IsFinite(Value) || Value >= static_cast<double>(MAX_int32))
	{
		return MAX_int32;
	}
	return FMath::Max(1, static_cast<int32>(FMath::FloorToDouble(Value + 0.5)));
}
