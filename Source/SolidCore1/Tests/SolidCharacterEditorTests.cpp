#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidCharacterEditor.h"
#include "Menus/SolidMainMenu.h"
#include "Party/SolidCompany.h"
#include "SolidBodyMesh.h"
#include "SolidHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorMenuTest,
	"SolidCore1.CharacterEditor.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorMenuTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TestEqual(TEXT("seven entries"), Entries.Num(), 7);
	TestEqual(TEXT("entry count"), SolidMainMenu::EntryCount(), 7);

	FSolidMainMenuEntry Found;
	TestTrue(TEXT("seventh is Character editor"), SolidMainMenu::FindEntry(6, Found));
	TestEqual(TEXT("editor label"), Found.Label, FString(TEXT("Character editor")));
	TestEqual(
		TEXT("editor item"),
		static_cast<uint8>(Found.Item),
		static_cast<uint8>(ESolidMainMenuItem::CharacterEditor));
	TestEqual(
		TEXT("editor action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Found.Item)),
		static_cast<uint8>(ESolidMainMenuAction::OpenCharacterEditor));

	TestEqual(TEXT("wrap from the editor"), SolidMainMenu::WrapIndex(6, 1), 0);
	TestEqual(TEXT("wrap up onto the editor"), SolidMainMenu::WrapIndex(0, -1), 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorHudTest,
	"SolidCore1.CharacterEditor.Hud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorHudTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	HUD->OpenCharacterEditor();
	TestTrue(TEXT("editor opens"), HUD->IsCharacterEditorOpen());
	TestFalse(TEXT("editor closes the menu"), HUD->IsMainMenuOpen());
	TestEqual(TEXT("editor starts on the first companion"), HUD->GetCharacterEditorIndex(), 0);

	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the editor"), HUD->IsCharacterEditorOpen());
	TestFalse(TEXT("F10 returns to the world"), HUD->IsMainMenuOpen());

	HUD->OpenCharacterEditor();
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the editor"), HUD->IsCharacterEditorOpen());
	TestFalse(TEXT("Esc returns to the world"), HUD->IsMainMenuOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorRosterTest,
	"SolidCore1.CharacterEditor.Roster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorRosterTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("wrap forward"), SolidCharacterEditor::WrapIndex(4, 1, 5), 0);
	TestEqual(TEXT("wrap back"), SolidCharacterEditor::WrapIndex(0, -1, 5), 4);
	TestEqual(TEXT("empty roster stays put"), SolidCharacterEditor::WrapIndex(3, 1, 0), 0);

	USolidCompany* Company = NewObject<USolidCompany>();
	TestNotNull(TEXT("company"), Company);
	if (!Company)
	{
		return false;
	}
	Company->InitializeDefaultCompanions();

	const FSolidCompanion* Sam = Company->GetCompanion(Company->FindCompanionIndexByName(TEXT("Sam")));
	const FSolidCompanion* Alex = Company->GetCompanion(Company->FindCompanionIndexByName(TEXT("Alex")));
	const FSolidCompanion* Max = Company->GetCompanion(Company->FindCompanionIndexByName(TEXT("Max")));
	TestNotNull(TEXT("sam"), Sam);
	TestNotNull(TEXT("alex"), Alex);
	TestNotNull(TEXT("max"), Max);
	if (!Sam || !Alex || !Max)
	{
		return false;
	}

	TestTrue(TEXT("Sam shirt is red"), Sam->TorsoColor.Equals(SolidCompanionColors::ShirtRed()));
	TestTrue(TEXT("Sam trousers are blue"), Sam->LegColor.Equals(SolidCompanionColors::TrouserBlue()));
	TestTrue(TEXT("Sam skin is pink"), Sam->SkinColor.Equals(SolidCompanionColors::SkinPink()));
	TestTrue(TEXT("Alex shirt is blue"), Alex->TorsoColor.Equals(SolidCompanionColors::TrouserBlue()));
	TestTrue(TEXT("Alex trousers are red"), Alex->LegColor.Equals(SolidCompanionColors::ShirtRed()));
	TestTrue(TEXT("Alex skin is pink"), Alex->SkinColor.Equals(SolidCompanionColors::SkinPink()));
	TestTrue(TEXT("Max skin is pink"), Max->SkinColor.Equals(SolidCompanionColors::SkinPink()));

	FLinearColor PaletteTorso;
	FLinearColor PaletteLegs;
	SolidCompanionColors::PaletteForName(TEXT("Max"), PaletteTorso, PaletteLegs);
	TestTrue(TEXT("Max shirt is the generated palette"), Max->TorsoColor.Equals(PaletteTorso));

	TArray<FString> Lines;
	SolidCharacterEditor::Describe(*Sam, Lines);
	const FSolidBodyParams Params = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	TestTrue(TEXT("name"), Lines.Contains(FString(TEXT("Sam"))));
	TestTrue(TEXT("vocation"), Lines.Contains(FString(TEXT("Rogue"))));
	TestTrue(TEXT("height"), Lines.Contains(FString::Printf(TEXT("Height %.2f"), Params.Height)));
	TestTrue(TEXT("shoulders"), Lines.Contains(FString::Printf(TEXT("Shoulder width %.2f"), Params.ShoulderWidth)));
	TestTrue(TEXT("hips"), Lines.Contains(FString::Printf(TEXT("Hip width %.2f"), Params.HipWidth)));
	TestTrue(TEXT("girth"), Lines.Contains(FString::Printf(TEXT("Torso girth %.2f"), Params.TorsoGirth)));
	TestTrue(TEXT("arms"), Lines.Contains(FString::Printf(TEXT("Arm length %.2f"), Params.ArmLength)));
	TestTrue(TEXT("legs"), Lines.Contains(FString::Printf(TEXT("Leg length %.2f"), Params.LegLength)));
	TestTrue(TEXT("head"), Lines.Contains(FString::Printf(TEXT("Head size %.2f"), Params.HeadSize)));
	TestTrue(TEXT("neck"), Lines.Contains(FString::Printf(TEXT("Neck length %.2f"), Params.NeckLength)));
	TestTrue(TEXT("bulk"), Lines.Contains(FString::Printf(TEXT("Bulk %.2f"), Params.Bulk)));
	TestTrue(
		TEXT("shirt numbers"),
		Lines.Contains(FString::Printf(
			TEXT("Shirt %.2f %.2f %.2f"),
			Sam->TorsoColor.R,
			Sam->TorsoColor.G,
			Sam->TorsoColor.B)));
	TestTrue(
		TEXT("trouser numbers"),
		Lines.Contains(FString::Printf(
			TEXT("Trousers %.2f %.2f %.2f"),
			Sam->LegColor.R,
			Sam->LegColor.G,
			Sam->LegColor.B)));
	TestTrue(
		TEXT("skin numbers"),
		Lines.Contains(FString::Printf(
			TEXT("Skin %.2f %.2f %.2f"),
			Sam->SkinColor.R,
			Sam->SkinColor.G,
			Sam->SkinColor.B)));
	TestTrue(TEXT("cycle hint"), Lines.Contains(FString(TEXT("Left / Right"))));
	TestTrue(TEXT("return hint"), Lines.Contains(FString(TEXT("F10  open world"))));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
