#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidClipLocomotion.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidClipLocomotionSelectClipTest,
	"SolidCore1.Clip.SelectClip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidClipLocomotionSelectClipTest::RunTest(const FString& Parameters)
{
	using SolidClipLocomotion::EClip;
	using SolidClipLocomotion::SelectClip;

	TestTrue(TEXT("idle at rest"),
		SelectClip(/*Speed=*/0.f, /*WalkTh=*/30.f, /*bPreferRun=*/false, /*bInAir=*/false, /*bAllowJump=*/true)
		== EClip::Idle);

	TestTrue(TEXT("walk above walk threshold"),
		SelectClip(50.f, 30.f, false, false, true) == EClip::Walk);

	TestTrue(TEXT("prefer-run wins over walk speed"),
		SelectClip(50.f, 30.f, /*bPreferRun=*/true, false, true) == EClip::Run);

	TestTrue(TEXT("jump in air when allowed"),
		SelectClip(0.f, 30.f, false, /*bInAir=*/true, /*bAllowJump=*/true) == EClip::Jump);

	TestTrue(TEXT("no jump clip => idle in air at rest"),
		SelectClip(0.f, 30.f, false, true, /*bAllowJump=*/false) == EClip::Idle);

	TestTrue(TEXT("no jump clip => walk in air if moving"),
		SelectClip(50.f, 30.f, false, true, false) == EClip::Walk);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
