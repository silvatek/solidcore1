#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGroundMove.h"
#include "SolidBridge.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGroundMoveClampTest,
	"SolidCore1.GroundMove.Clamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGroundMoveClampTest::RunTest(const FString& Parameters)
{
	auto Shore = [](const float X, const float Y)
	{
		return X <= 0.f;
	};

	const FVector2D Into = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 0.f), FVector2D(100.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("a step into closed ground stops"), Into.IsNearlyZero());

	const FVector2D Along = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 0.f), FVector2D(0.f, 80.f), 40.f, Shore);
	TestTrue(TEXT("a step along the edge continues"), Along.Equals(FVector2D(0.f, 80.f), 0.1f));

	const FVector2D Slide = SolidGroundMove::ClampPlanar(FVector2D(-30.f, 10.f), FVector2D(100.f, 50.f), 40.f, Shore);
	TestTrue(TEXT("a diagonal keeps the free axis"), Slide.Equals(FVector2D(0.f, 50.f), 0.1f));

	const FVector2D Open = SolidGroundMove::ClampPlanar(FVector2D(-200.f, 0.f), FVector2D(20.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("an open step is unchanged"), Open.Equals(FVector2D(20.f, 0.f), 0.1f));

	const FVector2D Leave = SolidGroundMove::ClampPlanar(FVector2D(40.f, 0.f), FVector2D(-10.f, 5.f), 40.f, Shore);
	TestTrue(TEXT("closed ground keeps only the step back out"), Leave.Equals(FVector2D(-10.f, 0.f), 0.1f));

	const FVector2D Deeper = SolidGroundMove::ClampPlanar(FVector2D(40.f, 0.f), FVector2D(20.f, 0.f), 40.f, Shore);
	TestTrue(TEXT("closed ground refuses a deeper step"), Deeper.IsNearlyZero());

	auto Corner = [](const float X, const float Y)
	{
		return !(X > 0.f && Y > 0.f);
	};
	const FVector2D PastCorner = SolidGroundMove::ClampPlanar(
		FVector2D(-20.f, -20.f), FVector2D(100.f, 80.f), 40.f, Corner);
	TestTrue(TEXT("a corner keeps one axis"), PastCorner.Equals(FVector2D(100.f, 0.f), 0.1f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGroundMoveRampTest,
	"SolidCore1.GroundMove.Ramp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGroundMoveRampTest::RunTest(const FString& Parameters)
{
	SolidBridge::FBridge Span;
	Span.WidthCm = 200.f;
	Span.bBroken = true;
	Span.Deck = {
		FVector(0.f, 0.f, 200.f),
		FVector(600.f, 0.f, 200.f),
	};
	Span.RampFrom = {
		FVector(0.f, 0.f, 200.f),
		FVector(-400.f, 0.f, 100.f),
		FVector(-800.f, 0.f, 0.f),
	};
	Span.RampTo = {
		FVector(600.f, 0.f, 200.f),
		FVector(1000.f, 0.f, 100.f),
		FVector(1400.f, 0.f, 0.f),
	};

	TestTrue(TEXT("the near ramp is a ramp"), SolidGroundMove::OnRamp(Span, -400.f, 0.f));
	TestTrue(TEXT("the far ramp is a ramp"), SolidGroundMove::OnRamp(Span, 1000.f, 0.f));
	TestTrue(TEXT("the lip is a ramp"), SolidGroundMove::OnRamp(Span, 0.f, 0.f));
	TestFalse(TEXT("past the lip is not a ramp"), SolidGroundMove::OnRamp(Span, 50.f, 0.f));
	TestFalse(TEXT("the deck center is not a ramp"), SolidGroundMove::OnRamp(Span, 300.f, 0.f));

	Span.bBroken = false;
	TestTrue(TEXT("a repaired ramp is still a ramp"), SolidGroundMove::OnRamp(Span, 1000.f, 0.f));
	TestFalse(TEXT("a repaired deck is not a ramp"), SolidGroundMove::OnRamp(Span, 300.f, 0.f));
	return true;
}

#endif
