#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "Journal/SolidJournal.h"
#include "SolidGameState.h"
#include "Company/SolidCaptain.h"
#include "SolidWorldTime.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidJournalTestPrivate
{
	int32 FindEntry(const TArray<SolidJournal::FRow>& Rows, const TCHAR* Text, const int32 Start = 0)
	{
		for (int32 Index = Start; Index < Rows.Num(); ++Index)
		{
			const SolidJournal::FRow& Row = Rows[Index];
			if (Row.Kind == SolidJournal::ERowKind::Entry && Row.Text == Text)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool ContainsText(const TArray<SolidJournal::FRow>& Rows, const TCHAR* Text)
	{
		for (const SolidJournal::FRow& Row : Rows)
		{
			if (Row.Text.Contains(Text))
			{
				return true;
			}
		}
		return false;
	}

	FString DescribeEvent(const TCHAR* EventName)
	{
		if (!EventName)
		{
			return FString();
		}
		for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
		{
			if (FCString::Strcmp(SolidEvents::Definitions[Index].Name, EventName) == 0)
			{
				return SolidJournal::Describe(SolidEvents::Definitions[Index]);
			}
		}
		return FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalEmptyTest,
	"SolidCore1.Journal.Empty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalEmptyTest::RunTest(const FString& Parameters)
{
	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(*NewObject<USolidGameState>(), Rows);
	TestTrue(TEXT("title"), SolidJournalTestPrivate::ContainsText(Rows, TEXT("Journal")));
	int32 Heading = INDEX_NONE;
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (Rows[Index].Kind == SolidJournal::ERowKind::Heading)
		{
			Heading = Index;
			break;
		}
	}
	TestTrue(TEXT("column headings"), Heading != INDEX_NONE);
	if (Heading != INDEX_NONE)
	{
		TestEqual(TEXT("time heading"), Rows[Heading].Stamp, FString(SolidJournal::TimeHeading));
		TestEqual(TEXT("summary heading"), Rows[Heading].Text, FString(SolidJournal::SummaryHeading));
	}
	TestTrue(TEXT("empty copy"), SolidJournalTestPrivate::ContainsText(Rows, TEXT("No events yet.")));
	TestTrue(TEXT("close hint"), SolidJournalTestPrivate::ContainsText(Rows, SolidJournal::CloseHint));
	TestFalse(TEXT("no Relion yet"), SolidJournalTestPrivate::ContainsText(Rows, TEXT("Relion")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalTriggeredOrderTest,
	"SolidCore1.Journal.TriggeredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalTriggeredOrderTest::RunTest(const FString& Parameters)
{
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	TArray<SolidJournal::FRow> AtStart;
	SolidJournal::CollectRows(State, AtStart);
	const int32 Iglin = SolidJournalTestPrivate::FindEntry(AtStart, TEXT("Enter Iglin"));
	TestTrue(TEXT("Iglin is the first entry"), Iglin != INDEX_NONE);
	if (Iglin != INDEX_NONE)
	{
		TestEqual(TEXT("Iglin stamp"), AtStart[Iglin].Stamp, FString(TEXT("00:00, 200/45 AtF")));
		const FString IglinChanges = SolidJournalTestPrivate::DescribeEvent(TEXT("Enter Iglin"));
		const bool bIglinDetail = Iglin + 1 < AtStart.Num()
			&& AtStart[Iglin + 1].Kind == SolidJournal::ERowKind::Detail;
		TestEqual(TEXT("Iglin detail follows the event"), bIglinDetail, !IglinChanges.IsEmpty());
		if (bIglinDetail)
		{
			TestEqual(TEXT("Iglin detail is the event"), AtStart[Iglin + 1].Text, IglinChanges);
		}
	}
	TestFalse(TEXT("Iglin does not mention Relion"), SolidJournalTestPrivate::ContainsText(AtStart, TEXT("Relion")));
	TestFalse(TEXT("Iglin is not the empty journal"), SolidJournalTestPrivate::ContainsText(AtStart, TEXT("No events yet.")));

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	TArray<SolidJournal::FRow> AfterRelion;
	SolidJournal::CollectRows(State, AfterRelion);
	const int32 IglinStill = SolidJournalTestPrivate::FindEntry(AfterRelion, TEXT("Enter Iglin"));
	const int32 Relion = SolidJournalTestPrivate::FindEntry(AfterRelion, TEXT("Enter Relion"));
	TestTrue(TEXT("Iglin stays first"), IglinStill != INDEX_NONE && Relion != INDEX_NONE && IglinStill < Relion);
	TestTrue(TEXT("lists Enter Relion second"), Relion != INDEX_NONE);
	TestTrue(TEXT("Relion enables raven sight"), SolidJournalTestPrivate::ContainsText(AfterRelion, TEXT("Raven sight enabled")));
	TestTrue(TEXT("Relion squad is Sam"), SolidJournalTestPrivate::ContainsText(AfterRelion, TEXT("Squad 1: Sam")));
	TestFalse(TEXT("Alex is not in yet"), SolidJournalTestPrivate::ContainsText(AfterRelion, TEXT("Alex")));
	if (Relion != INDEX_NONE)
	{
		TestEqual(TEXT("Relion stamp stays in its column"), AfterRelion[Relion].Stamp, FString(TEXT("00:00, 200/45 AtF")));
		TestTrue(TEXT("Relion detail follows the entry"), Relion + 1 < AfterRelion.Num()
			&& AfterRelion[Relion + 1].Kind == SolidJournal::ERowKind::Detail);
		if (Relion + 1 < AfterRelion.Num())
		{
			TestTrue(TEXT("detail leaves the time column empty"), AfterRelion[Relion + 1].Stamp.IsEmpty());
		}
	}

	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 2);
	TArray<SolidJournal::FRow> AfterKanfold;
	SolidJournal::CollectRows(State, AfterKanfold);
	const int32 IglinThird = SolidJournalTestPrivate::FindEntry(AfterKanfold, TEXT("Enter Iglin"));
	const int32 RelionThird = SolidJournalTestPrivate::FindEntry(AfterKanfold, TEXT("Enter Relion"));
	const int32 Kanfold = SolidJournalTestPrivate::FindEntry(AfterKanfold, TEXT("Enter Kanfold"));
	TestTrue(TEXT("Iglin stays first"), IglinThird != INDEX_NONE && RelionThird != INDEX_NONE && IglinThird < RelionThird);
	TestTrue(TEXT("Relion stays second"), RelionThird != INDEX_NONE && Kanfold != INDEX_NONE && RelionThird < Kanfold);
	TestTrue(TEXT("Kanfold is third"), Kanfold != INDEX_NONE);
	TestTrue(TEXT("Kanfold squad is both"), SolidJournalTestPrivate::ContainsText(AfterKanfold, TEXT("Squad 2: Sam, Alex")));
	TestFalse(TEXT("filled journal is not empty"), SolidJournalTestPrivate::ContainsText(AfterKanfold, TEXT("No events yet.")));

	USolidGameState& Skipped = *NewObject<USolidGameState>();
	USolidCaptain& SkippedCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 2);
	SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 0);
	SolidEvents::NotifyOccupiedTown(Skipped, SkippedCaptain, 1);
	TArray<SolidJournal::FRow> Reversed;
	SolidJournal::CollectRows(Skipped, Reversed);
	const int32 KanfoldFirst = SolidJournalTestPrivate::FindEntry(Reversed, TEXT("Enter Kanfold"));
	const int32 IglinSecond = SolidJournalTestPrivate::FindEntry(Reversed, TEXT("Enter Iglin"));
	const int32 RelionSecond = SolidJournalTestPrivate::FindEntry(Reversed, TEXT("Enter Relion"));
	TestTrue(TEXT("Kanfold is entry 1 when visited first"), KanfoldFirst != INDEX_NONE);
	TestTrue(
		TEXT("Iglin is recorded before Relion"),
		IglinSecond != INDEX_NONE && KanfoldFirst != INDEX_NONE && KanfoldFirst < IglinSecond);
	TestTrue(
		TEXT("Relion follows Iglin"),
		RelionSecond != INDEX_NONE && IglinSecond != INDEX_NONE && IglinSecond < RelionSecond);
	if (KanfoldFirst != INDEX_NONE)
	{
		TestEqual(TEXT("Kanfold stamp"), Reversed[KanfoldFirst].Stamp, FString(TEXT("00:00, 200/45 AtF")));
	}
	if (RelionSecond != INDEX_NONE)
	{
		TestEqual(TEXT("Relion stamp"), Reversed[RelionSecond].Stamp, FString(TEXT("00:00, 200/45 AtF")));
	}
	FString KanfoldChanges;
	for (int32 Index = 0; Index < SolidEvents::DefinitionCount; ++Index)
	{
		if (FCString::Strcmp(SolidEvents::Definitions[Index].Name, TEXT("Enter Kanfold")) == 0)
		{
			KanfoldChanges = SolidJournal::Describe(SolidEvents::Definitions[Index]);
			break;
		}
	}
	const bool bKanfoldSight = KanfoldFirst != INDEX_NONE
		&& KanfoldFirst + 1 < Reversed.Num()
		&& Reversed[KanfoldFirst + 1].Kind == SolidJournal::ERowKind::Detail
		&& Reversed[KanfoldFirst + 1].Text.Contains(TEXT("Raven sight"));
	TestEqual(
		TEXT("Kanfold sight line matches the event"),
		bKanfoldSight,
		KanfoldChanges.Contains(TEXT("Raven sight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalCaptainTitleTest,
	"SolidCore1.Journal.CaptainTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalCaptainTitleTest::RunTest(const FString& Parameters)
{
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0);
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 1);
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 2);

	TArray<SolidJournal::FRow> Rows;
	SolidJournal::CollectRows(State, Rows);
	TestTrue(TEXT("Relion records Vagrant"), SolidJournalTestPrivate::ContainsText(Rows, TEXT("Title Vagrant")));
	TestTrue(TEXT("Kanfold records Ranger"), SolidJournalTestPrivate::ContainsText(Rows, TEXT("Title Ranger")));
	const int32 Iglin = SolidJournalTestPrivate::FindEntry(Rows, TEXT("Enter Iglin"));
	TestTrue(TEXT("Iglin is present"), Iglin != INDEX_NONE);
	const FString IglinChanges = SolidJournalTestPrivate::DescribeEvent(TEXT("Enter Iglin"));
	const bool bIglinTitle = Iglin != INDEX_NONE
		&& Iglin + 1 < Rows.Num()
		&& Rows[Iglin + 1].Kind == SolidJournal::ERowKind::Detail
		&& Rows[Iglin + 1].Text.Contains(TEXT("Title"));
	TestEqual(
		TEXT("Iglin title line follows the event"),
		bIglinTitle,
		IglinChanges.Contains(TEXT("Title")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalWorldTimeTest,
	"SolidCore1.Journal.WorldTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalWorldTimeTest::RunTest(const FString& Parameters)
{
	const SolidWorldTime::FStamp Ninth = SolidWorldTime::FromSession(9, 84.0 * 60.0);
	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCaptain& StateCaptain = *NewObject<USolidCaptain>();
	SolidEvents::NotifyOccupiedTown(State, StateCaptain, 0, Ninth);

	TArray<SolidJournal::FRow> Lines;
	SolidJournal::CollectRows(State, Lines);
	const int32 Iglin = SolidJournalTestPrivate::FindEntry(Lines, TEXT("Enter Iglin"));
	TestTrue(TEXT("ninth session lists Iglin"), Iglin != INDEX_NONE);
	if (Iglin != INDEX_NONE)
	{
		TestEqual(TEXT("ninth session stamps Iglin"), Lines[Iglin].Stamp, FString(TEXT("01:24, 200/53 AtF")));
	}
	TestTrue(TEXT("Iglin is an event line"), State.Journal.Num() == 1
		&& State.Journal[0].Type == SolidJournal::EJournalType::Event);

	SolidEvents::RememberStronghold(State, Ninth, /*bEnter=*/true);
	const SolidWorldTime::FStamp Later = SolidWorldTime::FromSession(9, 90.0 * 60.0);
	SolidEvents::RememberStronghold(State, Later, /*bEnter=*/false);
	SolidEvents::RememberStronghold(State, Later, /*bEnter=*/true);

	TArray<SolidJournal::FRow> AfterCave;
	SolidJournal::CollectRows(State, AfterCave);
	TestEqual(TEXT("cave trips are noted again"), State.Journal.Num(), 4);
	const int32 EnterAt = SolidJournalTestPrivate::FindEntry(AfterCave, TEXT("Enter stronghold"));
	const int32 LeaveAt = SolidJournalTestPrivate::FindEntry(AfterCave, TEXT("Exit stronghold"));
	const int32 EnterAgain = SolidJournalTestPrivate::FindEntry(AfterCave, TEXT("Enter stronghold"), LeaveAt == INDEX_NONE ? 0 : LeaveAt + 1);
	TestTrue(TEXT("enter is recorded"), EnterAt != INDEX_NONE);
	TestTrue(TEXT("exit is recorded"), LeaveAt != INDEX_NONE && LeaveAt > EnterAt);
	TestTrue(TEXT("a second enter is its own line"), EnterAgain != INDEX_NONE && EnterAgain > LeaveAt);
	if (LeaveAt != INDEX_NONE)
	{
		TestEqual(TEXT("exit uses the later minute"), AfterCave[LeaveAt].Stamp, FString(TEXT("01:30, 200/53 AtF")));
		TestEqual(TEXT("exit text stays in its column"), AfterCave[LeaveAt].Text, FString(TEXT("Exit stronghold")));
	}
	TestFalse(TEXT("the line does not name the dungeon"), SolidJournalTestPrivate::ContainsText(AfterCave, TEXT("Meagre")));
	TestFalse(TEXT("the line does not name the type"), SolidJournalTestPrivate::ContainsText(AfterCave, TEXT("Dungeon")));

	TestEqual(TEXT("the play state keeps the journal"), State.Journal.Num(), 4);
	TestEqual(TEXT("exit text stays"), State.Journal[2].Text, FString(TEXT("Exit stronghold")));
	TestTrue(TEXT("exit is a stronghold line"), State.Journal[2].Type == SolidJournal::EJournalType::Stronghold);
	TestEqual(TEXT("a stronghold line has no event index"), State.Journal[2].EventIndex, INDEX_NONE);

	USolidGameState& Marked = *NewObject<USolidGameState>();
	USolidCaptain& MarkedCaptain = *NewObject<USolidCaptain>();
	SolidJournal::FJournalEntry Visit;
	Visit.Type = SolidJournal::EJournalType::Stronghold;
	Visit.Text = TEXT("Enter stronghold");
	Visit.EventIndex = 1;
	Marked.Journal.Add(Visit);
	TArray<SolidJournal::FRow> MarkedRows;
	SolidJournal::CollectRows(Marked, MarkedRows);
	TestTrue(TEXT("stronghold text is the summary"), SolidJournalTestPrivate::ContainsText(MarkedRows, TEXT("Enter stronghold")));
	TestFalse(TEXT("a stronghold line does not describe an event"), SolidJournalTestPrivate::ContainsText(MarkedRows, TEXT("Raven sight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalPageTest,
	"SolidCore1.Journal.Page",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalPageTest::RunTest(const FString& Parameters)
{
	float Wide = 0.f;
	float Tall = 0.f;
	SolidJournal::BoardSize(1920.f, 1080.f, 200.f, 80.f, Wide, Tall);
	TestTrue(TEXT("a roomy view uses the width share"), FMath::IsNearlyEqual(Wide, 1920.f * SolidJournal::BoardWidthFraction, 0.5f));
	TestTrue(TEXT("a roomy view uses the height share"), FMath::IsNearlyEqual(Tall, 1080.f * SolidJournal::BoardHeightFraction, 0.5f));
	TestTrue(TEXT("the page is wider than the old shrink-wrap"), Wide >= SolidJournal::MinBoardWidth);
	TestTrue(TEXT("the page is taller than the old shrink-wrap"), Tall >= SolidJournal::MinBoardHeight);

	float FittedW = 0.f;
	float FittedH = 0.f;
	SolidJournal::BoardSize(800.f, 600.f, 100.f, 40.f, FittedW, FittedH);
	TestTrue(TEXT("a small view keeps the board on screen"),
		FittedW <= 800.f - SolidJournal::BoardMargin * 2.f + 0.5f
		&& FittedH <= 600.f - SolidJournal::BoardMargin * 2.f + 0.5f
		&& FittedW > 0.f
		&& FittedH > 0.f);

	float Grown = 0.f;
	float GrownH = 0.f;
	SolidJournal::BoardSize(1920.f, 1080.f, 2000.f, 40.f, Grown, GrownH);
	TestTrue(TEXT("wide text stays inside the view"),
		FMath::IsNearlyEqual(Grown, 1920.f - SolidJournal::BoardMargin * 2.f, 0.5f));

	const SolidJournal::FColumns Columns = SolidJournal::Columns(36.f, 180.f);
	TestTrue(TEXT("the entry column clears the time column"),
		Columns.TextX >= Columns.StampX + 180.f + SolidJournal::ColumnGap - 0.1f);

	const SolidJournal::FColumns Empty = SolidJournal::Columns(36.f, 0.f);
	TestTrue(TEXT("an empty time column does not indent the text"), FMath::IsNearlyEqual(Empty.TextX, 36.f, 0.1f));

	const float BoardX = 100.f;
	const float BoardY = 200.f;
	const float BoardW = 900.f;
	const float Band = 9.f;
	const SolidJournal::FTitlePlate Plate = SolidJournal::TitlePlate(BoardX, BoardY, BoardW, 70.f, 18.f, Band);
	const float BandBottom = BoardY + Band;
	TestTrue(TEXT("the title covers the top border"), Plate.Y < BandBottom && Plate.Y + Plate.H > BoardY);
	TestTrue(TEXT("the title is centered on the top border"),
		FMath::IsNearlyEqual(Plate.X + Plate.W * 0.5f, BoardX + BoardW * 0.5f, 0.5f));
	TestTrue(TEXT("the title plate is narrower than the board"), Plate.W < BoardW);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
