#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Character/SolidCharacterStage.h"
#include "Character/SolidSkeletonHover.h"
#include "Companion/SolidCompanionCharacter.h"
#include "Menus/SolidCharacterEditor.h"
#include "Menus/SolidMainMenu.h"
#include "Squad/SolidCompany.h"
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
	TestEqual(TEXT("entry count"), Entries.Num(), SolidMainMenu::EntryCount());

	int32 EditorIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Item == ESolidMainMenuItem::CharacterEditor)
		{
			EditorIndex = Index;
			break;
		}
	}
	TestTrue(TEXT("character editor is listed"), EditorIndex != INDEX_NONE);
	if (EditorIndex == INDEX_NONE)
	{
		return false;
	}

	FSolidMainMenuEntry Found;
	TestTrue(TEXT("find the editor"), SolidMainMenu::FindEntry(EditorIndex, Found));
	TestEqual(TEXT("editor label"), Found.Label, FString(TEXT("Character editor")));
	TestEqual(
		TEXT("editor item"),
		static_cast<uint8>(Found.Item),
		static_cast<uint8>(ESolidMainMenuItem::CharacterEditor));
	TestEqual(
		TEXT("editor action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Found.Item)),
		static_cast<uint8>(ESolidMainMenuAction::OpenCharacterEditor));

	const int32 Count = SolidMainMenu::EntryCount();
	const int32 Next = (EditorIndex + 1) % Count;
	TestEqual(TEXT("wrap down from the editor"), SolidMainMenu::WrapIndex(EditorIndex, 1), Next);
	const int32 Previous = EditorIndex == 0 ? Count - 1 : EditorIndex - 1;
	TestEqual(TEXT("wrap up onto the editor"), SolidMainMenu::WrapIndex(Previous, 1), EditorIndex);
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
	TestFalse(TEXT("editor opens on the body"), HUD->IsCharacterEditorSkeleton());
	TestEqual(
		TEXT("space asks for the skeleton"),
		SolidCharacterEditor::SkeletonHint(false),
		FString(TEXT("Space  skeleton")));
	TestEqual(
		TEXT("space asks for the body"),
		SolidCharacterEditor::SkeletonHint(true),
		FString(TEXT("Space  body")));

	HUD->SetCharacterEditorSkeleton(true);
	TestTrue(TEXT("space shows the skeleton"), HUD->IsCharacterEditorSkeleton());
	HUD->SetSkeletonBoneTip(TEXT("calf_r"));
	TestEqual(TEXT("hover names the bone"), HUD->GetSkeletonBoneTip(), FString(TEXT("calf_r")));
	HUD->SetCharacterEditorSkeleton(false);
	TestTrue(TEXT("leaving the skeleton clears the name"), HUD->GetSkeletonBoneTip().IsEmpty());
	HUD->SetCharacterEditorSkeleton(true);
	HUD->SetSkeletonBoneTip(TEXT("foot_l"));

	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the editor"), HUD->IsCharacterEditorOpen());
	TestFalse(TEXT("F10 returns to the world"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("F10 clears the skeleton"), HUD->IsCharacterEditorSkeleton());
	TestTrue(TEXT("F10 clears the bone name"), HUD->GetSkeletonBoneTip().IsEmpty());

	HUD->OpenCharacterEditor();
	HUD->SetCharacterEditorSkeleton(true);
	HUD->SetSkeletonBoneTip(TEXT("thigh_l"));
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the editor"), HUD->IsCharacterEditorOpen());
	TestFalse(TEXT("Esc returns to the world"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("Esc clears the skeleton"), HUD->IsCharacterEditorSkeleton());
	TestTrue(TEXT("Esc clears the bone name"), HUD->GetSkeletonBoneTip().IsEmpty());

	HUD->OpenCharacterEditor();
	HUD->SetCharacterEditorSkeleton(true);
	HUD->SetSkeletonBoneTip(TEXT("foot_r"));
	HUD->CloseCharacterEditor();
	TestFalse(TEXT("closing the editor returns to the body"), HUD->IsCharacterEditorSkeleton());
	TestTrue(TEXT("closing the editor clears the bone name"), HUD->GetSkeletonBoneTip().IsEmpty());

	ASolidCharacterStage* Stage = NewObject<ASolidCharacterStage>();
	TestNotNull(TEXT("stage"), Stage);
	if (Stage)
	{
		Stage->SetSkeletonLines(true);
		TestTrue(TEXT("stage keeps the skeleton view"), Stage->IsShowingSkeletonLines());
		Stage->SetSkeletonLines(false);
		TestFalse(TEXT("stage returns to the body"), Stage->IsShowingSkeletonLines());
	}

	ASolidCompanionCharacter* Preview = NewObject<ASolidCompanionCharacter>();
	TestNotNull(TEXT("preview"), Preview);
	if (Preview)
	{
		Preview->SetSkeletonLines(true);
		TestTrue(TEXT("preview shows bone lines"), Preview->IsShowingSkeletonLines());
		USkeletalMeshComponent* PreviewMesh = Preview->GetMesh();
		if (PreviewMesh)
		{
			TestFalse(TEXT("skinned mesh hides"), PreviewMesh->GetVisibleFlag());
		}
		Preview->SetSkeletonLines(false);
		TestFalse(TEXT("preview returns to the body"), Preview->IsShowingSkeletonLines());
		if (PreviewMesh)
		{
			TestTrue(TEXT("skinned mesh returns"), PreviewMesh->GetVisibleFlag());
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSkeletonHoverTest,
	"SolidCore1.CharacterEditor.BoneHover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSkeletonHoverTest::RunTest(const FString& Parameters)
{
	const FVector2D From[] = {
		FVector2D(0.f, 0.f),
		FVector2D(100.f, 0.f),
		FVector2D(100.f, 0.f),
	};
	const FVector2D To[] = {
		FVector2D(100.f, 0.f),
		FVector2D(100.f, 80.f),
		FVector2D(100.f, 140.f),
	};
	const FString Names[] = {
		FString(TEXT("thigh_r")),
		FString(TEXT("calf_r")),
		FString(TEXT("foot_r")),
	};

	TestEqual(
		TEXT("along the thigh"),
		SolidSkeletonHover::BoneUnderPoint(From, To, Names, 3, FVector2D(40.f, 4.f), SolidSkeletonHover::PickRadiusPx),
		FString(TEXT("thigh_r")));
	TestEqual(
		TEXT("along the shin"),
		SolidSkeletonHover::BoneUnderPoint(From, To, Names, 3, FVector2D(104.f, 40.f), SolidSkeletonHover::PickRadiusPx),
		FString(TEXT("calf_r")));
	TestEqual(
		TEXT("a shared joint names the bone that ends there"),
		SolidSkeletonHover::BoneUnderPoint(From, To, Names, 3, FVector2D(100.f, 0.f), SolidSkeletonHover::PickRadiusPx),
		FString(TEXT("thigh_r")));
	TestEqual(
		TEXT("the next joint names the next bone"),
		SolidSkeletonHover::BoneUnderPoint(From, To, Names, 3, FVector2D(100.f, 80.f), SolidSkeletonHover::PickRadiusPx),
		FString(TEXT("calf_r")));
	TestEqual(
		TEXT("the last joint names the last bone"),
		SolidSkeletonHover::BoneUnderPoint(From, To, Names, 3, FVector2D(100.f, 140.f), SolidSkeletonHover::PickRadiusPx),
		FString(TEXT("foot_r")));
	TestTrue(
		TEXT("a miss stays quiet"),
		SolidSkeletonHover::BoneUnderPoint(
			From,
			To,
			Names,
			3,
			FVector2D(100.f + SolidSkeletonHover::PickRadiusPx + 4.f, 40.f),
			SolidSkeletonHover::PickRadiusPx).IsEmpty());
	TestTrue(
		TEXT("no bones stays quiet"),
		SolidSkeletonHover::BoneUnderPoint(nullptr, nullptr, nullptr, 0, FVector2D(0.f, 0.f), SolidSkeletonHover::PickRadiusPx).IsEmpty());

	TestTrue(TEXT("axe is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("axe_jnt")));
	TestTrue(TEXT("sheath is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("sheath_jnt_1")));
	TestTrue(TEXT("cloack is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("cloack_jnt_3_l")));
	TestTrue(TEXT("beard is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("beard_middle_jnt_2")));
	TestTrue(TEXT("shield is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("shield_jnt")));
	TestTrue(TEXT("skirt is clothing"), SolidSkeletonHover::IsOmittedBoneName(TEXT("skirt_pattern_front_jnt_4")));
	TestTrue(TEXT("ik stays off"), SolidSkeletonHover::IsOmittedBoneName(TEXT("ik_foot_root")));
	TestTrue(TEXT("left thigh twist stays off"), SolidSkeletonHover::IsOmittedBoneName(TEXT("thigh_twist_01_l")));
	TestTrue(TEXT("right calf twist stays off"), SolidSkeletonHover::IsOmittedBoneName(TEXT("calf_twist_01_r")));
	TestTrue(TEXT("a later calf twist stays off"), SolidSkeletonHover::IsOmittedBoneName(TEXT("calf_twist_02_r")));
	TestFalse(TEXT("calf stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("calf_l")));
	TestFalse(TEXT("right calf stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("calf_r")));
	TestFalse(TEXT("thigh stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("thigh_l")));
	TestFalse(TEXT("an arm twist stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("upperarm_twist_01_l")));
	TestFalse(TEXT("foot stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("foot_l")));
	TestFalse(TEXT("spine stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("spine_03")));
	TestFalse(TEXT("a later axe mention stays"), SolidSkeletonHover::IsOmittedBoneName(TEXT("hand_axe_grip")));

	ASolidCharacterStage* Stage = NewObject<ASolidCharacterStage>();
	TestNotNull(TEXT("stage"), Stage);
	if (Stage)
	{
		TestTrue(TEXT("a stage without a preview has no bone tip"), Stage->BoneUnderPointer(0.f, 0.f).IsEmpty());
		Stage->SetSkeletonLines(true);
		TestTrue(TEXT("a stage without a preview has no bone tip"), Stage->BoneUnderPointer(10.f, 10.f).IsEmpty());
	}
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
	TestTrue(TEXT("Sam skin is the default"), Sam->SkinColor.Equals(SolidCompanionColors::SkinPink()));
	const FLinearColor DefaultSkin = SolidCompanionColors::SkinPink();
	const FLinearColor ExpectedSkin(FColor(0xD6, 0xA8, 0x92));
	TestTrue(TEXT("default skin is #D6A892"), DefaultSkin.Equals(ExpectedSkin));
	TestFalse(
		TEXT("default skin is stored as linear, not the sRGB fractions"),
		DefaultSkin.Equals(FLinearColor(0xD6 / 255.f, 0xA8 / 255.f, 0x92 / 255.f)));
	TestTrue(TEXT("Alex shirt is blue"), Alex->TorsoColor.Equals(SolidCompanionColors::TrouserBlue()));
	TestTrue(TEXT("Alex trousers are red"), Alex->LegColor.Equals(SolidCompanionColors::ShirtRed()));
	TestTrue(TEXT("Alex skin is the default"), Alex->SkinColor.Equals(SolidCompanionColors::SkinPink()));
	TestTrue(TEXT("Max skin is the default"), Max->SkinColor.Equals(SolidCompanionColors::SkinPink()));

	FLinearColor PaletteTorso;
	FLinearColor PaletteLegs;
	SolidCompanionColors::PaletteForName(TEXT("Max"), PaletteTorso, PaletteLegs);
	TestTrue(TEXT("Max shirt is the generated palette"), Max->TorsoColor.Equals(PaletteTorso));

	TArray<FString> Lines;
	SolidCharacterEditor::Describe(*Sam, Lines);
	const FSolidBodyParams SamSeed = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	FSolidBodyControls Shown;
	Shown.Height = Sam->BodyHeight;
	Shown.Build = Sam->BodyBuild;
	Shown.Gender = Sam->BodyGender;
	const FSolidBodyParams Params = SolidBodyControls::Resolve(SamSeed, Shown);
	TestTrue(TEXT("name"), Lines.Contains(FString(TEXT("Sam"))));
	TestTrue(TEXT("vocation"), Lines.Contains(FString(TEXT("Rogue"))));
	TestTrue(TEXT("height"), Lines.Contains(FString::Printf(TEXT("Height %.2f"), Shown.Height)));
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
	const FSolidBodyParams NeutralBody = SolidBodyControls::Resolve(Seed, Neutral);
	TestTrue(TEXT("resolved body stays in range"), NeutralBody.ExactEquals(NeutralBody.Clamped()));
	TestTrue(TEXT("default gender is neutral"), FMath::IsNearlyEqual(Neutral.Gender, SolidBodyControls::GenderNeutral));

	const FSolidBodyParams SamSeed = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Sam")));
	const FSolidBodyParams SamResolved = SolidBodyControls::Resolve(SamSeed, SolidBodyControls::FromSeed(SamSeed));
	TestTrue(TEXT("Sam's default body stays in range"), SamResolved.ExactEquals(SamResolved.Clamped()));

	FSolidBodyControls Masculine = Neutral;
	Masculine.Gender = SolidBodyControls::Masculine;
	const FSolidBodyParams Man = SolidBodyControls::Resolve(Seed, Masculine);
	TestTrue(TEXT("masculine shoulders widen"), Man.ShoulderWidth > Seed.ShoulderWidth);
	TestTrue(TEXT("masculine hips narrow"), Man.HipWidth < Seed.HipWidth);

	FSolidBodyControls Feminine = Neutral;
	Feminine.Gender = SolidBodyControls::Feminine;
	const FSolidBodyParams Woman = SolidBodyControls::Resolve(Seed, Feminine);
	TestTrue(TEXT("feminine shoulders narrow"), Woman.ShoulderWidth < Seed.ShoulderWidth);
	TestTrue(TEXT("feminine hips widen"), Woman.HipWidth > Seed.HipWidth);

	FSolidBodyControls LowBuild = Neutral;
	LowBuild.Build = SolidBodyControls::ValueAtFraction(SolidBodyControls::Build, 0.f);
	FSolidBodyControls HighBuild = Neutral;
	HighBuild.Build = SolidBodyControls::ValueAtFraction(SolidBodyControls::Build, 1.f);
	const float LowBulk = SolidBodyControls::Resolve(Seed, LowBuild).Bulk;
	const float HighBulk = SolidBodyControls::Resolve(Seed, HighBuild).Bulk;
	TestTrue(
		TEXT("build moves bulk"),
		(HighBulk - LowBulk) * SolidBodyControls::BuildBulkDirection > 0.f);

	FSolidBodyControls Nudged = Neutral;
	const float HeightBefore = Nudged.Height;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, 1);
	TestTrue(
		TEXT("right raises height"),
		FMath::IsNearlyEqual(Nudged.Height, HeightBefore + SolidBodyControls::KeyStep, 0.001f));
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, -1);
	TestTrue(TEXT("left lowers height"), FMath::IsNearlyEqual(Nudged.Height, HeightBefore, 0.001f));
	const float HeightLow = SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 0.f);
	const float HeightHigh = SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 1.f);
	Nudged.Height = HeightLow;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Height, -1);
	TestTrue(TEXT("height stops at the low end"), FMath::IsNearlyEqual(Nudged.Height, HeightLow));
	const float GenderHigh = SolidBodyControls::ValueAtFraction(SolidBodyControls::Gender, 1.f);
	Nudged.Gender = GenderHigh;
	SolidBodyControls::Nudge(Nudged, SolidBodyControls::Gender, 1);
	TestTrue(TEXT("gender stops at the high end"), FMath::IsNearlyEqual(Nudged.Gender, GenderHigh));

	TestEqual(TEXT("up wraps onto gender"), SolidBodyControls::WrapSlider(0, -1), 2);
	TestEqual(TEXT("down wraps onto height"), SolidBodyControls::WrapSlider(2, 1), 0);
	TestEqual(TEXT("height label"), FString(SolidBodyControls::Label(SolidBodyControls::Height)), FString(TEXT("Height")));
	TestEqual(TEXT("build label"), FString(SolidBodyControls::Label(SolidBodyControls::Build)), FString(TEXT("Build")));
	TestEqual(TEXT("gender label"), FString(SolidBodyControls::Label(SolidBodyControls::Gender)), FString(TEXT("Gender")));

	TestTrue(TEXT("fraction at the low end"), FMath::IsNearlyEqual(SolidBodyControls::Fraction(SolidBodyControls::Height, HeightLow), 0.f));
	TestTrue(TEXT("fraction at the high end"), FMath::IsNearlyEqual(SolidBodyControls::Fraction(SolidBodyControls::Height, HeightHigh), 1.f));
	const float HeightMid = SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 0.5f);
	TestTrue(
		TEXT("mid bar is the mid height"),
		FMath::IsNearlyEqual(SolidBodyControls::Fraction(SolidBodyControls::Height, HeightMid), 0.5f, 0.001f));
	TestTrue(TEXT("gender bar starts at 0"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Gender, 0.f), 0.f));
	TestTrue(TEXT("gender bar ends at 1"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Gender, 1.f), 1.f));
	TestTrue(
		TEXT("mouse step quantizes"),
		FMath::Abs(SolidBodyControls::ValueAtFraction(SolidBodyControls::Height, 0.505f) - HeightMid)
			<= SolidBodyControls::MouseStep + 0.001f);

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
	const FSolidBodyControls SamControls = SolidBodyControls::FromSeed(SamSeed);
	TestTrue(TEXT("catalog height is the default slider"), FMath::IsNearlyEqual(Sam->BodyHeight, SamControls.Height));
	TestTrue(TEXT("catalog build is the default slider"), FMath::IsNearlyEqual(Sam->BodyBuild, SamControls.Build));
	TestTrue(TEXT("catalog gender is neutral"), FMath::IsNearlyEqual(Sam->BodyGender, SamControls.Gender));

	TArray<FString> Lines;
	SolidCharacterEditor::Describe(*Sam, Lines);
	TestTrue(TEXT("page up switches companions"), Lines.Contains(FString(TEXT("Page Up / Page Down"))));
	TestTrue(TEXT("up and down select a slider"), Lines.Contains(FString(TEXT("Up / Down"))));
	TestTrue(TEXT("left and right still named"), Lines.Contains(FString(TEXT("Left / Right"))));
	TestTrue(TEXT("build line"), Lines.Contains(FString::Printf(TEXT("Build %.2f"), Sam->BodyBuild)));
	TestTrue(TEXT("gender line"), Lines.Contains(FString::Printf(TEXT("Gender %.2f"), Sam->BodyGender)));
	TestEqual(TEXT("height line is a slider"), SolidCharacterEditor::SliderForLine(TEXT("Height 1.00")), SolidBodyControls::Height);
	TestEqual(TEXT("head size is not a slider"), SolidCharacterEditor::SliderForLine(TEXT("Head size 1.00")), INDEX_NONE);

	const float NextHeight = FMath::IsNearlyEqual(Sam->BodyHeight, HeightLow) ? HeightHigh : HeightLow;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBodySliderDerivationTest,
	"SolidCore1.Body.SliderDerivation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBodySliderDerivationTest::RunTest(const FString& Parameters)
{
	FSolidBodyParams SeedA;
	SeedA.Height = 1.2f;
	SeedA.Bulk = 0.8f;
	const FSolidBodyParams SeedB = FSolidBodyParams::FromSeed(SolidBodyMesh::SeedForName(TEXT("Quinn")));
	const FSolidBodyControls Neutral = SolidBodyControls::FromSeed(SeedA);

	const FSolidBodyParams MidA = SolidBodyControls::Resolve(SeedA, Neutral);
	const FSolidBodyParams MidB = SolidBodyControls::Resolve(SeedB, Neutral);
	TestTrue(TEXT("sliders ignore the seed"), MidA.ExactEquals(MidB));
	TestTrue(TEXT("neutral height is the viking scale"), FMath::IsNearlyEqual(MidA.Height, 1.f));
	TestTrue(TEXT("neutral bulk is the viking scale"), FMath::IsNearlyEqual(MidA.Bulk, 1.f));
	TestTrue(TEXT("neutral shoulders are the viking scale"), FMath::IsNearlyEqual(MidA.ShoulderWidth, 1.f));
	TestTrue(TEXT("neutral hips are the viking scale"), FMath::IsNearlyEqual(MidA.HipWidth, 1.f));
	TestTrue(TEXT("neutral legs are the viking scale"), FMath::IsNearlyEqual(MidA.LegLength, 1.f));
	TestTrue(TEXT("neutral arms are the viking scale"), FMath::IsNearlyEqual(MidA.ArmLength, 1.f));
	TestTrue(TEXT("neutral head is the viking scale"), FMath::IsNearlyEqual(MidA.HeadSize, 1.f));

	FSolidBodyControls Dwarf = Neutral;
	Dwarf.Build = 0.f;
	FSolidBodyControls Elf = Neutral;
	Elf.Build = 1.f;
	const FSolidBodyParams ShortWide = SolidBodyControls::Resolve(SeedA, Dwarf);
	const FSolidBodyParams TallThin = SolidBodyControls::Resolve(SeedA, Elf);
	TestTrue(TEXT("dwarf is shorter than the mid point"), ShortWide.Height < MidA.Height);
	TestTrue(TEXT("elf is taller than the mid point"), TallThin.Height > MidA.Height);
	TestTrue(TEXT("dwarf is wider than the elf"), ShortWide.Bulk > TallThin.Bulk);
	TestTrue(TEXT("dwarf legs are shorter"), ShortWide.LegLength < TallThin.LegLength);
	TestTrue(TEXT("dwarf head is larger"), ShortWide.HeadSize > TallThin.HeadSize);
	TestTrue(TEXT("elf shoulders are narrower"), TallThin.ShoulderWidth < ShortWide.ShoulderWidth);

	FSolidBodyControls Feminine = Neutral;
	Feminine.Gender = SolidBodyControls::Feminine;
	FSolidBodyControls Masculine = Neutral;
	Masculine.Gender = SolidBodyControls::Masculine;
	const FSolidBodyParams Woman = SolidBodyControls::Resolve(SeedA, Feminine);
	const FSolidBodyParams Man = SolidBodyControls::Resolve(SeedA, Masculine);
	TestTrue(TEXT("feminine is shorter than masculine"), Woman.Height < Man.Height);
	TestTrue(TEXT("feminine shoulders are narrower"), Woman.ShoulderWidth < Man.ShoulderWidth);
	TestTrue(TEXT("feminine hips are wider"), Woman.HipWidth > Man.HipWidth);
	TestTrue(TEXT("masculine chest is broader"), Man.TorsoGirth > Woman.TorsoGirth);

	FSolidBodyControls Short = Neutral;
	Short.Height = 0.f;
	FSolidBodyControls Tall = Neutral;
	Tall.Height = 1.f;
	const FSolidBodyParams Reduced = SolidBodyControls::Resolve(SeedA, Short);
	const FSolidBodyParams Increased = SolidBodyControls::Resolve(SeedA, Tall);
	TestTrue(TEXT("height 0 shortens the figure"), Reduced.Height < MidA.Height);
	TestTrue(TEXT("height 1 lengthens the figure"), Increased.Height > MidA.Height);
	TestTrue(TEXT("height slider scales the arms"), Reduced.ArmLength < MidA.ArmLength && Increased.ArmLength > MidA.ArmLength);
	TestTrue(TEXT("height slider leaves the bulk"), FMath::IsNearlyEqual(Reduced.Bulk, MidA.Bulk));
	TestTrue(TEXT("sliders stay on 0 to 1"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Build, 0.f), 0.f));
	TestTrue(TEXT("build slider ends at 1"), FMath::IsNearlyEqual(SolidBodyControls::ValueAtFraction(SolidBodyControls::Build, 1.f), 1.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCharacterEditorTurnAndBoneColorTest,
	"SolidCore1.CharacterEditor.TurnAndBoneColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCharacterEditorTurnAndBoneColorTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("home is clockwise"), SolidCharacterEditor::TurnClockwiseHint(), FString(TEXT("Home  clockwise")));
	TestEqual(TEXT("end is anticlockwise"), SolidCharacterEditor::TurnAnticlockwiseHint(), FString(TEXT("End  anticlockwise")));
	TestEqual(TEXT("one clockwise step"), SolidCharacterEditor::TurnYawDelta(true), -SolidCharacterEditor::TurnStepDegrees);
	TestEqual(TEXT("one anticlockwise step"), SolidCharacterEditor::TurnYawDelta(false), SolidCharacterEditor::TurnStepDegrees);
	TestTrue(TEXT("a step is visible"), SolidCharacterEditor::TurnStepDegrees >= 10.f);

	ASolidCharacterStage* Stage = NewObject<ASolidCharacterStage>();
	TestNotNull(TEXT("stage"), Stage);
	if (Stage)
	{
		TestTrue(TEXT("the figure starts facing you"), FMath::IsNearlyZero(Stage->GetPreviewYaw()));
		Stage->AddPreviewYaw(SolidCharacterEditor::TurnYawDelta(true));
		TestTrue(TEXT("clockwise yaw is negative"), Stage->GetPreviewYaw() < 0.f);
		Stage->AddPreviewYaw(SolidCharacterEditor::TurnYawDelta(false));
		TestTrue(TEXT("anticlockwise undoes clockwise"), FMath::IsNearlyZero(Stage->GetPreviewYaw()));
	}

	const int32 Parents[] = { INDEX_NONE, 0, 0, 0, 1 };
	int32 Colors[5] = { -1, -1, -1, -1, -1 };
	SolidSkeletonHover::AssignBoneColors(Parents, 5, Colors, SolidSkeletonHover::BonePaletteCount);
	TestTrue(TEXT("a child differs from its parent"), Colors[1] != Colors[0]);
	TestTrue(TEXT("siblings differ"), Colors[1] != Colors[2] && Colors[1] != Colors[3] && Colors[2] != Colors[3]);
	TestTrue(TEXT("a grandchild differs from its parent"), Colors[4] != Colors[1]);
	TestTrue(TEXT("colours stay in the palette"), Colors[0] >= 0 && Colors[0] < SolidSkeletonHover::BonePaletteCount);

	const int32 Chain[] = { INDEX_NONE, 0, 1, 2 };
	int32 ChainColors[4] = {};
	SolidSkeletonHover::AssignBoneColors(Chain, 4, ChainColors, SolidSkeletonHover::BonePaletteCount);
	TestTrue(TEXT("each link differs from the one before it"),
		ChainColors[1] != ChainColors[0]
		&& ChainColors[2] != ChainColors[1]
		&& ChainColors[3] != ChainColors[2]);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
