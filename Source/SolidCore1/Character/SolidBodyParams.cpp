#include "SolidBodyParams.h"

static_assert(
	sizeof(FSolidBodyParams) == sizeof(float) * 9,
	"FSolidBodyParams stays nine floats so ExactEquals has no padding.");

namespace
{
	float SampleChannel(FRandomStream& Stream)
	{
		return FMath::Lerp(SolidBody::ParamMin, SolidBody::ParamMax, Stream.GetFraction());
	}

	float ClampChannel(const float Value)
	{
		return FMath::Clamp(Value, SolidBody::ParamMin, SolidBody::ParamMax);
	}
}

FSolidBodyParams FSolidBodyParams::FromSeed(const uint32 Seed)
{
	// Channel order is part of the seed. Adding a channel goes at the end.
	FRandomStream Stream(Seed);
	FSolidBodyParams Params;
	Params.Height = SampleChannel(Stream);
	Params.ShoulderWidth = SampleChannel(Stream);
	Params.HipWidth = SampleChannel(Stream);
	Params.TorsoGirth = SampleChannel(Stream);
	Params.ArmLength = SampleChannel(Stream);
	Params.LegLength = SampleChannel(Stream);
	Params.HeadSize = SampleChannel(Stream);
	Params.NeckLength = SampleChannel(Stream);
	Params.Bulk = SampleChannel(Stream);
	return Params;
}

FSolidBodyParams FSolidBodyParams::Clamped() const
{
	FSolidBodyParams Params;
	Params.Height = ClampChannel(Height);
	Params.ShoulderWidth = ClampChannel(ShoulderWidth);
	Params.HipWidth = ClampChannel(HipWidth);
	Params.TorsoGirth = ClampChannel(TorsoGirth);
	Params.ArmLength = ClampChannel(ArmLength);
	Params.LegLength = ClampChannel(LegLength);
	Params.HeadSize = ClampChannel(HeadSize);
	Params.NeckLength = ClampChannel(NeckLength);
	Params.Bulk = ClampChannel(Bulk);
	return Params;
}

bool FSolidBodyParams::ExactEquals(const FSolidBodyParams& Other) const
{
	return FMemory::Memcmp(this, &Other, sizeof(FSolidBodyParams)) == 0;
}

namespace
{
	float& ControlChannel(FSolidBodyControls& Controls, const int32 Slider)
	{
		switch (Slider)
		{
		case SolidBodyControls::Build:
			return Controls.Build;
		case SolidBodyControls::Gender:
			return Controls.Gender;
		default:
			return Controls.Height;
		}
	}

	float SliderMin(const int32 Slider)
	{
		return Slider == SolidBodyControls::Gender ? 0.f : SolidBody::ParamMin;
	}

	float SliderMax(const int32 Slider)
	{
		return Slider == SolidBodyControls::Gender ? 1.f : SolidBody::ParamMax;
	}
}

FSolidBodyControls SolidBodyControls::FromSeed(const FSolidBodyParams& Seed)
{
	FSolidBodyControls Controls;
	Controls.Height = Seed.Height;
	Controls.Build = Seed.Bulk;
	Controls.Gender = SolidBodyControls::GenderNeutral;
	return Controls;
}

FSolidBodyParams SolidBodyControls::Resolve(const FSolidBodyParams& Seed, const FSolidBodyControls& Controls)
{
	FSolidBodyParams Params = Seed;
	Params.Height = FMath::Clamp(Controls.Height, SolidBody::ParamMin, SolidBody::ParamMax);

	const float Build = FMath::Clamp(Controls.Build, SolidBody::ParamMin, SolidBody::ParamMax);
	Params.Bulk = Build;
	// Skip the scale when the bits already match, so a default slider stays ExactEquals.
	if (Build != Seed.Bulk && Seed.Bulk != 0.f)
	{
		Params.TorsoGirth = FMath::Clamp(
			Seed.TorsoGirth * (Build / Seed.Bulk),
			SolidBody::ParamMin,
			SolidBody::ParamMax);
	}

	if (Controls.Gender != SolidBodyControls::GenderNeutral)
	{
		const float Gender = FMath::Clamp(Controls.Gender, 0.f, 1.f);
		const float Bias = (Gender - SolidBodyControls::GenderNeutral) * 2.f;
		Params.ShoulderWidth = FMath::Clamp(
			Seed.ShoulderWidth - Bias * SolidBodyControls::GenderSpread,
			SolidBody::ParamMin,
			SolidBody::ParamMax);
		Params.HipWidth = FMath::Clamp(
			Seed.HipWidth + Bias * SolidBodyControls::GenderSpread,
			SolidBody::ParamMin,
			SolidBody::ParamMax);
	}
	return Params;
}

int32 SolidBodyControls::WrapSlider(const int32 Index, const int32 Delta)
{
	int32 Wrapped = (Index + Delta) % SolidBodyControls::Count;
	if (Wrapped < 0)
	{
		Wrapped += SolidBodyControls::Count;
	}
	return Wrapped;
}

void SolidBodyControls::Nudge(FSolidBodyControls& Controls, const int32 Slider, const int32 Direction)
{
	if (Direction == 0)
	{
		return;
	}
	float& Value = ControlChannel(Controls, Slider);
	const float Step = Direction > 0 ? SolidBodyControls::KeyStep : -SolidBodyControls::KeyStep;
	Value = FMath::Clamp(Value + Step, SliderMin(Slider), SliderMax(Slider));
}

float SolidBodyControls::Fraction(const int32 Slider, const float Value)
{
	const float Span = SliderMax(Slider) - SliderMin(Slider);
	if (Span <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return FMath::Clamp((Value - SliderMin(Slider)) / Span, 0.f, 1.f);
}

float SolidBodyControls::ValueAtFraction(const int32 Slider, const float FractionValue)
{
	const float T = FMath::Clamp(FractionValue, 0.f, 1.f);
	const float Raw = FMath::Lerp(SliderMin(Slider), SliderMax(Slider), T);
	const float Stepped = FMath::RoundToFloat(Raw / SolidBodyControls::MouseStep) * SolidBodyControls::MouseStep;
	return FMath::Clamp(Stepped, SliderMin(Slider), SliderMax(Slider));
}

void SolidBodyControls::SetFraction(FSolidBodyControls& Controls, const int32 Slider, const float FractionValue)
{
	ControlChannel(Controls, Slider) = ValueAtFraction(Slider, FractionValue);
}

const TCHAR* SolidBodyControls::Label(const int32 Slider)
{
	switch (Slider)
	{
	case SolidBodyControls::Build:
		return TEXT("Build");
	case SolidBodyControls::Gender:
		return TEXT("Gender");
	default:
		return TEXT("Height");
	}
}
