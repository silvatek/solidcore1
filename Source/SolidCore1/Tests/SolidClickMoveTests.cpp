#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidClickMove.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidClickMoveRulesTest,
	"SolidCore1.ClickMove.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidClickMoveRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("raven sight accepts a click"), SolidClickMove::AllowsClick(true, false));
	TestFalse(TEXT("true sight ignores a click"), SolidClickMove::AllowsClick(false, false));
	TestFalse(TEXT("menu or drill ignores a click"), SolidClickMove::AllowsClick(true, true));

	TestTrue(TEXT("inside the arrival radius"), SolidClickMove::HasArrived(FVector2D(0.f, 0.f), FVector2D(90.f, 0.f)));
	TestFalse(TEXT("still short of the point"), SolidClickMove::HasArrived(FVector2D(0.f, 0.f), FVector2D(120.f, 0.f)));

	const FVector2D Toward = SolidClickMove::PlanarDirection(FVector2D(0.f, 0.f), FVector2D(400.f, 0.f));
	TestTrue(TEXT("walk toward the point"), Toward.Equals(FVector2D(1.f, 0.f), 0.001f));
	TestTrue(TEXT("stop on arrival"),
		SolidClickMove::PlanarDirection(FVector2D(0.f, 0.f), FVector2D(40.f, 0.f)).IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidClickMoveGroundTest,
	"SolidCore1.ClickMove.Ground",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidClickMoveGroundTest::RunTest(const FString& Parameters)
{
	auto Flat = [](float, float)
	{
		return 0.f;
	};
	auto Slope = [](const float X, float)
	{
		return X * 0.5f;
	};

	FVector Hit = FVector::ZeroVector;
	TestTrue(TEXT("straight down meets the ground"),
		SolidClickMove::IntersectHeight(FVector(0.f, 0.f, 50.f), FVector(0.f, 0.f, -1.f), Flat, Hit));
	TestTrue(TEXT("downward hit is the origin column"), Hit.Equals(FVector(0.f, 0.f, 0.f), 1.f));

	const FVector Diagonal(1.f, 0.f, -1.f);
	TestTrue(TEXT("diagonal meets flat ground"),
		SolidClickMove::IntersectHeight(FVector(0.f, 0.f, 100.f), Diagonal, Flat, Hit));
	TestTrue(TEXT("diagonal lands near x = 100"), Hit.Equals(FVector(100.f, 0.f, 0.f), 2.f));

	TestTrue(TEXT("downward ray meets the slope"),
		SolidClickMove::IntersectHeight(FVector(10.f, 4.f, 100.f), FVector(0.f, 0.f, -1.f), Slope, Hit));
	TestTrue(TEXT("slope hit keeps the column"), Hit.Equals(FVector(10.f, 4.f, 5.f), 1.f));

	TestFalse(TEXT("a ray above the ground misses"),
		SolidClickMove::IntersectHeight(FVector(0.f, 0.f, 20.f), FVector(1.f, 0.f, 0.f), Flat, Hit));
	TestFalse(TEXT("a ray that starts underground misses"),
		SolidClickMove::IntersectHeight(FVector(0.f, 0.f, -10.f), FVector(1.f, 0.f, -0.2f), Flat, Hit));
	TestFalse(TEXT("a zero direction misses"),
		SolidClickMove::IntersectHeight(FVector(0.f, 0.f, 20.f), FVector::ZeroVector, Flat, Hit));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
