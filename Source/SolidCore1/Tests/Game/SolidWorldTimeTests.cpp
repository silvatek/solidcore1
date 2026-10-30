#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidWorldTime.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWorldTimeCalendarTest,
	"SolidCore1.WorldTime.Calendar",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWorldTimeCalendarTest::RunTest(const FString& Parameters)
{
	FSolidWorldTime Clock;
	TestEqual(TEXT("first session"), Clock.NextSessionNumber(0), 1);
	TestEqual(TEXT("ninth session"), Clock.NextSessionNumber(8), 9);

	const FSolidWorldTime::FDate First = Clock.DateForSession(1);
	TestEqual(TEXT("first year"), First.Year, 200);
	TestEqual(TEXT("first day"), First.Day, 45);

	const FSolidWorldTime::FDate Ninth = Clock.DateForSession(9);
	TestEqual(TEXT("ninth year"), Ninth.Year, 200);
	TestEqual(TEXT("ninth day"), Ninth.Day, 53);

	const FSolidWorldTime::FDate Rolled = Clock.DateForSession(56);
	TestEqual(TEXT("day 100 rolls the year"), Rolled.Year, 201);
	TestEqual(TEXT("day 100 resets to 0"), Rolled.Day, 0);

	const FSolidWorldTime::FStamp Example = Clock.FromSession(9, 84.0 * 60.0);
	TestEqual(TEXT("example stamp"), Clock.Format(Example), FString(TEXT("01:24, 200/53 AtF")));

	const FSolidWorldTime::FStamp LongSit = Clock.FromSession(1, (26.0 * 60.0 + 10.0) * 60.0);
	TestEqual(TEXT("a long sitting stays on the same day"), LongSit.Day, 45);
	TestEqual(TEXT("hours do not wrap at 24"), Clock.Format(LongSit), FString(TEXT("26:10, 200/45 AtF")));

	const FSolidWorldTime::FStamp Truncated = Clock.FromSession(1, 84.0 * 60.0 + 59.0);
	TestEqual(TEXT("seconds do not round the minute up"), Clock.Format(Truncated), FString(TEXT("01:24, 200/45 AtF")));

	TestEqual(TEXT("before begin stays session 1"), Clock.GetSessionNumber(), 1);
	TestEqual(TEXT("before begin is minute 0"), Clock.Stamp(1000.0).MinuteOfSession, 0);
	Clock.Begin(8, 5000.0);
	TestEqual(TEXT("begin takes the next session"), Clock.GetSessionNumber(), 9);
	const FSolidWorldTime::FStamp Started = Clock.Stamp(5000.0 + 84.0 * 60.0);
	TestEqual(TEXT("stamp follows that session"), Clock.Format(Started), FString(TEXT("01:24, 200/53 AtF")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
