#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Events/SolidEvents.h"
#include "Menus/SolidJournal.h"

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
	TestTrue(TEXT("Iglin is not an entry"), !SolidJournalTestPrivate::JoinLines(AtStart).Contains(TEXT("Relion")));
	TestTrue(TEXT("still empty at Iglin"), SolidJournalTestPrivate::JoinLines(AtStart).Contains(TEXT("No events yet.")));

	SolidEvents::NotifyOccupiedTown(State, 1);
	TArray<FString> AfterRelion;
	SolidJournal::CollectLines(State, AfterRelion);
	const FString RelionText = SolidJournalTestPrivate::JoinLines(AfterRelion);
	TestTrue(TEXT("lists Enter Relion"), RelionText.Contains(TEXT("1  Enter Relion")));
	TestTrue(TEXT("Relion enables raven sight"), RelionText.Contains(TEXT("Raven sight enabled")));
	TestTrue(TEXT("Relion party is Sam"), RelionText.Contains(TEXT("Party 1: Sam")));
	TestFalse(TEXT("Alex is not in yet"), RelionText.Contains(TEXT("Alex")));

	SolidEvents::NotifyOccupiedTown(State, 2);
	TArray<FString> AfterKanfold;
	SolidJournal::CollectLines(State, AfterKanfold);
	const FString Both = SolidJournalTestPrivate::JoinLines(AfterKanfold);
	TestTrue(TEXT("Relion stays first"), Both.Contains(TEXT("1  Enter Relion")));
	TestTrue(TEXT("Kanfold is second"), Both.Contains(TEXT("2  Enter Kanfold")));
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
	TestTrue(TEXT("Kanfold is entry 1 when visited first"), ReverseText.Contains(TEXT("1  Enter Kanfold")));
	TestTrue(TEXT("Relion is entry 2 when visited second"), ReverseText.Contains(TEXT("2  Enter Relion")));
	TestTrue(TEXT("visit order is kept"), KanfoldFirst != INDEX_NONE && RelionSecond != INDEX_NONE && KanfoldFirst < RelionSecond);
	TestFalse(TEXT("skipped Relion did not enable sight in that entry"),
		ReverseText.Contains(TEXT("1  Enter Kanfold\n    Raven sight")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
