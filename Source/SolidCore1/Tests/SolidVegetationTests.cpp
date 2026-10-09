#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidMonolith.h"
#include "SolidTree.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTreeRandomVariationTest,
	"SolidCore1.Vegetation.TreeRandomVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTreeRandomVariationTest::RunTest(const FString& Parameters)
{
	ASolidTree* A = NewObject<ASolidTree>();
	ASolidTree* B = NewObject<ASolidTree>();
	TestNotNull(TEXT("tree A"), A);
	TestNotNull(TEXT("tree B"), B);

	FRandomStream RngA(42);
	FRandomStream RngB(42);
	A->ApplyRandomVariation(RngA);
	B->ApplyRandomVariation(RngB);

	TestTrue(TEXT("same seed => same trunk height"),
		FMath::IsNearlyEqual(A->TrunkHeightCm, B->TrunkHeightCm));
	TestTrue(TEXT("same seed => same trunk radius"),
		FMath::IsNearlyEqual(A->TrunkRadiusCm, B->TrunkRadiusCm));
	TestTrue(TEXT("same seed => same canopy height"),
		FMath::IsNearlyEqual(A->CanopyHeightCm, B->CanopyHeightCm));
	TestTrue(TEXT("same seed => same canopy radius"),
		FMath::IsNearlyEqual(A->CanopyRadiusCm, B->CanopyRadiusCm));

	TestTrue(TEXT("trunk height positive"), A->TrunkHeightCm > 10.f);
	TestTrue(TEXT("trunk radius clamped"), A->TrunkRadiusCm >= 8.f);
	TestTrue(TEXT("canopy radius clamped"), A->CanopyRadiusCm >= 40.f);

	FRandomStream RngOther(99);
	ASolidTree* C = NewObject<ASolidTree>();
	C->ApplyRandomVariation(RngOther);
	TestTrue(TEXT("different seed usually changes size"),
		!FMath::IsNearlyEqual(A->TrunkHeightCm, C->TrunkHeightCm)
		|| !FMath::IsNearlyEqual(A->CanopyRadiusCm, C->CanopyRadiusCm));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonolithDefaultsTest,
	"SolidCore1.Vegetation.MonolithDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonolithDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidMonolith* Mono = NewObject<ASolidMonolith>();
	TestNotNull(TEXT("monolith"), Mono);
	TestTrue(TEXT("width positive"), Mono->WidthCm > 0.f);
	TestTrue(TEXT("thickness positive"), Mono->ThicknessCm > 0.f);
	TestTrue(TEXT("height positive"), Mono->HeightCm > 0.f);
	TestTrue(TEXT("taller than wide"), Mono->HeightCm > Mono->WidthCm);

	Mono->BuildVisuals();
	TestNotNull(TEXT("slab mesh component"), Mono->GetRootComponent());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
