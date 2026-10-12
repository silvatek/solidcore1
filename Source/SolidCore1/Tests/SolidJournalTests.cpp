#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "Menus/SolidJournal.h"
#include "SolidStronghold.h"
#include "SolidWorldTime.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidJournalTestPrivate
{
	FString JoinLines(const TArray<FString>& Lines)
	{
		FString Joined;
		for (const FString& Line : Lines)
		{
			Joined += Line;
			Joined += TEXT("\n");
		}
		return Joined;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalEmptyTest,
	"SolidCore1.Journal.Empty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalEmptyTest::RunTest(const FString& Parameters)
{
	TArray<FString> Lines;
	SolidJournal::CollectLines(SolidEvents::FState(), Lines);
	const FString Joined = SolidJournalTestPrivate::JoinLines(Lines);
	TestTrue(TEXT("title"), Joined.Contains(TEXT("Journal")));
	TestTrue(TEXT("empty copy"), Joined.Contains(TEXT("No events yet.")));
	TestTrue(TEXT("close hint"), Joined.Contains(SolidJournal::CloseHint));
	TestFalse(TEXT("no Relion yet"), Joined.Contains(TEXT("Relion")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalTriggeredOrderTest,
	"SolidCore1.Journal.TriggeredOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalTriggeredOrderTest::RunTest(const FString& Parameters)
{
	SolidEvents::FState State;
	SolidEvents::NotifyOccupiedTown(State, 0);
	TArray<FString> AtStart;
	SolidJournal::CollectLines(State, AtStart);
	const FString IglinText = SolidJournalTestPrivate::JoinLines(AtStart);
	TestTrue(TEXT("Iglin is the first entry"), IglinText.Contains(TEXT("00:00, 200/45 AtF  Enter Iglin")));
	TestFalse(TEXT("Iglin has no change line"), IglinText.Contains(TEXT("    ")));
	TestFalse(TEXT("Iglin does not mention Relion"), IglinText.Contains(TEXT("Relion")));
	TestFalse(TEXT("Iglin is not the empty journal"), IglinText.Contains(TEXT("No events yet.")));

	SolidEvents::NotifyOccupiedTown(State, 1);
	TArray<FString> AfterRelion;
	SolidJournal::CollectLines(State, AfterRelion);
	const FString RelionText = SolidJournalTestPrivate::JoinLines(AfterRelion);
	TestTrue(TEXT("Iglin stays first"), RelionText.Contains(TEXT("00:00, 200/45 AtF  Enter Iglin")));
	TestTrue(TEXT("lists Enter Relion second"), RelionText.Contains(TEXT("00:00, 200/45 AtF  Enter Relion")));
	TestTrue(TEXT("Relion enables raven sight"), RelionText.Contains(TEXT("Raven sight enabled")));
	TestTrue(TEXT("Relion party is Sam"), RelionText.Contains(TEXT("Party 1: Sam")));
	TestFalse(TEXT("Alex is not in yet"), RelionText.Contains(TEXT("Alex")));

	SolidEvents::NotifyOccupiedTown(State, 2);
	TArray<FString> AfterKanfold;
	SolidJournal::CollectLines(State, AfterKanfold);
	const FString Both = SolidJournalTestPrivate::JoinLines(AfterKanfold);
	TestTrue(TEXT("Iglin stays first"), Both.Contains(TEXT("00:00, 200/45 AtF  Enter Iglin")));
	TestTrue(TEXT("Relion stays second"), Both.Contains(TEXT("00:00, 200/45 AtF  Enter Relion")));
	TestTrue(TEXT("Kanfold is third"), Both.Contains(TEXT("00:00, 200/45 AtF  Enter Kanfold")));
	TestTrue(TEXT("Kanfold party is both"), Both.Contains(TEXT("Party 2: Sam, Alex")));
	TestFalse(TEXT("filled journal is not empty"), Both.Contains(TEXT("No events yet.")));

	const int32 RelionAt = Both.Find(TEXT("Enter Relion"));
	const int32 KanfoldAt = Both.Find(TEXT("Enter Kanfold"));
	TestTrue(TEXT("Relion is listed before Kanfold"), RelionAt != INDEX_NONE && KanfoldAt != INDEX_NONE && RelionAt < KanfoldAt);

	SolidEvents::FState Skipped;
	SolidEvents::NotifyOccupiedTown(Skipped, 2);
	SolidEvents::NotifyOccupiedTown(Skipped, 1);
	TArray<FString> Reversed;
	SolidJournal::CollectLines(Skipped, Reversed);
	const FString ReverseText = SolidJournalTestPrivate::JoinLines(Reversed);
	const int32 KanfoldFirst = ReverseText.Find(TEXT("Enter Kanfold"));
	const int32 RelionSecond = ReverseText.Find(TEXT("Enter Relion"));
	TestTrue(TEXT("Kanfold is entry 1 when visited first"), ReverseText.Contains(TEXT("00:00, 200/45 AtF  Enter Kanfold")));
	TestTrue(TEXT("Relion is entry 2 when visited second"), ReverseText.Contains(TEXT("00:00, 200/45 AtF  Enter Relion")));
	TestTrue(TEXT("visit order is kept"), KanfoldFirst != INDEX_NONE && RelionSecond != INDEX_NONE && KanfoldFirst < RelionSecond);
	TestFalse(TEXT("skipped Relion did not enable sight in that entry"),
		ReverseText.Contains(TEXT("00:00, 200/45 AtF  Enter Kanfold\n    Raven sight")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalCaptainTitleTest,
	"SolidCore1.Journal.CaptainTitle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalCaptainTitleTest::RunTest(const FString& Parameters)
{
	SolidEvents::FState State;
	SolidEvents::NotifyOccupiedTown(State, 0);
	SolidEvents::NotifyOccupiedTown(State, 1);
	SolidEvents::NotifyOccupiedTown(State, 2);

	TArray<FString> Lines;
	SolidJournal::CollectLines(State, Lines);
	const FString Text = SolidJournalTestPrivate::JoinLines(Lines);
	TestTrue(TEXT("Relion records Vagrant"), Text.Contains(TEXT("Title Vagrant")));
	TestTrue(TEXT("Kanfold records Ranger"), Text.Contains(TEXT("Title Ranger")));
	TestFalse(TEXT("Iglin does not record a title"), Text.Contains(TEXT("Enter Iglin\n    Title")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidJournalWorldTimeTest,
	"SolidCore1.Journal.WorldTime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidJournalWorldTimeTest::RunTest(const FString& Parameters)
{
	const SolidWorldTime::FStamp Ninth = SolidWorldTime::FromSession(9, 84.0 * 60.0);
	SolidEvents::FState State;
	SolidEvents::NotifyOccupiedTown(State, 0, Ninth);

	TArray<FString> Lines;
	SolidJournal::CollectLines(State, Lines);
	const FString Text = SolidJournalTestPrivate::JoinLines(Lines);
	TestTrue(TEXT("ninth session stamps Iglin"), Text.Contains(TEXT("01:24, 200/53 AtF  Enter Iglin")));

	SolidEvents::RememberStronghold(State, Ninth, SolidStronghold::Starting.Attributes, /*bEnter=*/true);
	const SolidWorldTime::FStamp Later = SolidWorldTime::FromSession(9, 90.0 * 60.0);
	SolidEvents::RememberStronghold(State, Later, SolidStronghold::Starting.Attributes, /*bEnter=*/false);
	SolidEvents::RememberStronghold(State, Later, SolidStronghold::Starting.Attributes, /*bEnter=*/true);

	TArray<FString> AfterCave;
	SolidJournal::CollectLines(State, AfterCave);
	const FString CaveText = SolidJournalTestPrivate::JoinLines(AfterCave);
	TestEqual(TEXT("cave trips are noted again"), State.Notes.Num(), 4);
	const int32 EnterAt = CaveText.Find(TEXT("Enter Meagre Dungeon"));
	const int32 LeaveAt = CaveText.Find(TEXT("Leave Meagre Dungeon"));
	const int32 EnterAgain = CaveText.Find(TEXT("Enter Meagre Dungeon"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
	TestTrue(TEXT("enter is recorded"), EnterAt != INDEX_NONE);
	TestTrue(TEXT("leave is recorded"), LeaveAt != INDEX_NONE && LeaveAt > EnterAt);
	TestTrue(TEXT("a second enter is its own line"), EnterAgain != INDEX_NONE && EnterAgain > LeaveAt);
	TestTrue(TEXT("leave uses the later minute"), CaveText.Contains(TEXT("01:30, 200/53 AtF  Leave Meagre Dungeon")));

	SolidEvents::FState Copied = State;
	TestEqual(TEXT("the cave trip keeps the journal"), Copied.Notes.Num(), State.Notes.Num());
	TestEqual(TEXT("copied leave text"), Copied.Notes[2].Text, FString(TEXT("Leave Meagre Dungeon")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
