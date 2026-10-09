#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidNameLabel.h"
#include "SolidCharacter.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Components/StaticMeshComponent.h"
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
	TestTrue(TEXT("captain text color differs from companion"),
		SolidNameLabel::TextColorFor(SolidNameLabel::EStyle::Captain)
		!= SolidNameLabel::TextColorFor(SolidNameLabel::EStyle::Companion));
	TestTrue(TEXT("captain background contrasts with companion"),
		SolidNameLabel::BackgroundColorFor(SolidNameLabel::EStyle::Captain)
		!= SolidNameLabel::BackgroundColorFor(SolidNameLabel::EStyle::Companion));
	TestTrue(TEXT("border differs from background (captain)"),
		SolidNameLabel::BorderColorFor(SolidNameLabel::EStyle::Captain)
		!= SolidNameLabel::BackgroundColorFor(SolidNameLabel::EStyle::Captain));
	TestTrue(TEXT("border differs from background (companion)"),
		SolidNameLabel::BorderColorFor(SolidNameLabel::EStyle::Companion)
		!= SolidNameLabel::BackgroundColorFor(SolidNameLabel::EStyle::Companion));
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
	TestNotNull(TEXT("name label root"), Captain->GetNameLabelRoot());
	TestNotNull(TEXT("name label component"), Captain->GetNameLabel());
	TestNotNull(TEXT("border plate"), Captain->GetNameLabelBorder());
	TestNotNull(TEXT("background plate"), Captain->GetNameLabelBackground());
	if (UTextRenderComponent* Label = Captain->GetNameLabel())
	{
		TestEqual(TEXT("label text"), Label->Text.ToString(), FString(TEXT("Outcast")));
		TestEqual(TEXT("highlighted size"), Label->WorldSize, SolidNameLabel::CaptainWorldSize);
	}
	if (UStaticMeshComponent* Border = Captain->GetNameLabelBorder())
	{
		TestNotNull(TEXT("border mesh assigned"), Border->GetStaticMesh().Get());
	}
	if (UStaticMeshComponent* Background = Captain->GetNameLabelBackground())
	{
		TestNotNull(TEXT("background mesh assigned"), Background->GetStaticMesh().Get());
		TestTrue(TEXT("background smaller than border (width)"),
			Background->GetRelativeScale3D().Y < Captain->GetNameLabelBorder()->GetRelativeScale3D().Y);
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
	TestNotNull(TEXT("border plate"), Companion->GetNameLabelBorder());
	TestNotNull(TEXT("background plate"), Companion->GetNameLabelBackground());
	if (UTextRenderComponent* Label = Companion->GetNameLabel())
	{
		TestEqual(TEXT("label text"), Label->Text.ToString(), FString(TEXT("Sam")));
		TestEqual(TEXT("companion size"), Label->WorldSize, SolidNameLabel::CompanionWorldSize);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
