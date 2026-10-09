#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidTerrainFog.h"
#include "SolidTerrainMap.h"
#include "SolidTerrainTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFogDistanceBandsTest,
	"SolidCore1.Fog.DistanceBands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFogDistanceBandsTest::RunTest(const FString& Parameters)
{
	using namespace SolidTerrainFog;

	TestTrue(TEXT("0m is clear"), FMath::IsNearlyEqual(FogFromDistanceMeters(0.f), 0.f));
	TestTrue(TEXT("just inside clear radius is clear"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(ClearRadiusMeters - 0.01f), 0.f));
	TestTrue(TEXT("clear radius starts half fog"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(HalfFogStartMeters), 0.5f));
	TestTrue(TEXT("mid half band is 0.5"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(37.5f), 0.5f));
	TestTrue(TEXT("just inside full start stays half"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(FullFogStartMeters - 0.01f), 0.5f));
	// FogFromDistanceMeters uses DistM > FullFogStartMeters for 1.0 (strict).
	TestTrue(TEXT("exactly FullFogStartMeters is still half (strict >)"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(FullFogStartMeters), 0.5f));
	TestTrue(TEXT("just past full start is 1.0"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(FullFogStartMeters + 0.01f), 1.f));
	TestTrue(TEXT("far away is full fog"),
		FMath::IsNearlyEqual(FogFromDistanceMeters(200.f), 1.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFogUnitsTest,
	"SolidCore1.Fog.Units",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFogUnitsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("25m -> 2500cm"), SolidTerrainFog::MetersToCm(25.f), 2500.f);
	TestEqual(TEXT("50m -> 5000cm"), SolidTerrainFog::MetersToCm(50.f), 5000.f);
	TestTrue(TEXT("clear epsilon is small positive"), SolidTerrainFog::ClearFogEpsilon > 0.f);
	TestTrue(TEXT("half/full threshold between 0.5 and 1"),
		SolidTerrainFog::HalfFullFogThreshold > 0.5f
		&& SolidTerrainFog::HalfFullFogThreshold < 1.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFogSampleMistAroundTest,
	"SolidCore1.Fog.SampleMistAround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFogSampleMistAroundTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("null map => 0"),
		FMath::IsNearlyEqual(SolidTerrainFog::SampleMistAmountAround(nullptr, FVector::ZeroVector), 0.f));

	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	// At origin: local fog 0, but ring at 25/50m hits fogged cells.
	const float MistAtOrigin = SolidTerrainFog::SampleMistAmountAround(Map, FVector::ZeroVector);
	TestTrue(TEXT("mist at origin sees surrounding fog"), MistAtOrigin > 0.f);

	const float MistInFull = SolidTerrainFog::SampleMistAmountAround(Map, FVector(6000.f, 0.f, 0.f));
	TestTrue(TEXT("mist in full fog is 1"), FMath::IsNearlyEqual(MistInFull, 1.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
