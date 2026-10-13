#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidCharacterEditor.h"
#include "Menus/SolidMainMenu.h"
#include "Party/SolidCompany.h"
#include "SolidBodyMesh.h"
#include "SolidBodyParams.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorSliderTest,
	"SolidCore1.CharacterEditor.Sliders",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorSliderTest::RunTest(const FString& Parameters)
{
	FSolidBodyParams Seed;
	Seed.Height = 1.f;
	Seed.ShoulderWidth = 1.f;
	Seed.HipWidth = 1.f;
	Seed.TorsoGirth = 1.f;
	Seed.ArmLength = 1.f;
	Seed.LegLength = 1.f;
	Seed.HeadSize = 1.f;
	Seed.NeckLength = 1.f;
	Seed.Bulk = 1.f;

	const FSolidBodyControls Neutral = SolidBodyControls::FromSeed(Seed);
	TestTrue(TEXT("neutral controls match the seed"), SolidBodyControls::Resolve(Seed, Neutral).ExactEquals(Seed));

	const FSolidBodyParams SamSeed = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	TestTrue(
		TEXT("Sam's default sliders match his seed"),
		SolidBodyControls::Resolve(SamSeed, SolidBodyControls::FromSeed(SamSeed)).ExactEquals(SamSeed));

	FSolidBodyControls Masculine = Neutral;
	Masculine.Gender = 0.f;
	const FSolidBodyParams Man = SolidBodyControls::Resolve(Seed, Masculine);
	TestTrue(TEXT("masculine shoulders widen"), Man.ShoulderWidth > Seed.ShoulderWidth);
	TestTrue(TEXT("masculine hips narrow"), Man.HipWidth < Seed.HipWidth);

	FSolidBodyControls Feminine = Neutral;
	Feminine.Gender = 1.f;
	const FSolidBodyParams Woman = SolidBodyControls::Resolve(Seed, Feminine);
	TestTrue(TEXT("feminine shoulders narrow"), Woman.ShoulderWidth < Seed.ShoulderWidth);
	TestTrue(TEXT("feminine hips widen"), Woman.HipWidth > Seed.HipWidth);

	FSolidBodyControls Heavier = Neutral;
	Heavier.Build = 1.2f;
	const FSolidBodyParams Built = SolidBodyControls::Resolve(Seed, Heavier);
	TestTrue(TEXT("build sets bulk"), FMath::IsNearlyEqual(Built.Bulk, 1.2f));
	TestTrue(TEXT("build scales torso girth"), FMath::IsNearlyEqual(Built.TorsoGirth, 1.2f));
	TestTrue(TEXT("build leaves arm length"), FMath::IsNearlyEqual(Built.ArmLength, Seed.ArmLength));

	FSolidBodyControls Nudged = Neutral;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, 1);
	TestTrue(TEXT("right raises height"), FMath::IsNearlyEqual(Nudged.Height, 1.05f, 0.001f));
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, -1);
	TestTrue(TEXT("left lowers height"), FMath::IsNearlyEqual(Nudged.Height, 1.f, 0.001f));
	Nudged.Height = SolidBody::ParamMin;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, -1);
	TestTrue(TEXT("height stops at the low end"), FMath::IsNearlyEqual(Nudged.Height, SolidBody::ParamMin));
	Nudged.Gender = 1.f;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Gender, 1);
	TestTrue(TEXT("gender stops at 1"), FMath::IsNearlyEqual(Nudged.Gender, 1.f));

	TestEqual(TEXT("up wraps onto gender"), SolidBodyControls::WrapSlider(0, -1), 2);
	TestEqual(TEXT("down wraps onto height"), SolidBodyControls::WrapSlider(2, 1), 0);
	TestEqual(TEXT("height label"), FString(SolidBodyControls::Label(SolidBodyControls::Height)), FString(TEXT("Height")));
	TestEqual(TEXT("build label"), FString(SolidBodyControls::Label(SolidBodyControls::Build)), FString(TEXT("Build")));
	TestEqual(TEXT("gender label"), FString(SolidBodyControls::Label(SolidBodyControls::Gender)), FString(TEXT("Gender")));

	TestTrue(TEXT("fraction at the low end"), FMath::IsNearlyEqual(SolidBodyControls::Fraction(SolidBodyControls::Height, SolidBody::ParamMin), 0.f));
	TestTrue(TEXT("fraction at the high end"), FMath::IsNearlyEqual(SolidBodyControls::Fraction(SolidBodyControls::Height, SolidBody::ParamMax), 1.f));
	TestTrue(
		TEXT("mid bar is the mid height"),
		FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 0.5f), 1.05f, 0.001f));
	TestTrue(TEXT("gender bar starts at 0"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Gender, 0.f), 0.f));
	TestTrue(TEXT("gender bar ends at 1"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Gender, 1.f), 1.f));
	TestTrue(
		TEXT("mouse step quantizes"),
		FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 0.505f), 1.05f, 0.001f));

	USolidCompany* Company = NewObject<USolidCompany>();
	TestNotNull(TEXT("company"), Company);
	if (!Company)
	{
		return false;
	}
	Company->InitializeDefaultCompanions();
	const int32 SamIndex = Company->FindCompanionIndexByName(TEXT("Sam"));
	const FSolidCompanion* Sam = Company->GetCompanion(SamIndex);
	TestNotNull(TEXT("sam"), Sam);
	if (!Sam)
	{
		return false;
	}
	TestTrue(TEXT("catalog height is the seed"), FMath::IsNearlyEqual(Sam->BodyHeight, SamSeed.Height));
	TestTrue(TEXT("catalog build is the seed bulk"), FMath::IsNearlyEqual(Sam->BodyBuild, SamSeed.Bulk));
	TestTrue(TEXT("catalog gender is neutral"), FMath::IsNearlyEqual(Sam->BodyGender, SolidBodyControls::GenderNeutral));

	TArray<FString> Lines;
	SolidCharacterEditor::Describe(*Sam, Lines);
	TestTrue(TEXT("page up switches companions"), Lines.Contains(FString(TEXT("Page Up / Page Down"))));
	TestTrue(TEXT("up and down select a slider"), Lines.Contains(FString(TEXT("Up / Down"))));
	TestTrue(TEXT("left and right still named"), Lines.Contains(FString(TEXT("Left / Right"))));
	TestTrue(TEXT("build line"), Lines.Contains(FString::Printf(TEXT("Build %.2f"), SamSeed.Bulk)));
	TestTrue(TEXT("gender line"), Lines.Contains(FString(TEXT("Gender 0.50"))));
	TestEqual(TEXT("height line is a slider"), SolidCharacterEditor::SliderForLine(TEXT("Height 1.00")), SolidBodyControls::Height);
	TestEqual(TEXT("head size is not a slider"), SolidCharacterEditor::SliderForLine(TEXT("Head size 1.00")), INDEX_NONE);

	const float NextHeight = FMath::IsNearlyEqual(Sam->BodyHeight, SolidBody::ParamMin)
		? SolidBody::ParamMax
		: SolidBody::ParamMin;
	TestTrue(TEXT("store a new height"), Company->SetCompanionBodyControls(SamIndex, NextHeight, Sam->BodyBuild, Sam->BodyGender));
	TestFalse(TEXT("same controls do not write"), Company->SetCompanionBodyControls(SamIndex, NextHeight, Sam->BodyBuild, Sam->BodyGender));
	TestFalse(TEXT("missing companion does not write"), Company->SetCompanionBodyControls(99, NextHeight, 1.f, 0.5f));
	Sam = Company->GetCompanion(SamIndex);
	TestNotNull(TEXT("sam remains"), Sam);
	if (!Sam)
	{
		return false;
	}
	Lines.Reset();
	SolidCharacterEditor::Describe(*Sam, Lines);
	TestTrue(TEXT("panel shows the new height"), Lines.Contains(FString::Printf(TEXT("Height %.2f"), NextHeight)));

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}
	HUD->OpenCharacterEditor();
	TestEqual(TEXT("editor opens on height"), HUD->GetCharacterEditorSlider(), 0);
	HUD->MoveCharacterEditorSlider(1);
	TestEqual(TEXT("down selects build"), HUD->GetCharacterEditorSlider(), SolidBodyControls::Build);
	HUD->MoveCharacterEditorSlider(-1);
	HUD->MoveCharacterEditorSlider(-1);
	TestEqual(TEXT("up wraps to gender"), HUD->GetCharacterEditorSlider(), SolidBodyControls::Gender);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorSliderHitTest,
	"SolidCore1.CharacterEditor.SliderHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorSliderHitTest::RunTest(const FString& Parameters)
{
	const FSolidSliderRow Height = SolidCharacterEditor::MakeSliderRow(SolidBodyControls::Height, 16.f, 40.f, 14.f, 120.f);
	const FSolidSliderRow Build = SolidCharacterEditor::MakeSliderRow(SolidBodyControls::Build, 16.f, 56.f, 14.f, 120.f);
	const FSolidSliderRow Rows[] = { Height, Build };

	TestTrue(TEXT("track sits past the text"), Height.Bar.X > 120.f);
	TestTrue(TEXT("left of the track is 0"), FMath::IsNearlyEqual(SolidCharacterEditor::BarFraction(Height.Bar, Height.Bar.X), 0.f));
	TestTrue(TEXT("right of the track is 1"), FMath::IsNearlyEqual(SolidCharacterEditor::BarFraction(Height.Bar, Height.Bar.X + Height.Bar.W), 1.f));
	TestTrue(
		TEXT("quarter of the track"),
		FMath::IsNearlyEqual(SolidCharacterEditor::BarFraction(Height.Bar, Height.Bar.X + Height.Bar.W * 0.25f), 0.25f));

	const FSolidSliderHit OnBar = SolidCharacterEditor::HitSliders(
		Rows, 2, Height.Bar.X + 10.f, Height.Bar.Y + 2.f);
	TestEqual(TEXT("bar hit slider"), OnBar.Slider, SolidBodyControls::Height);
	TestEqual(TEXT("bar hit kind"), static_cast<uint8>(OnBar.Kind), static_cast<uint8>(ESolidSliderHit::Bar));

	const FSolidSliderHit OnLabel = SolidCharacterEditor::HitSliders(Rows, 2, 20.f, 44.f);
	TestEqual(TEXT("label hit slider"), OnLabel.Slider, SolidBodyControls::Height);
	TestEqual(TEXT("label hit kind"), static_cast<uint8>(OnLabel.Kind), static_cast<uint8>(ESolidSliderHit::Label));

	const FSolidSliderHit OnBuild = SolidCharacterEditor::HitSliders(
		Rows, 2, Build.Bar.X + 4.f, Build.Bar.Y + 4.f);
	TestEqual(TEXT("second bar"), OnBuild.Slider, SolidBodyControls::Build);
	TestEqual(TEXT("second bar kind"), static_cast<uint8>(OnBuild.Kind), static_cast<uint8>(ESolidSliderHit::Bar));

	const FSolidSliderHit Miss = SolidCharacterEditor::HitSliders(
		Rows, 2, Height.Bar.X + Height.Bar.W + 8.f, Height.Bar.Y + 2.f);
	TestEqual(TEXT("past the track misses"), Miss.Slider, INDEX_NONE);
	TestEqual(TEXT("miss kind"), static_cast<uint8>(Miss.Kind), static_cast<uint8>(ESolidSliderHit::None));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
