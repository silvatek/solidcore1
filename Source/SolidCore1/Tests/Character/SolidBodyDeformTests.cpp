#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidBodyParams.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool ChannelInRange(const float Value)
	{
		return Value >= SolidBody::ParamMin && Value <= SolidBody::ParamMax;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySeedTest,
	"SolidCore1.Body.Seed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySeedTest::RunTest(const FString& Parameters)
{
	const FSolidBodyParams First = FSolidBodyParams::FromSeed(42);
	const FSolidBodyParams Second = FSolidBodyParams::FromSeed(42);
	TestTrue(TEXT("same seed matches byte for byte"), First.ExactEquals(Second));
	TestTrue(TEXT("different seeds differ"), !FSolidBodyParams::FromSeed(1).ExactEquals(FSolidBodyParams::FromSeed(2)));

	TestTrue(TEXT("height in range"), ChannelInRange(First.Height));
	TestTrue(TEXT("shoulders in range"), ChannelInRange(First.ShoulderWidth));
	TestTrue(TEXT("hips in range"), ChannelInRange(First.HipWidth));
	TestTrue(TEXT("torso in range"), ChannelInRange(First.TorsoGirth));
	TestTrue(TEXT("arms in range"), ChannelInRange(First.ArmLength));
	TestTrue(TEXT("legs in range"), ChannelInRange(First.LegLength));
	TestTrue(TEXT("head in range"), ChannelInRange(First.HeadSize));
	TestTrue(TEXT("neck in range"), ChannelInRange(First.NeckLength));
	TestTrue(TEXT("bulk in range"), ChannelInRange(First.Bulk));

	const FSolidBodyParams Identity;
	TestTrue(TEXT("default height"), FMath::IsNearlyEqual(Identity.Height, 1.f));
	TestTrue(TEXT("default is in range"), Identity.ExactEquals(Identity.Clamped()));

	FSolidBodyParams Wild;
	Wild.Height = 0.1f;
	Wild.Bulk = 4.f;
	const FSolidBodyParams Clamped = Wild.Clamped();
	TestTrue(TEXT("short height clamps up"), FMath::IsNearlyEqual(Clamped.Height, SolidBody::ParamMin));
	TestTrue(TEXT("bulk clamps down"), FMath::IsNearlyEqual(Clamped.Bulk, SolidBody::ParamMax));
	TestTrue(TEXT("untouched channel stays"), FMath::IsNearlyEqual(Clamped.ArmLength, 1.f));
	return true;
}

#endif
