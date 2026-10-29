#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
#include "SolidTerrainFog.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightToggleTest,
	"SolidCore1.Sight.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightToggleTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("raven label"), FString(SolidSight::Label(ESolidSight::Raven)), FString(TEXT("Raven sight")));
	TestEqual(TEXT("true label"), FString(SolidSight::Label(ESolidSight::True)), FString(TEXT("True sight")));
	TestEqual(
		TEXT("toggle leaves raven"),
		static_cast<uint8>(SolidSight::Toggle(ESolidSight::Raven)),
		static_cast<uint8>(ESolidSight::True));
	TestEqual(
		TEXT("toggle returns to raven"),
		static_cast<uint8>(SolidSight::Toggle(ESolidSight::True)),
		static_cast<uint8>(ESolidSight::Raven));
	TestTrue(TEXT("right stick click toggles sight"), SolidSight::ControllerToggleKey() == EKeys::Gamepad_RightThumbstick);
	TestFalse(TEXT("A stays jump"), SolidSight::ControllerToggleKey() == EKeys::Gamepad_FaceButton_Bottom);

	ASolidCharacter* Character = NewObject<ASolidCharacter>();
	TestNotNull(TEXT("character"), Character);
	TestEqual(
		TEXT("starts in true sight"),
		static_cast<uint8>(Character->GetSight()),
		static_cast<uint8>(ESolidSight::True));
	TestTrue(TEXT("true sight"), Character->IsTrueSight());

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	USpringArmComponent* Boom = Character->GetCameraBoom();
	USceneComponent* Nameplate = Character->GetNameLabelRoot();
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	TestNotNull(TEXT("mesh"), Mesh);
	TestNotNull(TEXT("boom"), Boom);
	TestNotNull(TEXT("nameplate"), Nameplate);
	TestNotNull(TEXT("movement"), Move);
	if (!Mesh || !Boom || !Nameplate || !Move)
	{
		return false;
	}

	const float SavedZoom = Character->GetUserZoomArmLength();
	TestTrue(TEXT("body hidden from the owner"), Mesh->bOwnerNoSee);
	TestTrue(TEXT("hidden body still casts a shadow"), Mesh->bCastHiddenShadow);
	TestFalse(TEXT("nameplate hidden"), Nameplate->IsVisible());
	TestTrue(TEXT("arm length is zero"), FMath::IsNearlyZero(Boom->TargetArmLength));
	TestTrue(TEXT("camera at eye height"),
		FMath::IsNearlyEqual(Boom->TargetOffset.Z, SolidSight::TrueSightEyeHeightCm));
	TestTrue(TEXT("camera sits forward of the capsule"),
		FMath::IsNearlyEqual(Boom->SocketOffset.X, SolidSight::TrueSightForwardCm));
	TestFalse(TEXT("boom lag off"), Boom->bEnableCameraLag);
	TestFalse(TEXT("boom collision off"), Boom->bDoCollisionTest);
	TestFalse(TEXT("body does not yaw toward movement"), Move->bOrientRotationToMovement);
	TestTrue(TEXT("body follows look yaw"), Character->bUseControllerRotationYaw);
	TestTrue(TEXT("stored zoom unchanged"),
		FMath::IsNearlyEqual(Character->GetUserZoomArmLength(), SavedZoom));

	Character->ToggleSight();
	TestFalse(TEXT("raven sight"), Character->IsTrueSight());
	TestFalse(TEXT("body visible to the owner"), Mesh->bOwnerNoSee);
	TestFalse(TEXT("nameplate stays hidden without companions"), Nameplate->IsVisible());
	TestTrue(TEXT("arm length restored"), FMath::IsNearlyEqual(Boom->TargetArmLength, SavedZoom));
	TestTrue(TEXT("body yaws toward movement"), Move->bOrientRotationToMovement);
	TestFalse(TEXT("look yaw does not turn the body"), Character->bUseControllerRotationYaw);
	TestTrue(TEXT("boom lag restored"), Boom->bEnableCameraLag);

	Character->ToggleSight();
	TestTrue(TEXT("back to true sight"), Character->IsTrueSight());
	TestTrue(TEXT("body hidden again"), Mesh->bOwnerNoSee);
	TestFalse(TEXT("nameplate hidden again"), Nameplate->IsVisible());
	TestTrue(TEXT("arm length is zero again"), FMath::IsNearlyZero(Boom->TargetArmLength));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightMarkerTest,
	"SolidCore1.Sight.Marker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightMarkerTest::RunTest(const FString& Parameters)
{
	const FString Eye(SolidSight::Marker(ESolidSight::True));
	const FString Eagle(SolidSight::Marker(ESolidSight::Raven));
	TestEqual(TEXT("true sight is an eye"), Eye, FString(TEXT("\U0001F441")));
	TestEqual(TEXT("raven sight is an eagle"), Eagle, FString(TEXT("\U0001F985")));
	TestFalse(TEXT("the two markers differ"), Eye.Equals(Eagle));
	TestEqual(TEXT("falcon uses the eagle"), FString(SolidSight::Marker(ESolidSight::Falcon)), Eagle);
	TestEqual(TEXT("eagle sight uses the eagle"), FString(SolidSight::Marker(ESolidSight::Eagle)), Eagle);
	TestEqual(TEXT("raven has no mark behind the bird"), FString(SolidSight::Mark(ESolidSight::Raven)), FString());
	TestEqual(TEXT("true sight has no mark behind the eye"), FString(SolidSight::Mark(ESolidSight::True)), FString());
	TestEqual(TEXT("falcon mark is a plus"), FString(SolidSight::Mark(ESolidSight::Falcon)), FString(TEXT("+")));
	TestEqual(TEXT("eagle mark is the globe"), FString(SolidSight::Mark(ESolidSight::Eagle)), FString(TEXT("\U0001F310")));
	TestTrue(TEXT("the globe is larger than the plus"),
		SolidSight::MarkFontSize(ESolidSight::Eagle) > SolidSight::MarkFontSize(ESolidSight::Falcon));
	TestTrue(TEXT("falcon widens the oval"),
		SolidSight::ChipWidth(ESolidSight::Falcon) > SolidSight::ChipWidth(ESolidSight::Raven));
	TestTrue(TEXT("eagle widens the oval"),
		SolidSight::ChipWidth(ESolidSight::Eagle) > SolidSight::ChipWidth(ESolidSight::True));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightIndicatorTest,
	"SolidCore1.Sight.Indicator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightIndicatorTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("raven sight locked hides the indicator"),
		SolidSight::ShowsIndicator(false, false));
	TestFalse(
		TEXT("character editor hides it while raven sight is locked"),
		SolidSight::ShowsIndicator(false, true));
	TestTrue(
		TEXT("raven sight unlocked shows the indicator"),
		SolidSight::ShowsIndicator(true, false));
	TestFalse(
		TEXT("character editor hides it after raven sight unlocks"),
		SolidSight::ShowsIndicator(true, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightHorizontalViewTest,
	"SolidCore1.Sight.HorizontalView",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightHorizontalViewTest::RunTest(const FString& Parameters)
{
	const FRotator LookingDown(-35.5f, 120.f, 4.f);
	const FRotator TrueView = SolidSight::ViewForSight(ESolidSight::True, LookingDown);
	TestTrue(TEXT("true sight look is level"), FMath::IsNearlyZero(TrueView.Pitch, 0.1f));
	TestTrue(TEXT("true sight yaw stays"), FMath::IsNearlyEqual(TrueView.Yaw, LookingDown.Yaw, 0.1f));
	TestTrue(TEXT("true sight roll is level"), FMath::IsNearlyZero(TrueView.Roll, 0.1f));

	const FRotator RavenView = SolidSight::ViewForSight(ESolidSight::Raven, LookingDown);
	TestTrue(TEXT("raven sight keeps the downward look"), FMath::IsNearlyEqual(RavenView.Pitch, LookingDown.Pitch, 0.1f));
	TestTrue(TEXT("raven sight keeps the yaw"), FMath::IsNearlyEqual(RavenView.Yaw, LookingDown.Yaw, 0.1f));
	TestTrue(TEXT("raven sight keeps the roll"), FMath::IsNearlyEqual(RavenView.Roll, LookingDown.Roll, 0.1f));

	const FRotator AlreadyLevel(0.f, -40.f, 0.f);
	const FRotator StillLevel = SolidSight::ViewForSight(ESolidSight::True, AlreadyLevel);
	TestTrue(TEXT("a level look stays level"), FMath::IsNearlyZero(StillLevel.Pitch, 0.1f));
	TestTrue(TEXT("a level yaw stays"), FMath::IsNearlyEqual(StillLevel.Yaw, AlreadyLevel.Yaw, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightCycleTest,
	"SolidCore1.Sight.Cycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightCycleTest::RunTest(const FString& Parameters)
{
	auto Sight = [](const ESolidSight Mode)
	{
		return static_cast<uint8>(Mode);
	};

	SolidSight::FAvailable Pair;
	Pair.bRaven = true;
	TestEqual(TEXT("true sight steps to raven"), Sight(SolidSight::Cycle(ESolidSight::True, Pair)), Sight(ESolidSight::Raven));
	TestEqual(TEXT("raven sight steps to true"), Sight(SolidSight::Cycle(ESolidSight::Raven, Pair)), Sight(ESolidSight::True));

	const SolidSight::FAvailable TrueOnly;
	TestEqual(
		TEXT("a locked raven sight is skipped"),
		Sight(SolidSight::Cycle(ESolidSight::True, TrueOnly)),
		Sight(ESolidSight::True));

	SolidSight::FAvailable All;
	All.bRaven = true;
	All.bFalcon = true;
	All.bEagle = true;
	TestEqual(TEXT("falcon follows raven"), Sight(SolidSight::Cycle(ESolidSight::Raven, All)), Sight(ESolidSight::Falcon));
	TestEqual(TEXT("eagle follows falcon"), Sight(SolidSight::Cycle(ESolidSight::Falcon, All)), Sight(ESolidSight::Eagle));
	TestEqual(TEXT("true sight follows eagle"), Sight(SolidSight::Cycle(ESolidSight::Eagle, All)), Sight(ESolidSight::True));

	SolidSight::FAvailable SkipFalcon = All;
	SkipFalcon.bFalcon = false;
	TestEqual(
		TEXT("a locked falcon sight is skipped"),
		Sight(SolidSight::Cycle(ESolidSight::Raven, SkipFalcon)),
		Sight(ESolidSight::Eagle));

	TestEqual(TEXT("falcon label"), FString(SolidSight::Label(ESolidSight::Falcon)), FString(TEXT("Falcon sight")));
	TestTrue(TEXT("low zoom is 20 m"), FMath::IsNearlyEqual(SolidSight::FalconHeightCm(0.f), 2000.f));
	TestTrue(TEXT("high zoom is 200 m"), FMath::IsNearlyEqual(SolidSight::FalconHeightCm(1.f), 20000.f));
	TestTrue(TEXT("falcon stands at 80% of the clear radius"),
		FMath::IsNearlyEqual(
			SolidSight::FalconMaxOffsetCm,
			SolidTerrainFog::MetersToCm(SolidTerrainFog::ClearRadiusMeters) * SolidSight::FalconClearFraction));
	const FVector2D West = SolidSight::FalconHorizontalOffset(FRotator(0.f, 0.f, 0.f));
	TestTrue(TEXT("looking east keeps the camera west"), FMath::IsNearlyEqual(West.X, -SolidSight::FalconMaxOffsetCm, 1.f));
	TestTrue(TEXT("looking east stays on the captain's Y"), FMath::IsNearlyZero(West.Y, 1.f));
	TestTrue(TEXT("the west offset is the falcon cap"), FMath::IsNearlyEqual(West.Size(), SolidSight::FalconMaxOffsetCm, 1.f));
	const FVector2D Overhead = SolidSight::FalconHorizontalOffset(FRotator(-90.f, 0.f, 0.f));
	TestTrue(TEXT("a straight-down look stays over the captain"), Overhead.IsNearlyZero(1.f));
	const FVector2D Diagonal = SolidSight::FalconHorizontalOffset(FRotator(-30.f, 45.f, 0.f));
	TestTrue(TEXT("a diagonal stay is the falcon cap"), FMath::IsNearlyEqual(Diagonal.Size(), SolidSight::FalconMaxOffsetCm, 1.f));
	const FRotator AtCaptain = SolidSight::FalconLookAtCaptain(
		FVector(0.f, -SolidSight::FalconMaxOffsetCm, SolidSight::FalconMaxOffsetCm), FVector::ZeroVector);
	TestTrue(TEXT("the entry look faces the captain"), FMath::IsNearlyEqual(AtCaptain.Yaw, 90.f, 1.f));
	TestTrue(TEXT("the entry look pitches down"), FMath::IsNearlyEqual(AtCaptain.Pitch, -45.f, 1.f));
	TestTrue(TEXT("the entry look is level in roll"), FMath::IsNearlyZero(AtCaptain.Roll, 1.f));
	TestEqual(TEXT("eagle label"), FString(SolidSight::Label(ESolidSight::Eagle)), FString(TEXT("Eagle sight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSightEagleCameraTest,
	"SolidCore1.Sight.EagleCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSightEagleCameraTest::RunTest(const FString& Parameters)
{
	const FVector2D Min(-51200.f, -51200.f);
	const FVector2D Max(51200.f, 51200.f);
	const FVector2D Centre = SolidSight::WorldCentre(Min, Max);
	TestTrue(TEXT("the world centre is the origin"), Centre.IsNearlyZero(1.f));

	const FVector2D Captain(20000.f, 0.f);
	const FVector2D OnCentre = SolidSight::EagleGroundXY(
		Captain, Centre, 200.f, [](const FVector2D&) { return 0.f; });
	TestTrue(TEXT("clear fog stands on the centre"), OnCentre.Equals(Centre, 1.f));

	const float ClearReach = 2500.f;
	const FVector2D Edge = SolidSight::EagleGroundXY(
		Captain,
		Centre,
		200.f,
		[Captain, ClearReach](const FVector2D& At)
		{
			return FVector2D::Distance(At, Captain) <= ClearReach ? 0.f : 1.f;
		});
	TestTrue(
		TEXT("fog stops the camera near the clear edge"),
		FMath::IsNearlyEqual(FVector2D::Distance(Edge, Captain), ClearReach, 200.f));
	TestTrue(
		TEXT("that edge is nearer the centre than the captain"),
		FVector2D::Distance(Edge, Centre) < FVector2D::Distance(Captain, Centre) - 1.f);
	TestTrue(TEXT("the edge stays on the line"), FMath::IsNearlyZero(Edge.Y, 1.f));

	const float Height = SolidSight::EagleHeightCm(Centre, Min, Max, 90.f, 16.f / 9.f);
	const float Expected = 51200.f * (16.f / 9.f);
	TestTrue(TEXT("the centre height fits the square"), FMath::IsNearlyEqual(Height, Expected, 1.f));

	const float CornerHeight = SolidSight::EagleHeightCm(Min, Min, Max, 90.f, 16.f / 9.f);
	TestTrue(TEXT("a corner needs more height"), CornerHeight > Height);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
