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
		(void)Slider;
		return 0.f;
	}

	float SliderMax(const int32 Slider)
	{
		(void)Slider;
		return 1.f;
	}

	float ClampSlider(const float Value)
	{
		return FMath::Clamp(Value, SliderMin(0), SliderMax(0));
	}

	/** Build 0 is 1 - Delta, build 1 is 1 + Delta, and build 0.5 is exactly 1. */
	float FromBuild(const float Delta, const float Build)
	{
		return 1.f + (ClampSlider(Build) - 0.5f) * 2.f * Delta;
	}

	float HeightScale(const float Height)
	{
		const float Slider = ClampSlider(Height);
		if (Slider <= 0.5f)
		{
			return FMath::Lerp(SolidBodyControls::HeightScaleLow, 1.f, Slider / 0.5f);
		}
		return FMath::Lerp(1.f, SolidBodyControls::HeightScaleHigh, (Slider - 0.5f) / 0.5f);
	}
}

FSolidBodyControls SolidBodyControls::FromSeed(const FSolidBodyParams& Seed)
{
	(void)Seed;
	FSolidBodyControls Controls;
	Controls.Height = SolidBodyControls::GenderNeutral;
	Controls.Build = SolidBodyControls::GenderNeutral;
	Controls.Gender = SolidBodyControls::GenderNeutral;
	return Controls;
}

FSolidBodyParams SolidBodyControls::Resolve(const FSolidBodyParams& Seed, const FSolidBodyControls& Controls)
{
	(void)Seed;
	const float Build = ClampSlider(Controls.Build);
	const float Gender = ClampSlider(Controls.Gender);
	const float Bias = (Gender - SolidBodyControls::GenderNeutral) * 2.f;

	FSolidBodyParams Params;
	Params.Height = FromBuild(SolidBodyControls::BuildHeightDelta, Build);
	Params.ShoulderWidth = FromBuild(SolidBodyControls::BuildShoulderDelta, Build);
	Params.HipWidth = FromBuild(SolidBodyControls::BuildHipDelta, Build);
	Params.TorsoGirth = FromBuild(SolidBodyControls::BuildTorsoDelta, Build);
	Params.ArmLength = FromBuild(SolidBodyControls::BuildArmDelta, Build);
	Params.LegLength = FromBuild(SolidBodyControls::BuildLegDelta, Build);
	Params.HeadSize = FromBuild(SolidBodyControls::BuildHeadDelta, Build);
	Params.NeckLength = FromBuild(SolidBodyControls::BuildNeckDelta, Build);
	Params.Bulk = FromBuild(SolidBodyControls::BuildBulkDelta, Build);

	// Feminine is shorter. Masculine is taller. Legs already follow Height in the deform.
	const float GenderHeight = 1.f + Bias * SolidBodyControls::GenderHeightSpread;
	Params.Height *= GenderHeight;
	Params.ArmLength *= GenderHeight;
	Params.NeckLength *= GenderHeight;
	Params.ShoulderWidth *= 1.f + Bias * SolidBodyControls::GenderShoulderSpread;
	Params.HipWidth *= 1.f - Bias * SolidBodyControls::GenderHipSpread;
	Params.TorsoGirth *= 1.f + Bias * SolidBodyControls::GenderChestSpread;
	Params.Bulk *= 1.f + Bias * SolidBodyControls::GenderBulkSpread;
	Params.NeckLength *= 1.f - Bias * SolidBodyControls::GenderNeckSpread;
	Params.HeadSize *= 1.f - Bias * SolidBodyControls::GenderHeadSpread;

	const float Scale = HeightScale(Controls.Height);
	Params.Height *= Scale;
	Params.ArmLength *= Scale;
	Params.NeckLength *= Scale;
	return Params.Clamped();
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
