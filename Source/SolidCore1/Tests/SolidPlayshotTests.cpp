#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidPlayshot.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPlayshotFileNameTest,
	"SolidCore1.Playshot.FileName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPlayshotFileNameTest::RunTest(const FString& Parameters)
{
	const FDateTime Time(2026, 10, 27, 12, 47, 1);
	const FString BuildId(TEXT("SC1-0667"));
	const FString FirstName(TEXT("SC1-0667_20261027-124701.png"));
	const FString SecondName(TEXT("SC1-0667_20261027-124701_2.png"));
	const FString ThirdName(TEXT("SC1-0667_20261027-124701_3.png"));

	TArray<FString> Existing;
	TArray<FString> ForBuild;
	TestEqual(
		TEXT("the first name in that second"),
		SolidPlayshot::FileName(BuildId, Time, Existing, ForBuild),
		FirstName);
	TestEqual(TEXT("an empty folder has none for the build"), ForBuild.Num(), 0);

	Existing.Add(TEXT("SC1-0666_20261027-124700.png"));
	Existing.Add(FirstName);
	Existing.Add(TEXT("notes.txt"));
	TestEqual(
		TEXT("the second press in that second"),
		SolidPlayshot::FileName(BuildId, Time, Existing, ForBuild),
		SecondName);
	TestEqual(TEXT("one name belongs to this build"), ForBuild.Num(), 1);
	if (ForBuild.Num() == 1)
	{
		TestEqual(TEXT("that name is the first picture"), ForBuild[0], FirstName);
	}

	Existing.Add(SecondName);
	TestEqual(
		TEXT("the next press"),
		SolidPlayshot::FileName(BuildId, Time, Existing, ForBuild),
		ThirdName);
	TestEqual(TEXT("both pictures belong to this build"), ForBuild.Num(), 2);
	if (ForBuild.Num() == 2)
	{
		TestEqual(TEXT("the first picture stays first"), ForBuild[0], FirstName);
		TestEqual(TEXT("the second picture follows it"), ForBuild[1], SecondName);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
