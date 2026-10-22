#include "SolidMonster.h"
#include "Inventory/SolidItemType.h"

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

int32 SolidMonster::PowerFor(const ESolidMonsterSize Size, const double StandardNormal)
{
	int32 Grade = INDEX_NONE;
	for (int32 Index = 0; Index < SizeCount; ++Index)
	{
		if (Sizes[Index] == Size)
		{
			Grade = Index;
			break;
		}
	}
	if (Grade == INDEX_NONE)
	{
		return 0;
	}
	return SolidItemType::PowerForGrade(Grade, Grade == SizeCount - 1, StandardNormal);
}

bool FSolidMonster::Detects(const FVector2D& Point) const
{
	return FVector2D::Distance(XY, Point) <= DetectionRange;
}

void FSolidMonster::Arm(const TCHAR* WeaponName, const TCHAR* ArmourName, const double StandardNormal)
{
	Weapon.Name = (WeaponName && WeaponName[0]) ? WeaponName : TEXT("");
	Weapon.Speed = 1.f;
	Weapon.Power = SolidMonster::PowerFor(Size, StandardNormal);
	Armour.Name = (ArmourName && ArmourName[0]) ? ArmourName : TEXT("");
	Armour.Strength = 1.f;
}

float FSolidMonster::MeshFootOffsetCm(
	const float BoundsOriginZ,
	const float BoundsExtentZ,
	const float Scale)
{
	const float Bottom = BoundsOriginZ - BoundsExtentZ;
	return -Bottom * Scale;
}
