#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidSquadBuilder.h"
#include "Company/SolidBattlePlan.h"
#include "Company/SolidCaptain.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"
#include "SolidGameInstance.h"
#include "SolidGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidBattlePlanTestPrivate
{
	int32 FindCompanyPlan(const USolidCompany* Company, const TCHAR* Name)
	{
		if (!Company || !Name)
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < Company->GetBattlePlanCount(); ++Index)
		{
			const FSolidBattlePlan* Plan = Company->GetBattlePlan(Index);
			if (Plan && Plan->Name == Name)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	const FSolidBattlePlan* FindPlan(const USolidCompany* Company, const TCHAR* Name)
	{
		const int32 Index = FindCompanyPlan(Company, Name);
		return Index == INDEX_NONE ? nullptr : Company->GetBattlePlan(Index);
	}

	int32 FindAssignedSlot(const USolidSquad* Squad, const TCHAR* Name)
	{
		if (!Squad || !Name)
		{
			return INDEX_NONE;
		}
		for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
		{
			const FSolidBattlePlan* Plan = Squad->GetAssignedBattlePlan(Slot);
			if (Plan && Plan->Name == Name)
			{
				return Slot;
			}
		}
		return INDEX_NONE;
	}

	/**
	 * The assigned slot for a catalog plan.
	 * The starter squad already has every plan, so that slot is the answer.
	 * A later catalog leaves a plan off the keys until it is placed, then synced onto the first empty key.
	 */
	int32 SlotForPlan(USolidSquad* Squad, const TCHAR* Name)
	{
		const int32 Existing = FindAssignedSlot(Squad, Name);
		if (Existing != INDEX_NONE)
		{
			return Existing;
		}
		if (!Squad)
		{
			return INDEX_NONE;
		}
		USolidCompany* Company = Squad->GetCompany();
		const int32 CompanyIndex = FindCompanyPlan(Company, Name);
		if (CompanyIndex == INDEX_NONE)
		{
			return INDEX_NONE;
		}

		TArray<int32> Slots = Squad->GetAssignedCompanyPlanIndices();
		Slots.SetNum(SolidBattleFormationSlots::MaxAssignedBattlePlans);
		for (int32& Slot : Slots)
		{
			if (Slot == INDEX_NONE)
			{
				Slot = CompanyIndex;
				break;
			}
		}
		Squad->SetAssignedBattlePlans(Slots);
		return FindAssignedSlot(Squad, Name);
	}

	FString AssignedNames(const USolidSquad* Squad)
	{
		FString Names;
		if (!Squad)
		{
			return Names;
		}
		for (int32 Slot = 0; Slot < SolidBattleFormationSlots::MaxAssignedBattlePlans; ++Slot)
		{
			const FSolidBattlePlan* Plan = Squad->GetAssignedBattlePlan(Slot);
			if (!Plan)
			{
				continue;
			}
			if (!Names.IsEmpty())
			{
				Names += TEXT(", ");
			}
			Names += Plan->Name;
		}
		return Names;
	}

	bool EventChangesBattlePlan(const TCHAR* EventName)
	{
		for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
		{
			const SolidEvents::FEvent& Event = SolidEvents::Definitions[Index];
			if (FCString::Strcmp(Event.Name, EventName) != 0)
			{
				continue;
			}
			const int32 ChangeCount = FMath::Min(Event.ChangeCount, SolidEvents::MaxChangesPerEvent);
			for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
			{
				const SolidEvents::EChange Kind = Event.Changes[ChangeIndex].Kind;
				if (Kind == SolidEvents::EChange::EnableBattlePlan
					|| Kind == SolidEvents::EChange::DisableBattlePlan)
				{
					return true;
				}
			}
			return false;
		}
		return false;
	}

	struct FPlay
	{
		USolidGameInstance* Instance = nullptr;
		ASolidGameMode* GameMode = nullptr;
		USolidCompany* Company = nullptr;
		USolidSquad* Squad = nullptr;
		USolidGameState* State = nullptr;
		USolidCaptain* Captain = nullptr;

		bool Open()
		{
			Instance = NewObject<USolidGameInstance>();
			GameMode = Instance ? NewObject<ASolidGameMode>(Instance) : nullptr;
			Company = GameMode ? GameMode->GetCompany() : nullptr;
			Squad = GameMode ? GameMode->GetSquad() : nullptr;
			State = GameMode ? GameMode->GetConfiguration() : nullptr;
			Captain = Company ? Company->GetCaptain() : nullptr;
			return Instance && GameMode && Company && Squad && State && Captain;
		}

		void Sync()
		{
			if (Squad && State)
			{
				Squad->SyncEnabledBattlePlans(*State);
			}
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanIdentityTest,
	"SolidCore1.BattlePlan.Identity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanIdentityTest::RunTest(const FString& Parameters)
{
	FSolidBattlePlan Starter;
	Starter.Name = TEXT("Starter");
	Starter.Formation = ESolidBattleFormation::Line;
	Starter.Spacing = ESolidBattleSpacing::Standard;

	FSolidBattlePlan Copy = Starter;
	TestTrue(TEXT("a copy is the same plan"), Copy.SameAs(Starter));
	TestTrue(TEXT("the name is Starter"), Starter.Is(TEXT("Starter")));
	TestFalse(TEXT("the name is Attack"), Starter.Is(TEXT("Attack")));

	FSolidBattlePlan Parade;
	Parade.Name = TEXT("Parade");
	Parade.Formation = ESolidBattleFormation::Parade;
	Parade.Spacing = ESolidBattleSpacing::Standard;
	TestFalse(TEXT("Parade is Starter"), Parade.SameAs(Starter));
	TestTrue(TEXT("Parade faces the captain"), Parade.IsParade());
	TestFalse(TEXT("Starter faces the captain"), Starter.IsParade());

	FSolidBattlePlan Renamed = Starter;
	Renamed.Name = TEXT("Attack");
	TestFalse(TEXT("a new name stays the same plan"), Renamed.SameAs(Starter));

	FSolidBattlePlan Wider = Starter;
	Wider.Spacing = ESolidBattleSpacing::Wide;
	TestFalse(TEXT("a new spacing stays the same plan"), Wider.SameAs(Starter));

	FSolidBattlePlan Column = Starter;
	Column.Formation = ESolidBattleFormation::Column;
	TestFalse(TEXT("a new formation stays the same plan"), Column.SameAs(Starter));
	return true;
}

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
	const int32 CatalogCount = Company->GetBattlePlanCount();
	TestTrue(TEXT("the catalog has the five formations"), CatalogCount >= 5);
	Company->InitializeDefaultBattlePlans();
	TestEqual(TEXT("idempotent init"), Company->GetBattlePlanCount(), CatalogCount);

	const FSolidBattlePlan* Line = SolidBattlePlanTestPrivate::FindPlan(Company, TEXT("Line"));
	const FSolidBattlePlan* Column = SolidBattlePlanTestPrivate::FindPlan(Company, TEXT("Column"));
	const FSolidBattlePlan* Tight = SolidBattlePlanTestPrivate::FindPlan(Company, TEXT("Tight mob"));
	const FSolidBattlePlan* Loose = SolidBattlePlanTestPrivate::FindPlan(Company, TEXT("Loose mob"));
	const FSolidBattlePlan* Parade = SolidBattlePlanTestPrivate::FindPlan(Company, TEXT("Parade"));
	TestNotNull(TEXT("line plan"), Line);
	TestNotNull(TEXT("column plan"), Column);
	TestNotNull(TEXT("tight mob"), Tight);
	TestNotNull(TEXT("loose mob"), Loose);
	TestNotNull(TEXT("parade plan"), Parade);
	if (!Line || !Column || !Tight || !Loose || !Parade)
	{
		return false;
	}
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
	TestNull(TEXT("out of range"), Company->GetBattlePlan(99));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadAssignedPlansTest,
	"SolidCore1.BattlePlan.SquadAssigned",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadAssignedPlansTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);

	Squad->InitializeFromCompany(Company);
	const int32 AssignedCap = FMath::Min(
		Company->GetBattlePlanCount(), SolidBattleFormationSlots::MaxAssignedBattlePlans);
	TestEqual(TEXT("starter keys follow the catalog"), Squad->GetAssignedCount(), AssignedCap);
	TestTrue(TEXT("active is line by default"),
		Squad->GetActiveFormation() == ESolidBattleFormation::Line);
	TestEqual(TEXT("active slot is F1"), Squad->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("default spacing standard"),
		Squad->GetActiveSpacing() == ESolidBattleSpacing::Standard);

	const int32 ColumnSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Column"));
	const int32 TightSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Tight mob"));
	const int32 LooseSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Loose mob"));
	const int32 ParadeSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Parade"));
	TestTrue(TEXT("column has a key"), ColumnSlot != INDEX_NONE);
	TestTrue(TEXT("select column"), Squad->SelectAssignedSlot(ColumnSlot));
	TestTrue(TEXT("active column"), Squad->GetActiveFormation() == ESolidBattleFormation::Column);
	TestTrue(TEXT("select tight mob"), Squad->SelectAssignedSlot(TightSlot));
	TestTrue(TEXT("active mob"), Squad->GetActiveFormation() == ESolidBattleFormation::Mob);
	TestTrue(TEXT("tight narrow"), Squad->GetActiveSpacing() == ESolidBattleSpacing::Narrow);
	TestTrue(TEXT("select loose mob"), Squad->SelectAssignedSlot(LooseSlot));
	TestTrue(TEXT("loose wide"), Squad->GetActiveSpacing() == ESolidBattleSpacing::Wide);
	TestTrue(TEXT("select parade"), Squad->SelectAssignedSlot(ParadeSlot));
	TestTrue(TEXT("active parade"), Squad->GetActiveFormation() == ESolidBattleFormation::Parade);
	TestFalse(TEXT("reject the first key past the assigned plans"), Squad->SelectAssignedSlot(Squad->GetAssignedCount()));
	TestEqual(TEXT("slot unchanged after reject"), Squad->GetActiveAssignedSlot(), ParadeSlot);

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
	TestTrue(TEXT("set assigned"), Squad->SetAssignedBattlePlans(TooMany));
	TestEqual(TEXT("capped at 8"), Squad->GetAssignedCount(), SolidBattleFormationSlots::MaxAssignedBattlePlans);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeBattlePlanSelectTest,
	"SolidCore1.BattlePlan.GameModeSelect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeBattlePlanSelectTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	TestNotNull(TEXT("company"), GameMode->GetCompany());
	TestNotNull(TEXT("squad"), GameMode->GetSquad());
	TestNotNull(TEXT("line is in the catalog"), SolidBattlePlanTestPrivate::FindPlan(GameMode->GetCompany(), TEXT("Line")));
	TestNotNull(TEXT("parade is in the catalog"), SolidBattlePlanTestPrivate::FindPlan(GameMode->GetCompany(), TEXT("Parade")));
	TestTrue(TEXT("default line"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Line);
	TestEqual(TEXT("default F1"), GameMode->GetSquad()->GetActiveAssignedSlot(), 0);

	USolidSquad* Squad = GameMode->GetSquad();
	const int32 ColumnSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Column"));
	const int32 TightSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Tight mob"));
	const int32 LooseSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Loose mob"));
	TestTrue(TEXT("column"), GameMode->SelectBattlePlanSlot(ColumnSlot));
	TestTrue(TEXT("formation column"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Column);
	TestTrue(TEXT("tight"), GameMode->SelectBattlePlanSlot(TightSlot));
	TestTrue(TEXT("loose"), GameMode->SelectBattlePlanSlot(LooseSlot));
	TestTrue(TEXT("loose wide"),
		GameMode->GetSquad()->GetActiveSpacing() == ESolidBattleSpacing::Wide);
	TestEqual(TEXT("other plans leave the squad empty"), GameMode->GetMaxSquadSize(), 0);
	TestFalse(TEXT("other plans leave raven sight locked"), GameMode->IsSightEnabled(ESolidSight::Raven));

	const int32 ParadeSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Parade"));
	TestTrue(TEXT("parade"), GameMode->SelectBattlePlanSlot(ParadeSlot));
	TestTrue(TEXT("formation parade"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Parade);
	TestTrue(TEXT("parade unlocks raven sight"), GameMode->IsSightEnabled(ESolidSight::Raven));
	TestEqual(TEXT("parade raises the squad cap"), GameMode->GetMaxSquadSize(), SolidEvents::RosterCount);
	TestNotNull(TEXT("parade company captain"), GameMode->GetCompany()->GetCaptain());
	TestEqual(
		TEXT("parade titles the captain"),
		FString(CaptainTitleName(GameMode->GetCompany()->GetCaptain()->Title)),
		FString(CaptainTitleName(ESolidCaptainTitle::Captain)));
	TestEqual(
		TEXT("parade leaves the captain where he stands"),
		GameMode->GetCompany()->GetCaptain()->OccupiedTown,
		INDEX_NONE);

	auto Fired = [](const USolidGameState& State, const TCHAR* Name)
	{
		for (const int32 EventIndex : State.FiredOrder)
		{
			if (EventIndex >= 0
				&& EventIndex < SolidEvents::DefinitionCount
				&& FCString::Strcmp(SolidEvents::Definitions[EventIndex].Name, Name) == 0)
			{
				return true;
			}
		}
		return false;
	};
	const USolidGameState* StatePtr = GameMode->GetConfiguration();
	TestNotNull(TEXT("play state"), StatePtr);
	if (!StatePtr)
	{
		return false;
	}
	const USolidGameState& State = *StatePtr;
	TestTrue(TEXT("parade fires Enter Iglin"), Fired(State, TEXT("Enter Iglin")));
	TestTrue(TEXT("parade fires Enter Relion"), Fired(State, TEXT("Enter Relion")));
	TestTrue(TEXT("parade fires Enter Dragongate"), Fired(State, TEXT("Enter Dragongate")));
	TestFalse(TEXT("parade does not fire Enter Kanfold"), Fired(State, TEXT("Enter Kanfold")));
	TestFalse(TEXT("parade does not fire Reach broken bridge"), Fired(State, TEXT("Reach broken bridge")));
	TestFalse(TEXT("parade does not fire Driven from the village"), Fired(State, TEXT("Driven from the village")));
	const int32 FiredCount = State.FiredOrder.Num();
	const int32 ParadeAgain = SolidBattlePlanTestPrivate::FindAssignedSlot(Squad, TEXT("Parade"));
	TestTrue(TEXT("parade again"), GameMode->SelectBattlePlanSlot(ParadeAgain));
	TestEqual(TEXT("parade events fire once"), GameMode->GetConfiguration()->FiredOrder.Num(), FiredCount);

	TestFalse(
		TEXT("the first key past the assigned plans is empty"),
		GameMode->SelectBattlePlanSlot(GameMode->GetSquad()->GetAssignedCount()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidGameModeBattlePlanSurvivesEnsureTest,
	"SolidCore1.BattlePlan.SelectionSurvivesEnsure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidGameModeBattlePlanSurvivesEnsureTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);

	USolidSquad* Squad = GameMode->GetSquad();
	const int32 ColumnSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Column"));
	TestTrue(TEXT("column"), GameMode->SelectBattlePlanSlot(ColumnSlot));
	const int32 ColumnAfter = GameMode->GetSquad()->GetActiveAssignedSlot();
	TestTrue(TEXT("formation is column"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Column);

	// Enter-town refresh calls EnsureCompanyAndSquad on a timer. That used to
	// rebuild the squad and snap the active plan back to Line.
	GameMode->EnsureCompanyAndSquad();
	TestEqual(TEXT("slot stays column"), GameMode->GetSquad()->GetActiveAssignedSlot(), ColumnAfter);
	TestTrue(TEXT("formation stays column"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Column);

	const int32 ParadeSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Parade"));
	TestTrue(TEXT("parade"), GameMode->SelectBattlePlanSlot(ParadeSlot));
	const int32 ParadeAfter = GameMode->GetSquad()->GetActiveAssignedSlot();
	TestTrue(TEXT("formation is parade"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Parade);
	GameMode->EnsureCompanyAndSquad();
	TestEqual(TEXT("slot stays parade"), GameMode->GetSquad()->GetActiveAssignedSlot(), ParadeAfter);
	TestTrue(TEXT("formation stays parade"),
		GameMode->GetSquad()->GetActiveFormation() == ESolidBattleFormation::Parade);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanGamepadSlotTest,
	"SolidCore1.BattlePlan.GamepadSlot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanGamepadSlotTest::RunTest(const FString& Parameters)
{
	const SolidBattlePlanInput::EFace Faces[] = {
		SolidBattlePlanInput::EFace::Bottom,
		SolidBattlePlanInput::EFace::Right,
		SolidBattlePlanInput::EFace::Left,
		SolidBattlePlanInput::EFace::Top,
	};
	for (int32 Index = 0; Index < SolidBattlePlanInput::FaceCount; ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("no shoulder, face %d"), Index),
			SolidBattlePlanInput::SlotForShoulderAndFace(false, false, Faces[Index]),
			INDEX_NONE);
		TestEqual(
			FString::Printf(TEXT("left shoulder, face %d"), Index),
			SolidBattlePlanInput::SlotForShoulderAndFace(true, false, Faces[Index]),
			Index);
		TestEqual(
			FString::Printf(TEXT("right shoulder, face %d"), Index),
			SolidBattlePlanInput::SlotForShoulderAndFace(false, true, Faces[Index]),
			Index + 4);
		TestEqual(
			FString::Printf(TEXT("both shoulders, face %d"), Index),
			SolidBattlePlanInput::SlotForShoulderAndFace(true, true, Faces[Index]),
			Index);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEmptyKeyTest,
	"SolidCore1.BattlePlan.EmptyKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEmptyKeyTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	Squad->InitializeFromCompany(Company);

	const int32 TightSlot = SolidBattlePlanTestPrivate::FindAssignedSlot(Squad, TEXT("Tight mob"));
	const int32 TightIndex = SolidBattlePlanTestPrivate::FindCompanyPlan(Company, TEXT("Tight mob"));
	const int32 ColumnSlot = SolidBattlePlanTestPrivate::FindAssignedSlot(Squad, TEXT("Column"));
	const int32 LooseSlot = SolidBattlePlanTestPrivate::FindAssignedSlot(Squad, TEXT("Loose mob"));
	TestTrue(TEXT("tight mob is on a key"), TightSlot != INDEX_NONE);
	TestTrue(TEXT("column is on a key"), ColumnSlot != INDEX_NONE);
	TestTrue(TEXT("loose mob is on a key"), LooseSlot != INDEX_NONE);

	SolidSquadBuilder::FDrag Drag;
	Drag.Kind = SolidSquadBuilder::EDrag::Plan;
	Drag.Index = TightIndex;
	Drag.Slot = TightSlot;
	SolidSquadBuilder::FHit Hit;
	Hit.Kind = SolidSquadBuilder::EHit::PlansBox;
	TestTrue(TEXT("tight mob returns to Plans"), SolidSquadBuilder::Drop(Squad, 2, Drag, Hit));
	TestNull(TEXT("that key is empty"), Squad->GetAssignedBattlePlan(TightSlot));
	TestEqual(TEXT("column stays"), Squad->GetAssignedBattlePlan(ColumnSlot)->Name, FString(TEXT("Column")));
	TestEqual(TEXT("loose mob stays"), Squad->GetAssignedBattlePlan(LooseSlot)->Name, FString(TEXT("Loose mob")));
	TestFalse(TEXT("an empty key in the middle does nothing"), Squad->SelectAssignedSlot(TightSlot));
	TestEqual(TEXT("the active key stays F1"), Squad->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("loose mob still selects"), Squad->SelectAssignedSlot(LooseSlot));
	TestTrue(TEXT("loose mob is still wide"), Squad->GetActiveSpacing() == ESolidBattleSpacing::Wide);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanParadeOtherKeyTest,
	"SolidCore1.BattlePlan.ParadeOtherKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanParadeOtherKeyTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	USolidSquad* Squad = GameMode->GetSquad();
	TestNotNull(TEXT("squad"), Squad);

	const int32 FiredBefore = GameMode->GetConfiguration()->FiredOrder.Num();
	const int32 ParadeIndex = SolidBattlePlanTestPrivate::FindCompanyPlan(GameMode->GetCompany(), TEXT("Parade"));
	const int32 ParadeSlot = SolidBattlePlanTestPrivate::SlotForPlan(Squad, TEXT("Parade"));
	TestTrue(TEXT("parade has a key"), ParadeSlot != INDEX_NONE && ParadeSlot != 5);
	SolidSquadBuilder::FDrag Drag;
	Drag.Kind = SolidSquadBuilder::EDrag::Plan;
	Drag.Index = ParadeIndex;
	Drag.Slot = ParadeSlot;
	SolidSquadBuilder::FHit Hit;
	Hit.Kind = SolidSquadBuilder::EHit::KeyBox;
	Hit.Index = 5;
	TestTrue(TEXT("Parade moves onto F6"), SolidSquadBuilder::Drop(Squad, 2, Drag, Hit));
	TestEqual(TEXT("the drop does not fire"), GameMode->GetConfiguration()->FiredOrder.Num(), FiredBefore);
	TestEqual(TEXT("F1 stays the active plan"), Squad->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("F6 now holds Parade"), Squad->GetAssignedBattlePlan(5)->Formation == ESolidBattleFormation::Parade);
	TestNull(TEXT("the old key is empty"), Squad->GetAssignedBattlePlan(ParadeSlot));

	TestTrue(TEXT("F6 parade"), GameMode->SelectBattlePlanSlot(5));
	TestTrue(TEXT("formation parade"), Squad->GetActiveFormation() == ESolidBattleFormation::Parade);
	TestTrue(TEXT("parade unlocks raven sight"), GameMode->IsSightEnabled(ESolidSight::Raven));
	TestEqual(TEXT("parade raises the squad cap"), GameMode->GetMaxSquadSize(), SolidEvents::RosterCount);
	const int32 FiredOnce = GameMode->GetConfiguration()->FiredOrder.Num();
	TestTrue(TEXT("parade fired"), FiredOnce > FiredBefore);
	const int32 ParadeAgain = SolidBattlePlanTestPrivate::FindAssignedSlot(Squad, TEXT("Parade"));
	TestTrue(TEXT("parade again"), GameMode->SelectBattlePlanSlot(ParadeAgain));
	TestEqual(TEXT("parade events fire once"), GameMode->GetConfiguration()->FiredOrder.Num(), FiredOnce);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanNewGameTest,
	"SolidCore1.BattlePlan.NewGame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanNewGameTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeDefaultBattlePlans();
	const TCHAR* Order[] = {
		TEXT("Starter"), TEXT("Attack"), TEXT("Defend"), TEXT("Parade"),
		TEXT("Line"), TEXT("Column"), TEXT("Tight mob"), TEXT("Loose mob"),
	};
	int32 Previous = INDEX_NONE;
	for (const TCHAR* Name : Order)
	{
		const int32 Index = SolidBattlePlanTestPrivate::FindCompanyPlan(Company, Name);
		TestTrue(*FString::Printf(TEXT("%s is in the catalog"), Name), Index != INDEX_NONE && Index > Previous);
		Previous = Index;
		const FSolidBattlePlan* Plan = Company->GetBattlePlan(Index);
		if (FCString::Strcmp(Name, TEXT("Starter")) == 0
			|| FCString::Strcmp(Name, TEXT("Attack")) == 0
			|| FCString::Strcmp(Name, TEXT("Defend")) == 0)
		{
			TestTrue(*FString::Printf(TEXT("%s uses Line"), Name), Plan && Plan->Formation == ESolidBattleFormation::Line);
			TestTrue(*FString::Printf(TEXT("%s uses standard spacing"), Name), Plan && Plan->Spacing == ESolidBattleSpacing::Standard);
		}
	}

	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		AddError(TEXT("a new game"));
		return false;
	}
	TestEqual(TEXT("Starter is the only key"), SolidBattlePlanTestPrivate::AssignedNames(Play.Squad), FString(TEXT("Starter")));
	TestEqual(TEXT("F1 is active"), Play.Squad->GetActiveAssignedSlot(), 0);
	TestEqual(TEXT("the active plan is Starter"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Starter")));
	TestFalse(TEXT("one plan hides the list"), SolidBattlePlanList::IsShown(Play.Squad->GetAssignedCount()));
	TestTrue(TEXT("F1 selects Starter"), Play.GameMode->SelectBattlePlanSlot(0));
	TestFalse(TEXT("F2 is empty"), Play.GameMode->SelectBattlePlanSlot(1));
	TestEqual(TEXT("selecting Starter fires nothing"), Play.State->FiredOrder.Num(), 0);
	TestTrue(TEXT("Starter is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	TestFalse(TEXT("Attack starts off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Attack")));
	TestFalse(TEXT("Line starts off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Line")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEarlyEventsTest,
	"SolidCore1.BattlePlan.EarlyEvents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEarlyEventsTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Enter Iglin leaves plans"), SolidBattlePlanTestPrivate::EventChangesBattlePlan(TEXT("Enter Iglin")));
	TestFalse(TEXT("Reach broken bridge leaves plans"), SolidBattlePlanTestPrivate::EventChangesBattlePlan(TEXT("Reach broken bridge")));
	TestFalse(TEXT("Driven from the village leaves plans"), SolidBattlePlanTestPrivate::EventChangesBattlePlan(TEXT("Driven from the village")));
	TestTrue(TEXT("Killed by monsters edits plans"), SolidBattlePlanTestPrivate::EventChangesBattlePlan(TEXT("Killed by monsters")));

	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	SolidEvents::NotifyOccupiedTown(*Play.State, *Play.Captain, SolidTowns::Iglin);
	SolidEvents::NotifyReachBrokenBridge(*Play.State, *Play.Captain);
	TestTrue(TEXT("the message still leaves Starter on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	TestFalse(TEXT("the message does not enable Attack"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Attack")));
	TestEqual(TEXT("the keys stay on Starter"), SolidBattlePlanTestPrivate::AssignedNames(Play.Squad), FString(TEXT("Starter")));

	Play.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	const TArray<FString> AfterClose = Play.State->EnabledBattlePlans;
	SolidEvents::MarkComplete(*Play.State, *Play.Captain, SolidEvents::IdDrivenFromVillage);
	TestTrue(TEXT("Driven from the village keeps the flags"), Play.State->EnabledBattlePlans == AfterClose);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanMonsterCloseTest,
	"SolidCore1.BattlePlan.MonsterClose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanMonsterCloseTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	Play.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	TestEqual(TEXT("Attack then Defend"), SolidBattlePlanTestPrivate::AssignedNames(Play.Squad), FString(TEXT("Attack, Defend")));
	TestNull(TEXT("F3 is empty"), Play.Squad->GetAssignedBattlePlan(2));
	TestNull(TEXT("F8 is empty"), Play.Squad->GetAssignedBattlePlan(7));
	TestEqual(TEXT("Attack is active"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Attack")));
	TestEqual(TEXT("Attack is on F1"), Play.Squad->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("two plans show the list"), SolidBattlePlanList::IsShown(Play.Squad->GetAssignedCount()));
	TestFalse(TEXT("Starter is off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	TestTrue(TEXT("Defend is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Defend")));

	const int32 Fired = Play.State->FiredOrder.Num();
	TestTrue(TEXT("Attack selects"), Play.GameMode->SelectBattlePlanSlot(0));
	TestTrue(TEXT("Defend selects"), Play.GameMode->SelectBattlePlanSlot(1));
	TestEqual(TEXT("Attack and Defend fire nothing more"), Play.State->FiredOrder.Num(), Fired);

	const FString Detail = USolidJournal::Describe(SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdKilledByMonsters)]);
	TestTrue(TEXT("journal records Starter disabled"), Detail.Contains(TEXT("Starter disabled")));
	TestTrue(TEXT("journal records Attack enabled"), Detail.Contains(TEXT("Attack enabled")));
	TestTrue(TEXT("journal records Defend enabled"), Detail.Contains(TEXT("Defend enabled")));
	TestTrue(TEXT("the cave line stays"), Detail.Contains(TEXT("Cave enabled")));

	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(*Play.State, *Play.Captain, SolidEvents::Change::EnableBattlePlan(TEXT("Attack")), Result);
	SolidEvents::ApplyChange(*Play.State, *Play.Captain, SolidEvents::Change::DisableBattlePlan(TEXT("Starter")), Result);
	TestEqual(TEXT("Attack stays on once"), Play.State->EnabledBattlePlans.FilterByPredicate([](const FString& Name) { return Name == TEXT("Attack"); }).Num(), 1);
	TestFalse(TEXT("Starter stays off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	const TArray<FString> Flags = Play.State->EnabledBattlePlans;
	SolidEvents::ApplyChange(*Play.State, *Play.Captain, SolidEvents::Change::EnableBattlePlan(TEXT("Not a plan")), Result);
	TestTrue(TEXT("an unknown plan changes nothing"), Play.State->EnabledBattlePlans == Flags);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEnterRelionTest,
	"SolidCore1.BattlePlan.EnterRelion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEnterRelionTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	Play.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	SolidEvents::NotifyOccupiedTown(*Play.State, *Play.Captain, SolidTowns::Iglin);
	const SolidEvents::FResult Relion = SolidEvents::NotifyOccupiedTown(*Play.State, *Play.Captain, SolidTowns::Relion);
	TestTrue(TEXT("Enter Relion fires"), Relion.bFired);
	Play.Sync();
	TestEqual(
		TEXT("Attack, Defend, Parade"),
		SolidBattlePlanTestPrivate::AssignedNames(Play.Squad),
		FString(TEXT("Attack, Defend, Parade")));
	TestEqual(TEXT("Attack stays active"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Attack")));
	TestEqual(TEXT("Parade is on F3"), Play.Squad->GetAssignedBattlePlan(2)->Name, FString(TEXT("Parade")));
	TestNull(TEXT("F4 is empty"), Play.Squad->GetAssignedBattlePlan(3));
	TestTrue(TEXT("the list is shown"), SolidBattlePlanList::IsShown(Play.Squad->GetAssignedCount()));
	const FString Detail = USolidJournal::Describe(SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdEnterRelion)]);
	TestTrue(TEXT("journal records Parade enabled"), Detail.Contains(TEXT("Parade enabled")));
	TestTrue(TEXT("Relion still repairs the bridge"), Detail.Contains(TEXT("Iglin bridge repaired")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanActiveSurvivesEnableTest,
	"SolidCore1.BattlePlan.ActiveSurvivesEnable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanActiveSurvivesEnableTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	Play.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("Defend can be active"), Play.Squad->SelectAssignedSlot(1));
	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(*Play.State, *Play.Captain, SolidEvents::Change::EnableBattlePlan(TEXT("Parade")), Result);
	Play.Sync();
	TestEqual(TEXT("Defend stays active"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Defend")));
	TestEqual(
		TEXT("Parade joins after Defend"),
		SolidBattlePlanTestPrivate::AssignedNames(Play.Squad),
		FString(TEXT("Attack, Defend, Parade")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEnterKanfoldTest,
	"SolidCore1.BattlePlan.EnterKanfold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEnterKanfoldTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Without;
	if (!Without.Open())
	{
		return false;
	}
	Without.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	const SolidEvents::FResult Kanfold = SolidEvents::NotifyOccupiedTown(*Without.State, *Without.Captain, SolidTowns::Kanfold);
	TestTrue(TEXT("Enter Kanfold fires"), Kanfold.bFired);
	Without.Sync();
	TestEqual(TEXT("the squad cap is 2"), Without.State->MaxSquadSize, 2);
	TestEqual(
		TEXT("the four formations"),
		SolidBattlePlanTestPrivate::AssignedNames(Without.Squad),
		FString(TEXT("Line, Column, Tight mob, Loose mob")));
	TestEqual(TEXT("Attack becomes Line"), Without.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Line")));
	TestFalse(TEXT("Starter is off"), SolidEvents::IsBattlePlanEnabled(*Without.State, TEXT("Starter")));
	TestFalse(TEXT("Attack is off"), SolidEvents::IsBattlePlanEnabled(*Without.State, TEXT("Attack")));
	TestFalse(TEXT("Defend is off"), SolidEvents::IsBattlePlanEnabled(*Without.State, TEXT("Defend")));
	TestFalse(TEXT("Parade stays off"), SolidEvents::IsBattlePlanEnabled(*Without.State, TEXT("Parade")));
	TestNull(TEXT("F5 is empty"), Without.Squad->GetAssignedBattlePlan(4));
	const int32 Fired = Without.State->FiredOrder.Num();
	TestTrue(TEXT("Line selects"), Without.GameMode->SelectBattlePlanSlot(0));
	TestEqual(TEXT("Line does not fire a town"), Without.State->FiredOrder.Num(), Fired);

	SolidBattlePlanTestPrivate::FPlay With;
	if (!With.Open())
	{
		return false;
	}
	With.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Iglin);
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Relion);
	With.Sync();
	const int32 ParadeSlot = SolidBattlePlanTestPrivate::FindAssignedSlot(With.Squad, TEXT("Parade"));
	TestTrue(TEXT("Parade is selected"), With.Squad->SelectAssignedSlot(ParadeSlot));
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Kanfold);
	With.Sync();
	TestEqual(
		TEXT("Parade leads the four"),
		SolidBattlePlanTestPrivate::AssignedNames(With.Squad),
		FString(TEXT("Parade, Line, Column, Tight mob, Loose mob")));
	TestEqual(TEXT("Parade stays active"), With.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Parade")));
	TestEqual(TEXT("Parade is on F1"), With.Squad->GetActiveAssignedSlot(), 0);
	TestNull(TEXT("F6 is empty"), With.Squad->GetAssignedBattlePlan(5));

	const FString Detail = USolidJournal::Describe(SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdEnterKanfold)]);
	TestTrue(
		TEXT("journal records the Kanfold plan edits"),
		Detail.Contains(TEXT("Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEnterDragongateTest,
	"SolidCore1.BattlePlan.EnterDragongate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEnterDragongateTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Without;
	if (!Without.Open())
	{
		return false;
	}
	Without.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	const SolidEvents::FResult Gate = SolidEvents::NotifyOccupiedTown(*Without.State, *Without.Captain, SolidTowns::Dragongate);
	TestTrue(TEXT("Enter Dragongate fires"), Gate.bFired);
	Without.Sync();
	TestEqual(TEXT("the squad cap is the company"), Without.State->MaxSquadSize, SolidEvents::RosterCount);
	TestEqual(
		TEXT("the four formations"),
		SolidBattlePlanTestPrivate::AssignedNames(Without.Squad),
		FString(TEXT("Line, Column, Tight mob, Loose mob")));
	TestEqual(TEXT("Attack becomes Line"), Without.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Line")));
	TestFalse(TEXT("Parade stays off"), SolidEvents::IsBattlePlanEnabled(*Without.State, TEXT("Parade")));
	TestFalse(TEXT("Dragongate does not enter the cave"), Gate.bEnterStronghold);

	SolidBattlePlanTestPrivate::FPlay With;
	if (!With.Open())
	{
		return false;
	}
	With.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Iglin);
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Relion);
	With.Sync();
	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Dragongate);
	With.Sync();
	TestEqual(
		TEXT("Parade leads the four"),
		SolidBattlePlanTestPrivate::AssignedNames(With.Squad),
		FString(TEXT("Parade, Line, Column, Tight mob, Loose mob")));
	TestEqual(TEXT("Attack becomes Parade"), With.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Parade")));
	TestEqual(TEXT("Parade is on F1"), With.Squad->GetActiveAssignedSlot(), 0);
	TestTrue(TEXT("Parade stays on"), SolidEvents::IsBattlePlanEnabled(*With.State, TEXT("Parade")));

	SolidEvents::NotifyOccupiedTown(*With.State, *With.Captain, SolidTowns::Kanfold);
	With.Sync();
	TestEqual(
		TEXT("Kanfold then Dragongate does not duplicate Line"),
		With.State->EnabledBattlePlans.FilterByPredicate([](const FString& Name) { return Name == TEXT("Line"); }).Num(),
		1);
	TestEqual(
		TEXT("the keys stay Parade and the four"),
		SolidBattlePlanTestPrivate::AssignedNames(With.Squad),
		FString(TEXT("Parade, Line, Column, Tight mob, Loose mob")));

	const FString Detail = USolidJournal::Describe(SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdEnterDragongate)]);
	TestTrue(
		TEXT("journal records the Dragongate plan edits"),
		Detail.Contains(TEXT("Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled")));
	TestTrue(TEXT("Dragongate still titles Captain"), Detail.Contains(TEXT("Title Captain")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanParadeSkipsKanfoldTest,
	"SolidCore1.BattlePlan.ParadeSkipsKanfold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanParadeSkipsKanfoldTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	const int32 ParadeSlot = SolidBattlePlanTestPrivate::SlotForPlan(Play.Squad, TEXT("Parade"));
	TestTrue(TEXT("Parade selects"), Play.GameMode->SelectBattlePlanSlot(ParadeSlot));
	auto Fired = [](const USolidGameState& State, const TCHAR* Name)
	{
		for (const int32 EventIndex : State.FiredOrder)
		{
			if (EventIndex >= 0
				&& EventIndex < SolidEvents::DefinitionCount
				&& FCString::Strcmp(SolidEvents::Definitions[EventIndex].Name, Name) == 0)
			{
				return true;
			}
		}
		return false;
	};
	TestTrue(TEXT("Parade fires Enter Iglin"), Fired(*Play.State, TEXT("Enter Iglin")));
	TestTrue(TEXT("Parade fires Enter Relion"), Fired(*Play.State, TEXT("Enter Relion")));
	TestTrue(TEXT("Parade fires Enter Dragongate"), Fired(*Play.State, TEXT("Enter Dragongate")));
	TestFalse(TEXT("Parade skips Enter Kanfold"), Fired(*Play.State, TEXT("Enter Kanfold")));
	TestTrue(TEXT("Parade is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Parade")));
	TestTrue(TEXT("Line is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Line")));
	TestTrue(TEXT("Column is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Column")));
	TestTrue(TEXT("Tight mob is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Tight mob")));
	TestTrue(TEXT("Loose mob is on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Loose mob")));
	TestFalse(TEXT("Starter is off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	TestFalse(TEXT("Attack is off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Attack")));
	TestFalse(TEXT("Defend is off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Defend")));
	TestEqual(TEXT("Parade stays the active plan"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Parade")));
	TestTrue(TEXT("the list is shown"), SolidBattlePlanList::IsShown(Play.Squad->GetAssignedCount()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanEightPlansStayTest,
	"SolidCore1.BattlePlan.EightPlansStay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanEightPlansStayTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	const TCHAR* Names[] = {
		TEXT("Starter"), TEXT("Attack"), TEXT("Defend"), TEXT("Parade"),
		TEXT("Line"), TEXT("Column"), TEXT("Tight mob"), TEXT("Loose mob"),
	};
	SolidEvents::FResult Result;
	for (const TCHAR* Name : Names)
	{
		SolidEvents::ApplyChange(*Play.State, *Play.Captain, SolidEvents::Change::EnableBattlePlan(Name), Result);
	}
	FSolidBattlePlan Extra;
	Extra.Name = TEXT("Skirmish");
	Extra.Formation = ESolidBattleFormation::Line;
	Extra.Spacing = ESolidBattleSpacing::Standard;
	const int32 Skirmish = Play.Company->AddBattlePlan(Extra);
	TArray<int32> Custom;
	Custom.Add(Skirmish);
	Custom.Add(SolidBattlePlanTestPrivate::FindCompanyPlan(Play.Company, TEXT("Line")));
	TestTrue(TEXT("a custom selection"), Play.Squad->SetAssignedBattlePlans(Custom));
	const TArray<int32> Before = Play.Squad->GetAssignedCompanyPlanIndices();
	Play.Sync();
	TestTrue(TEXT("eight enabled plans leave the keys"), Play.Squad->GetAssignedCompanyPlanIndices() == Before);

	SolidBattlePlanTestPrivate::FPlay Few;
	if (!Few.Open())
	{
		return false;
	}
	const int32 Added = Few.Company->AddBattlePlan(Extra);
	SolidEvents::ApplyChange(*Few.State, *Few.Captain, SolidEvents::Change::EnableBattlePlan(TEXT("Skirmish")), Result);
	Few.Sync();
	TestEqual(
		TEXT("an enabled extra plan joins in catalog order"),
		SolidBattlePlanTestPrivate::AssignedNames(Few.Squad),
		FString(TEXT("Starter, Skirmish")));
	TestEqual(TEXT("Skirmish is the added plan"), Few.Squad->GetAssignedBattlePlan(1)->Name, Few.Company->GetBattlePlan(Added)->Name);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidBattlePlanTripKeepsPlansTest,
	"SolidCore1.BattlePlan.TripKeepsPlans",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidBattlePlanTripKeepsPlansTest::RunTest(const FString& Parameters)
{
	SolidBattlePlanTestPrivate::FPlay Play;
	if (!Play.Open())
	{
		return false;
	}
	Play.GameMode->NoteMessageClosed(SolidMessages::IdKilledByMonsters);
	TestTrue(TEXT("Defend is active for the trip"), Play.Squad->SelectAssignedSlot(1));
	const TArray<int32> Saved = Play.Squad->GetAssignedCompanyPlanIndices();
	TArray<int32> Mess;
	Mess.Add(SolidBattlePlanTestPrivate::FindCompanyPlan(Play.Company, TEXT("Starter")));
	Play.Squad->SetAssignedBattlePlans(Mess);
	SolidEvents::RestoreAssignedBattlePlans(*Play.Squad, *Play.State, Saved, 1, true, 0, 1);
	TestEqual(
		TEXT("the trip keeps Attack and Defend"),
		SolidBattlePlanTestPrivate::AssignedNames(Play.Squad),
		FString(TEXT("Attack, Defend")));
	TestEqual(TEXT("the trip keeps Defend active"), Play.Squad->GetActiveBattlePlan()->Name, FString(TEXT("Defend")));
	TestTrue(TEXT("Attack stays on"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Attack")));
	TestFalse(TEXT("Starter stays off"), SolidEvents::IsBattlePlanEnabled(*Play.State, TEXT("Starter")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
