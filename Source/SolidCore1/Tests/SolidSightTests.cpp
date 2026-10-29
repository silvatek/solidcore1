#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCharacter.h"
#include "SolidSight.h"
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
	TestEqual(TEXT("eagle label"), FString(SolidSight::Label(ESolidSight::Eagle)), FString(TEXT("Eagle sight")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
