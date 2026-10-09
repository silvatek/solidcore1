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
	TestEqual(TEXT("display name Sam"), Companion->GetCharacterDisplayName(), FString(TEXT("Sam")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionAlexFollowSpacingTest,
	"SolidCore1.Companion.AlexFollowSpacing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionAlexFollowSpacingTest::RunTest(const FString& Parameters)
{
	// Mirrors GameMode spawn spacing: Alex trails further back than Sam.
	constexpr float SamFollow = 280.f;
	constexpr float AlexFollow = 480.f;
	TestTrue(TEXT("Alex further back than Sam"), AlexFollow > SamFollow);

	ASolidCompanionCharacter* Alex = NewObject<ASolidCompanionCharacter>();
	TestNotNull(TEXT("alex"), Alex);
	Alex->FollowDistance = AlexFollow;
	Alex->SideOffset = -100.f;
	Alex->CatchUpDistance = 950.f;
	Alex->SetCharacterDisplayName(TEXT("Alex"));
	TestEqual(TEXT("alex name"), Alex->GetCharacterDisplayName(), FString(TEXT("Alex")));
	TestEqual(TEXT("alex follow distance"), Alex->FollowDistance, AlexFollow);
	TestTrue(TEXT("alex opposite side from Sam default"), Alex->SideOffset < 0.f);
	TestEqual(TEXT("alex label text"), Alex->GetNameLabel()->Text.ToString(), FString(TEXT("Alex")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
