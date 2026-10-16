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

#endif // WITH_DEV_AUTOMATION_TESTS
