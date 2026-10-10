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
	TestTrue(TEXT("hotspot is the local origin"), SolidPointer::Hotspot().IsNearlyZero());

	const SolidPointer::FTriangle Head = SolidPointer::Triangle(0);
	TestTrue(TEXT("head tip is the hotspot"), Head.A.Equals(SolidPointer::Hotspot()));

	FVector2D BoundsMin = FVector2D::ZeroVector;
	FVector2D BoundsMax = FVector2D::ZeroVector;
	SolidPointer::LocalBounds(BoundsMin, BoundsMax);
	TestTrue(TEXT("tip is the top-left of the arrow"), BoundsMin.IsNearlyZero());
	TestTrue(TEXT("arrow reaches to the right"), BoundsMax.X > 16.f);
	TestTrue(TEXT("tail hangs below the head"), BoundsMax.Y > 28.f);

	const SolidPointer::FTriangle Unit(
		FVector2D(0.f, 0.f), FVector2D(10.f, 0.f), FVector2D(0.f, 10.f));
	TestTrue(TEXT("unit triangle contains a point inside"), SolidPointer::PointInTriangle(FVector2D(1.f, 1.f), Unit));
	TestFalse(TEXT("unit triangle rejects the opposite corner"), SolidPointer::PointInTriangle(FVector2D(9.f, 9.f), Unit));

	const SolidPointer::FTriangle Tail = SolidPointer::Triangle(2);
	const FVector2D TailCenter = (Tail.A + Tail.B + Tail.C) / 3.f;
	TestTrue(TEXT("tail center is part of the pointer"), SolidPointer::CoversLocalPoint(TailCenter));
	TestFalse(TEXT("left of the tip is empty"), SolidPointer::CoversLocalPoint(FVector2D(-2.f, 8.f)));
	TestFalse(TEXT("below the tail is empty"), SolidPointer::CoversLocalPoint(FVector2D(8.f, BoundsMax.Y + 4.f)));

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

	const FVector2D Screen(320.f, 180.f);
	const SolidPointer::FTriangle Placed = SolidPointer::Place(Head, Screen);
	TestTrue(TEXT("drawn tip sits on the mouse"), Placed.A.Equals(Screen));
	TestTrue(TEXT("drawn edge follows the mouse"), Placed.B.Equals(Head.B + Screen));
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
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
