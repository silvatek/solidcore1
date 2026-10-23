#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HUD/SolidPointer.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPointerShapeTest,
	"SolidCore1.Pointer.Shape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPointerShapeTest::RunTest(const FString& Parameters)
{
	const SolidPointer::FTriangle Head = SolidPointer::Triangle(0);

	TestTrue(TEXT("outline is outside the bronze edge"), SolidPointer::OutlinePad > SolidPointer::EdgePad);
	FVector2D Offsets[SolidPointer::OutlineOffsetCount];
	SolidPointer::OutlineOffsets(SolidPointer::OutlinePad, Offsets);
	FVector2D OffsetSum = FVector2D::ZeroVector;
	for (const FVector2D& Offset : Offsets)
	{
		TestTrue(TEXT("outline stamp sits on the rim"), FMath::IsNearlyEqual(Offset.Size(), SolidPointer::OutlinePad, 0.01f));
		OffsetSum += Offset;
	}
	TestTrue(TEXT("outline stamps surround the tip"), OffsetSum.IsNearlyZero(0.01f));

	const FVector2D HeadCenter = (Head.A + Head.B + Head.C) / 3.f;
	float SpanLeft = 0.f;
	float SpanRight = 0.f;
	TestTrue(TEXT("head row crosses the centroid"),
		SolidPointer::SpanAtY(Head, HeadCenter.Y, SpanLeft, SpanRight));
	TestTrue(TEXT("centroid sits inside that row"), HeadCenter.X >= SpanLeft && HeadCenter.X <= SpanRight);
	TestFalse(TEXT("above the tip has no row"),
		SolidPointer::SpanAtY(Head, -1.f, SpanLeft, SpanRight));

	const FVector2D Screen(320.f, 180.f);
	const SolidPointer::FTriangle Placed = SolidPointer::Place(Head, Screen);
	TestTrue(TEXT("drawn tip sits on the mouse"), Placed.A.Equals(Screen));
	TestTrue(TEXT("drawn edge follows the mouse"), Placed.B.Equals(Head.B + Screen));

	TestEqual(TEXT("the pointer is the two head triangles"), SolidPointer::TriangleCount, 2);
	const SolidPointer::FTriangle Other = SolidPointer::Triangle(1);
	TestTrue(TEXT("the second triangle meets the tip"), Other.A.IsNearlyZero(0.01f));
	const FVector2D Vertices[] = { Head.A, Head.B, Head.C, Other.A, Other.B, Other.C };
	for (const FVector2D& Vertex : Vertices)
	{
		TestTrue(TEXT("the tail is not drawn"), Vertex.Y <= Head.B.Y + 0.01f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPointerVisibilityTest,
	"SolidCore1.Pointer.Visibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPointerVisibilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("draw when the mouse is over the view"), SolidPointer::ShouldDraw(true, false));
	TestFalse(TEXT("hide while right-mouse look is held"), SolidPointer::ShouldDraw(true, true));
	TestFalse(TEXT("hide when the mouse has no position"), SolidPointer::ShouldDraw(false, false));
	TestFalse(TEXT("hide when looking and the mouse has no position"), SolidPointer::ShouldDraw(false, true));
	TestFalse(TEXT("free pointer does not look"), SolidPointer::AllowsMouseLook(false));
	TestTrue(TEXT("right mouse looks"), SolidPointer::AllowsMouseLook(true));

	TestFalse(TEXT("button up in both paths"), SolidPointer::IsRightMouseHeld(false, false));
	TestTrue(TEXT("player input sees the button"), SolidPointer::IsRightMouseHeld(true, false));
	TestTrue(TEXT("slate sees the button"), SolidPointer::IsRightMouseHeld(false, true));

	const FVector2D Look = SolidPointer::MouseLookDelta(4.f, 5.f);
	TestTrue(TEXT("yaw follows mouse x"), FMath::IsNearlyEqual(Look.X, 4.f));
	TestTrue(TEXT("pitch negates mouse y"), FMath::IsNearlyEqual(Look.Y, -5.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPointerMouseDragTest,
	"SolidCore1.Pointer.MouseDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPointerMouseDragTest::RunTest(const FString& Parameters)
{
	auto Applied = [](const FVector2D& Pixels, const float YawScale, const float PitchScale)
	{
		const FVector2D Look = SolidPointer::MouseLookDelta(Pixels.X, Pixels.Y);
		return FVector2D(Look.X * YawScale, Look.Y * PitchScale);
	};

	const FVector2D Scaled = SolidPointer::MouseDragPixels(20.f, -10.f, 2.5f, -2.5f);
	TestTrue(TEXT("yaw pixels undo the 2.5 scale"), FMath::IsNearlyEqual(Scaled.X, 8.f));
	TestTrue(TEXT("pitch pixels undo the inverted scale"), FMath::IsNearlyEqual(Scaled.Y, -4.f));
	const FVector2D ScaledApplied = Applied(Scaled, 2.5f, -2.5f);
	TestTrue(TEXT("scaled yaw is the step"), FMath::IsNearlyEqual(ScaledApplied.X, 20.f));
	TestTrue(TEXT("scaled pitch is the step"), FMath::IsNearlyEqual(ScaledApplied.Y, -10.f));

	const FVector2D Level = SolidPointer::MouseDragPixels(0.f, 10.f, 2.5f, -2.5f);
	const FVector2D LevelApplied = Applied(Level, 2.5f, -2.5f);
	TestTrue(TEXT("a downward look levels"), FMath::IsNearlyEqual(LevelApplied.Y, 10.f));

	const FVector2D Unit = SolidPointer::MouseDragPixels(20.f, -10.f, 1.f, 1.f);
	const FVector2D UnitApplied = Applied(Unit, 1.f, 1.f);
	TestTrue(TEXT("unit yaw is the step"), FMath::IsNearlyEqual(UnitApplied.X, 20.f));
	TestTrue(TEXT("unit pitch is the step"), FMath::IsNearlyEqual(UnitApplied.Y, -10.f));

	TestTrue(TEXT("a zero scale sends no pixels"), SolidPointer::MouseDragPixels(20.f, -10.f, 0.f, 0.f).IsNearlyZero());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
