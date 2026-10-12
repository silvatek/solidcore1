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
	TestEqual(TEXT("first session"), SolidWorldTime::NextSessionNumber(0), 1);
	TestEqual(TEXT("ninth session"), SolidWorldTime::NextSessionNumber(8), 9);

	const SolidWorldTime::FDate First = SolidWorldTime::DateForSession(1);
	TestEqual(TEXT("first year"), First.Year, 200);
	TestEqual(TEXT("first day"), First.Day, 45);

	const SolidWorldTime::FDate Ninth = SolidWorldTime::DateForSession(9);
	TestEqual(TEXT("ninth year"), Ninth.Year, 200);
	TestEqual(TEXT("ninth day"), Ninth.Day, 53);

	const SolidWorldTime::FDate Rolled = SolidWorldTime::DateForSession(56);
	TestEqual(TEXT("day 100 rolls the year"), Rolled.Year, 201);
	TestEqual(TEXT("day 100 resets to 0"), Rolled.Day, 0);

	const SolidWorldTime::FStamp Example = SolidWorldTime::FromSession(9, 84.0 * 60.0);
	TestEqual(TEXT("example stamp"), SolidWorldTime::Format(Example), FString(TEXT("01:24, 200/53 AtF")));

	const SolidWorldTime::FStamp LongSit = SolidWorldTime::FromSession(1, (26.0 * 60.0 + 10.0) * 60.0);
	TestEqual(TEXT("a long sitting stays on the same day"), LongSit.Day, 45);
	TestEqual(TEXT("hours do not wrap at 24"), SolidWorldTime::Format(LongSit), FString(TEXT("26:10, 200/45 AtF")));

	const SolidWorldTime::FStamp Truncated = SolidWorldTime::FromSession(1, 84.0 * 60.0 + 59.0);
	TestEqual(TEXT("seconds do not round the minute up"), SolidWorldTime::Format(Truncated), FString(TEXT("01:24, 200/45 AtF")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
