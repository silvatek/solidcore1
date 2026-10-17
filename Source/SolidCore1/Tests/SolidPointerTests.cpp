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

#endif // WITH_DEV_AUTOMATION_TESTS
