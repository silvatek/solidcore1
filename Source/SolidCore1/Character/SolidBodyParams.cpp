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
