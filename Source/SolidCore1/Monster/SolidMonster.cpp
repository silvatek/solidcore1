#include "SolidMonster.h"

FSolidMonsterStandingSpan FSolidMonster::StandingSpan(TArray<float>& SamplesZ)
{
	FSolidMonsterStandingSpan Span;
	if (SamplesZ.Num() == 0)
	{
		return Span;
	}

	SamplesZ.Sort();
	const int32 Last = SamplesZ.Num() - 1;
	const float FullHeight = SamplesZ[Last] - SamplesZ[0];
	if (FullHeight <= SpanOutlierCm || SamplesZ.Num() < 50)
	{
		Span.BottomZ = SamplesZ[0];
		Span.Height = FullHeight;
		return Span;
	}

	int32 BestLow = 0;
	int32 BestHigh = 0;
	int32 BestCount = 0;
	int32 End = 0;
	for (int32 Start = 0; Start <= Last; ++Start)
	{
		if (End < Start)
		{
			End = Start;
		}
		while (End < Last && SamplesZ[End + 1] - SamplesZ[Start] <= SpanOutlierCm)
		{
			++End;
		}
		const int32 BandCount = End - Start + 1;
		if (BandCount > BestCount)
		{
			BestCount = BandCount;
			BestLow = Start;
			BestHigh = End;
		}
	}

	Span.BottomZ = SamplesZ[BestLow];
	Span.Height = SamplesZ[BestHigh] - SamplesZ[BestLow];
	return Span;
}

float FSolidMonster::UniformScale(const float BoundsHeightCm)
{
	if (BoundsHeightCm <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	return StandingHeightCm / BoundsHeightCm;
}

float FSolidMonster::MeshFootOffsetCm(
	const float BoundsOriginZ,
	const float BoundsExtentZ,
	const float Scale)
{
	const float Bottom = BoundsOriginZ - BoundsExtentZ;
	return -Bottom * Scale;
}
