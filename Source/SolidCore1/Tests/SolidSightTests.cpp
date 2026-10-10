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
		TEXT("starts in raven sight"),
		static_cast<uint8>(Character->GetSight()),
		static_cast<uint8>(ESolidSight::Raven));
	TestFalse(TEXT("not true sight"), Character->IsTrueSight());

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
	TestTrue(TEXT("nameplate starts visible"), Nameplate->IsVisible());
	TestFalse(TEXT("body starts visible to the owner"), Mesh->bOwnerNoSee);

	Character->ToggleSight();
	TestTrue(TEXT("true sight"), Character->IsTrueSight());
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
	TestFalse(TEXT("back to raven sight"), Character->IsTrueSight());
	TestFalse(TEXT("body visible to the owner again"), Mesh->bOwnerNoSee);
	TestTrue(TEXT("nameplate visible again"), Nameplate->IsVisible());
	TestTrue(TEXT("arm length restored"), FMath::IsNearlyEqual(Boom->TargetArmLength, SavedZoom));
	TestTrue(TEXT("body yaws toward movement"), Move->bOrientRotationToMovement);
	TestFalse(TEXT("look yaw does not turn the body"), Character->bUseControllerRotationYaw);
	TestTrue(TEXT("boom lag restored"), Boom->bEnableCameraLag);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
