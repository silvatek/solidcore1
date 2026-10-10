#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Party/SolidBattlePlan.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"
#include "SolidGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattleFormationSlotsTest,
	"SolidCore1.BattlePlan.FormationSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattleFormationSlotsTest::RunTest(const FString& Parameters)
{
	constexpr int32 Count = 2;

	const FVector2D Line0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Line, 0, Count, ESolidBattleSpacing::Standard);
	const FVector2D Line1 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Line, 1, Count, ESolidBattleSpacing::Standard);
	TestTrue(TEXT("line: both slightly behind"), Line0.X < 0.f && Line1.X < 0.f);
	TestTrue(TEXT("line: opposite flanks"), Line0.Y * Line1.Y < 0.f);
	TestEqual(
		TEXT("line: same depth"),
		static_cast<float>(Line0.X),
		static_cast<float>(Line1.X));

	const FVector2D Col0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Column, 0, Count, ESolidBattleSpacing::Standard);
	const FVector2D Col1 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Column, 1, Count, ESolidBattleSpacing::Standard);
	TestTrue(TEXT("column: alex further back than sam"), Col1.X < Col0.X);
	TestEqual(TEXT("column: centered"), static_cast<float>(Col0.Y), 0.f);
	TestEqual(TEXT("column: alex centered"), static_cast<float>(Col1.Y), 0.f);

	const FVector2D MobStd0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Mob, 0, Count, ESolidBattleSpacing::Standard);
	const FVector2D MobNarrow0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Mob, 0, Count, ESolidBattleSpacing::Narrow);
	const FVector2D MobWide0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Mob, 0, Count, ESolidBattleSpacing::Wide);
	TestTrue(TEXT("mob: both behind"), MobStd0.X < 0.f);
	TestTrue(TEXT("mob: tighter than line (standard)"), FMath::Abs(MobStd0.Y) < FMath::Abs(Line0.Y));
	TestTrue(TEXT("narrow closer than standard"), FMath::Abs(MobNarrow0.Y) < FMath::Abs(MobStd0.Y));
	TestTrue(TEXT("wide farther than standard"), FMath::Abs(MobWide0.Y) > FMath::Abs(MobStd0.Y));
	TestTrue(TEXT("narrow shallower than standard"), FMath::Abs(MobNarrow0.X) < FMath::Abs(MobStd0.X));
	TestTrue(TEXT("wide deeper than standard"), FMath::Abs(MobWide0.X) > FMath::Abs(MobStd0.X));

	const FVector2D Parade0 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Parade, 0, Count, ESolidBattleSpacing::Standard);
	const FVector2D Parade1 = SolidBattleFormationSlots::SlotOffset(
		ESolidBattleFormation::Parade, 1, Count, ESolidBattleSpacing::Standard);
	TestTrue(TEXT("parade: both in front"), Parade0.X > 0.f && Parade1.X > 0.f);
	TestEqual(
		TEXT("parade: same depth"),
		static_cast<float>(Parade0.X),
		static_cast<float>(Parade1.X));
	TestTrue(TEXT("parade: opposite flanks"), Parade0.Y * Parade1.Y < 0.f);
	TestTrue(TEXT("parade stands further than the old 2.8 m rank"), Parade0.X > 450.f);

	const float StandingFront = Parade0.X - SolidBattleFormationSlots::ParadeArrivalSlackCm;
	const float HalfSpan = FMath::Abs(Parade0.Y);
	const float Outer = HalfSpan + SolidBattleFormationSlots::ParadeBodyHalfWidthCm;
	const float HorizontalDeg = FMath::RadiansToDegrees(FMath::Atan2(Outer, StandingFront));
	const float HorizontalLimit =
		SolidBattleFormationSlots::CaptainHalfFovDeg * SolidBattleFormationSlots::ParadeViewFill;
	TestTrue(TEXT("parade rank fits across the captain's view"), HorizontalDeg <= HorizontalLimit + 0.5f);

	const float Slant = FMath::Sqrt(StandingFront * StandingFront + HalfSpan * HalfSpan);
	const float VerticalDeg = FMath::RadiansToDegrees(
		FMath::Atan2(SolidBattleFormationSlots::ParadeFeetBelowEyeCm, Slant));
	const float VerticalLimit =
		SolidBattleFormationSlots::CaptainHalfVerticalFovDeg() * SolidBattleFormationSlots::ParadeViewFill;
	TestTrue(TEXT("parade rank fits the captain's view vertically"), VerticalDeg <= VerticalLimit + 0.5f);
	TestTrue(TEXT("a very wide rank stays short of the fog"),
		SolidBattleFormationSlots::ParadeStandingFrontCm(5000.f)
		<= SolidBattleFormationSlots::ParadeMaxFrontCm + 0.5f);
	TestTrue(TEXT("parade faces the captain"),
		SolidBattleFormationSlots::FacesCaptain(ESolidBattleFormation::Parade));
	TestFalse(TEXT("line does not face the captain"),
		SolidBattleFormationSlots::FacesCaptain(ESolidBattleFormation::Line));
	TestTrue(TEXT("face once inside the slot"),
		SolidBattleFormationSlots::ShouldFaceCaptain(ESolidBattleFormation::Parade, 90.f, 90.f));
	TestFalse(TEXT("keep walking until the slot"),
		SolidBattleFormationSlots::ShouldFaceCaptain(ESolidBattleFormation::Parade, 91.f, 90.f));

	const FVector2D CaptainXY = FVector2D::ZeroVector;
	const float Yaw = SolidBattleFormationSlots::YawFacingPoint(Parade0, CaptainXY);
	const FVector2D Facing(
		FMath::Cos(FMath::DegreesToRadians(Yaw)),
		FMath::Sin(FMath::DegreesToRadians(Yaw)));
	const FVector2D ToCaptain = (CaptainXY - Parade0).GetSafeNormal();
	TestTrue(TEXT("parade yaw looks at the captain"),
		FVector2D::DotProduct(Facing, ToCaptain) > 0.99f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanyDefaultPlansTest,
	"SolidCore1.BattlePlan.CompanyDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanyDefaultPlansTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	TestNotNull(TEXT("company"), Company);
	Company->InitializeDefaultBattlePlans();
	TestEqual(TEXT("five starter plans"), Company->GetBattlePlanCount(), 5);
	Company->InitializeDefaultBattlePlans();
	TestEqual(TEXT("idempotent init"), Company->GetBattlePlanCount(), 5);

	const FSolidBattlePlan* Line = Company->GetBattlePlan(0);
	const FSolidBattlePlan* Column = Company->GetBattlePlan(1);
	const FSolidBattlePlan* Tight = Company->GetBattlePlan(2);
	const FSolidBattlePlan* Loose = Company->GetBattlePlan(3);
	const FSolidBattlePlan* Parade = Company->GetBattlePlan(4);
	TestNotNull(TEXT("line plan"), Line);
	TestNotNull(TEXT("column plan"), Column);
	TestNotNull(TEXT("tight mob"), Tight);
	TestNotNull(TEXT("loose mob"), Loose);
	TestNotNull(TEXT("parade plan"), Parade);
	TestEqual(TEXT("line name"), Line->Name, FString(TEXT("Line")));
	TestEqual(TEXT("column name"), Column->Name, FString(TEXT("Column")));
	TestEqual(TEXT("tight name"), Tight->Name, FString(TEXT("Tight mob")));
	TestEqual(TEXT("loose name"), Loose->Name, FString(TEXT("Loose mob")));
	TestEqual(TEXT("parade name"), Parade->Name, FString(TEXT("Parade")));
	TestTrue(TEXT("line formation"), Line->Formation == ESolidBattleFormation::Line);
	TestTrue(TEXT("column formation"), Column->Formation == ESolidBattleFormation::Column);
	TestTrue(TEXT("tight formation mob"), Tight->Formation == ESolidBattleFormation::Mob);
	TestTrue(TEXT("loose formation mob"), Loose->Formation == ESolidBattleFormation::Mob);
	TestTrue(TEXT("line standard spacing"), Line->Spacing == ESolidBattleSpacing::Standard);
	TestTrue(TEXT("column standard spacing"), Column->Spacing == ESolidBattleSpacing::Standard);
	TestTrue(TEXT("tight narrow spacing"), Tight->Spacing == ESolidBattleSpacing::Narrow);
	TestTrue(TEXT("loose wide spacing"), Loose->Spacing == ESolidBattleSpacing::Wide);
	TestTrue(TEXT("parade formation"), Parade->Formation == ESolidBattleFormation::Parade);
	TestTrue(TEXT("parade standard spacing"), Parade->Spacing == ESolidBattleSpacing::Standard);
	TestEqual(TEXT("find loose mob"), Company->FindBattlePlanIndexByName(TEXT("Loose mob")), 3);
	TestEqual(TEXT("find parade"), Company->FindBattlePlanIndexByName(TEXT("Parade")), 4);
	TestNull(TEXT("out of range"), Company->GetBattlePlan(99));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartyAssignedPlansTest,
	"SolidCore1.BattlePlan.PartyAssigned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartyAssignedPlansTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidParty* Party = NewObject<USolidParty>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("party"), Party);

	Party->InitializeFromCompany(Company);
	TestEqual(TEXT("assigned starter count"), Party->GetAssignedCount(), 5);
	TestTrue(TEXT("active is line by default"),
		Party->GetActiveFormation() == ESolidBattleFormation::Line);
	TestEqual(TEXT("active slot is F1"), Party->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("default spacing standard"),
		Party->GetActiveSpacing() == ESolidBattleSpacing::Standard);

	TestTrue(TEXT("select F2 column"), Party->SelectAssignedSlot(1));
	TestTrue(TEXT("active column"), Party->GetActiveFormation() == ESolidBattleFormation::Column);
	TestTrue(TEXT("select F3 tight mob"), Party->SelectAssignedSlot(2));
	TestTrue(TEXT("active mob"), Party->GetActiveFormation() == ESolidBattleFormation::Mob);
	TestTrue(TEXT("tight narrow"), Party->GetActiveSpacing() == ESolidBattleSpacing::Narrow);
	TestTrue(TEXT("select F4 loose mob"), Party->SelectAssignedSlot(3));
	TestTrue(TEXT("loose wide"), Party->GetActiveSpacing() == ESolidBattleSpacing::Wide);
	TestTrue(TEXT("select F5 parade"), Party->SelectAssignedSlot(4));
	TestTrue(TEXT("active parade"), Party->GetActiveFormation() == ESolidBattleFormation::Parade);
	TestFalse(TEXT("reject empty F6"), Party->SelectAssignedSlot(5));
	TestFalse(TEXT("reject F8 when only 5"), Party->SelectAssignedSlot(7));
	TestEqual(TEXT("slot unchanged after reject"), Party->GetActiveAssignedSlot(), 4);

	// Max 8 assigned — grow the Company catalog first so indices are unique.
	TArray<int32> TooMany;
	for (int32 i = 0; i < 12; ++i)
	{
		FSolidBattlePlan Extra;
		Extra.Name = FString::Printf(TEXT("Extra%d"), i);
		Extra.Formation = ESolidBattleFormation::Mob;
		Extra.Spacing = ESolidBattleSpacing::Standard;
		TooMany.Add(Company->AddBattlePlan(Extra));
	}
	TestTrue(TEXT("set assigned"), Party->SetAssignedBattlePlans(TooMany));
	TestEqual(TEXT("capped at 8"), Party->GetAssignedCount(), SolidBattleFormationSlots::MaxAssignedBattlePlans);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeBattlePlanSelectTest,
	"SolidCore1.BattlePlan.GameModeSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeBattlePlanSelectTest::RunTest(const FString& Parameters)
{
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	TestNotNull(TEXT("company"), GameMode->GetCompany());
	TestNotNull(TEXT("party"), GameMode->GetParty());
	TestEqual(TEXT("company plans"), GameMode->GetCompany()->GetBattlePlanCount(), 5);
	TestTrue(TEXT("default line"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Line);
	TestEqual(TEXT("default F1"), GameMode->GetParty()->GetActiveAssignedSlot(), 0);

	TestTrue(TEXT("F2 column"), GameMode->SelectBattlePlanSlot(1));
	TestTrue(TEXT("formation column"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Column);
	TestTrue(TEXT("F3 tight"), GameMode->SelectBattlePlanSlot(2));
	TestTrue(TEXT("F4 loose"), GameMode->SelectBattlePlanSlot(3));
	TestTrue(TEXT("loose wide"),
		GameMode->GetParty()->GetActiveSpacing() == ESolidBattleSpacing::Wide);
	TestTrue(TEXT("F5 parade"), GameMode->SelectBattlePlanSlot(4));
	TestTrue(TEXT("formation parade"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Parade);
	TestFalse(TEXT("F6 empty"), GameMode->SelectBattlePlanSlot(5));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeBattlePlanSurvivesEnsureTest,
	"SolidCore1.BattlePlan.SelectionSurvivesEnsure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeBattlePlanSurvivesEnsureTest::RunTest(const FString& Parameters)
{
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);

	TestTrue(TEXT("F2 column"), GameMode->SelectBattlePlanSlot(1));
	TestEqual(TEXT("slot is F2"), GameMode->GetParty()->GetActiveAssignedSlot(), 1);

	// Enter-town refresh calls EnsureCompanyAndParty on a timer. That used to
	// rebuild the party and snap the active plan back to Line.
	GameMode->EnsureCompanyAndParty();
	TestEqual(TEXT("slot stays F2"), GameMode->GetParty()->GetActiveAssignedSlot(), 1);
	TestTrue(TEXT("formation stays column"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Column);

	TestTrue(TEXT("F5 parade"), GameMode->SelectBattlePlanSlot(4));
	GameMode->EnsureCompanyAndParty();
	TestEqual(TEXT("slot stays F5"), GameMode->GetParty()->GetActiveAssignedSlot(), 4);
	TestTrue(TEXT("formation stays parade"),
		GameMode->GetParty()->GetActiveFormation() == ESolidBattleFormation::Parade);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
