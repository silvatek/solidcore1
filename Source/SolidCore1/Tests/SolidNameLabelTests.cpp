#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidNameLabel.h"
#include "SolidCharacter.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Components/TextRenderComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameLabelStyleTest,
	"SolidCore1.NameLabel.Style",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameLabelStyleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("captain world size larger than companion"),
		SolidNameLabel::WorldSizeFor(SolidNameLabel::EStyle::Captain)
		> SolidNameLabel::WorldSizeFor(SolidNameLabel::EStyle::Companion));
	TestTrue(TEXT("captain color differs from companion"),
		SolidNameLabel::ColorFor(SolidNameLabel::EStyle::Captain)
		!= SolidNameLabel::ColorFor(SolidNameLabel::EStyle::Companion));

	UTextRenderComponent* Label = NewObject<UTextRenderComponent>();
	TestNotNull(TEXT("label"), Label);
	SolidNameLabel::Configure(Label, TEXT("Outcast"), SolidNameLabel::EStyle::Captain, 96.f);
	TestEqual(TEXT("configured text"), Label->Text.ToString(), FString(TEXT("Outcast")));
	TestEqual(TEXT("configured captain size"), Label->WorldSize, SolidNameLabel::CaptainWorldSize);
	TestEqual(TEXT("configured captain color"), Label->TextRenderColor, SolidNameLabel::ColorFor(SolidNameLabel::EStyle::Captain));
	TestEqual(TEXT("height above capsule"), Label->GetRelativeLocation().Z, 96.f + SolidNameLabel::HeightAboveCapsuleCm);

	SolidNameLabel::Configure(Label, TEXT("Sam"), SolidNameLabel::EStyle::Companion, 96.f);
	TestEqual(TEXT("companion text"), Label->Text.ToString(), FString(TEXT("Sam")));
	TestEqual(TEXT("companion size"), Label->WorldSize, SolidNameLabel::CompanionWorldSize);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameLabelCaptainDefaultsTest,
	"SolidCore1.NameLabel.CaptainDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameLabelCaptainDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidCharacter* Captain = NewObject<ASolidCharacter>();
	TestNotNull(TEXT("captain"), Captain);
	TestEqual(TEXT("display name Outcast"), Captain->GetCharacterDisplayName(), FString(TEXT("Outcast")));
	TestNotNull(TEXT("name label component"), Captain->GetNameLabel());
	if (UTextRenderComponent* Label = Captain->GetNameLabel())
	{
		TestEqual(TEXT("label text"), Label->Text.ToString(), FString(TEXT("Outcast")));
		TestEqual(TEXT("highlighted size"), Label->WorldSize, SolidNameLabel::CaptainWorldSize);
	}

	Captain->SetCharacterDisplayName(TEXT("Scout"));
	TestEqual(TEXT("renamed"), Captain->GetCharacterDisplayName(), FString(TEXT("Scout")));
	TestEqual(TEXT("label follows rename"), Captain->GetNameLabel()->Text.ToString(), FString(TEXT("Scout")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidNameLabelCompanionDefaultsTest,
	"SolidCore1.NameLabel.CompanionDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidNameLabelCompanionDefaultsTest::RunTest(const FString& Parameters)
{
	ASolidCompanionCharacter* Companion = NewObject<ASolidCompanionCharacter>();
	TestNotNull(TEXT("companion"), Companion);
	TestEqual(TEXT("display name Sam"), Companion->GetCharacterDisplayName(), FString(TEXT("Sam")));
	TestNotNull(TEXT("name label component"), Companion->GetNameLabel());
	if (UTextRenderComponent* Label = Companion->GetNameLabel())
	{
		TestEqual(TEXT("label text"), Label->Text.ToString(), FString(TEXT("Sam")));
		TestEqual(TEXT("companion size"), Label->WorldSize, SolidNameLabel::CompanionWorldSize);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
