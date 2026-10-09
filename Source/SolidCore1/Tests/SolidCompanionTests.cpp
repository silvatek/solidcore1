#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Companion/SolidCompanionCharacter.h"
#include "SolidCharacter.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionDefaultsTest,
	"SolidCore1.Companion.Defaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidCompanionCharacter* Companion = NewObject<ASolidCompanionCharacter>();
	TestNotNull(TEXT("companion"), Companion);
	TestEqual(TEXT("follow distance"), Companion->FollowDistance, 280.f);
	TestEqual(TEXT("acceptance radius"), Companion->AcceptanceRadius, 90.f);
	TestEqual(TEXT("catch-up distance"), Companion->CatchUpDistance, 700.f);
	TestEqual(TEXT("walk speed"), Companion->WalkSpeed, 500.f);
	TestEqual(TEXT("catch-up speed"), Companion->CatchUpSpeed, 900.f);
	TestTrue(TEXT("match follow target speed"), Companion->bMatchFollowTargetSpeed);
	TestNull(TEXT("no follow target yet"), Companion->GetFollowTarget());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionFollowTargetTest,
	"SolidCore1.Companion.SetFollowTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionFollowTargetTest::RunTest(const FString& Parameters)
{
	ASolidCompanionCharacter* Companion = NewObject<ASolidCompanionCharacter>();
	ASolidCharacter* Captain = NewObject<ASolidCharacter>();
	TestNotNull(TEXT("companion"), Companion);
	TestNotNull(TEXT("captain"), Captain);

	Companion->SetFollowTarget(Captain);
	TestEqual(TEXT("follow target is captain"), Companion->GetFollowTarget(), static_cast<AActor*>(Captain));

	Companion->SetFollowTarget(nullptr);
	TestNull(TEXT("cleared follow target"), Companion->GetFollowTarget());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
