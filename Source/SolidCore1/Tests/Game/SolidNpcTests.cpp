#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Game/SolidNpc.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNpcPlaceTest,
	"SolidCore1.Game.NpcPlace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNpcPlaceTest::RunTest(const FString& Parameters)
{
	const FVector2D Mark(25200.f, 28000.f);
	const FVector2D Place = ASolidNpc::Place(Mark);
	TestTrue(TEXT("the stand keeps the mark's east and west"), FMath::IsNearlyEqual(Place.X, Mark.X));
	TestTrue(
		TEXT("ten metres south"),
		FMath::IsNearlyEqual(Place.Y, Mark.Y - ASolidNpc::SouthOfDragongateCm));
	TestTrue(TEXT("south is one thousand centimetres"), FMath::IsNearlyEqual(ASolidNpc::SouthOfDragongateCm, 1000.f));
	TestTrue(
		TEXT("the face turns north"),
		FMath::IsNearlyEqual(ASolidNpc::FacesCaptainYawDeg, 180.f));
	return true;
}

#endif
