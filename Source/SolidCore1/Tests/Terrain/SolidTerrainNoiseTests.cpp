#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainNoise.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNoiseHashDeterminismTest,
	"SolidCore1.Noise.HashDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNoiseHashDeterminismTest::RunTest(const FString& Parameters)
{
	const uint32 A = SolidTerrainNoise::HashCoords(12, -7, 1337);
	const uint32 B = SolidTerrainNoise::HashCoords(12, -7, 1337);
	const uint32 C = SolidTerrainNoise::HashCoords(12, -7, 1338);
	TestEqual(TEXT("same inputs hash equal"), A, B);
	TestTrue(TEXT("seed changes hash"), A != C);

	const float F0 = SolidTerrainNoise::HashToFloat(0, 0, 1);
	const float F1 = SolidTerrainNoise::HashToFloat(0, 0, 1);
	TestTrue(TEXT("hash float in [0,1]"), F0 >= 0.f && F0 <= 1.f);
	TestTrue(TEXT("hash float deterministic"), FMath::IsNearlyEqual(F0, F1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNoiseValueAndFbmTest,
	"SolidCore1.Noise.ValueAndFbm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNoiseValueAndFbmTest::RunTest(const FString& Parameters)
{
	const float V = SolidTerrainNoise::ValueNoise2D(3.25f, -1.5f, 42);
	TestTrue(TEXT("value noise in [0,1]"), V >= 0.f && V <= 1.f);

	const float Fbm = SolidTerrainNoise::Fbm2D(10.f, 20.f, 99, 5);
	TestTrue(TEXT("fbm in [0,1]"), Fbm >= 0.f && Fbm <= 1.f);
	TestTrue(TEXT("fbm deterministic"),
		FMath::IsNearlyEqual(Fbm, SolidTerrainNoise::Fbm2D(10.f, 20.f, 99, 5)));

	const float H = SolidTerrainNoise::SampleHeight(1000.f, -500.f, 7, 0.00012f, 3000.f, 100.f);
	TestTrue(TEXT("height >= base"), H >= 100.f - KINDA_SMALL_NUMBER);
	TestTrue(TEXT("height <= base+amp"), H <= 100.f + 3000.f + KINDA_SMALL_NUMBER);

	const float Tone = SolidTerrainNoise::SampleGrassTone(2500.f, 2500.f, 3);
	TestTrue(TEXT("grass tone in [0,1]"), Tone >= 0.f && Tone <= 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNoiseSmoothstepTest,
	"SolidCore1.Noise.Smoothstep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNoiseSmoothstepTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("smoothstep(0)=0"), FMath::IsNearlyEqual(SolidTerrainNoise::Smoothstep(0.f), 0.f));
	TestTrue(TEXT("smoothstep(1)=1"), FMath::IsNearlyEqual(SolidTerrainNoise::Smoothstep(1.f), 1.f));
	TestTrue(TEXT("smoothstep(0.5)=0.5"), FMath::IsNearlyEqual(SolidTerrainNoise::Smoothstep(0.5f), 0.5f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
