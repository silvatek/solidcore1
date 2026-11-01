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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPlayshotHudTextTest,
	"SolidCore1.Playshot.HudText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPlayshotHudTextTest::RunTest(const FString& Parameters)
{
	TArray<uint8> Png;
	const uint8 Signature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	Png.Append(Signature, UE_ARRAY_COUNT(Signature));
	const uint8 Ihdr[] = {
		0, 0, 0, 13, 'I', 'H', 'D', 'R',
		0, 0, 0, 1, 0, 0, 0, 1, 8, 0, 0, 0, 0,
		0, 0, 0, 0
	};
	Png.Append(Ihdr, UE_ARRAY_COUNT(Ihdr));
	const uint8 Iend[] = { 0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82 };
	Png.Append(Iend, UE_ARRAY_COUNT(Iend));

	const int32 Before = Png.Num();
	TArray<uint8> Prefix;
	Prefix.Append(Png.GetData(), Before - UE_ARRAY_COUNT(Iend));
	const FString Text(TEXT("Build SC1-0866\nPawn  1, 2, 3  (feet Z=4)\nfeet\u0394=+0 cm"));
	TestTrue(TEXT("the lines fit in the PNG"), SolidPlayshot::StampHud(Png, Text));
	TestEqual(TEXT("the picture bytes stay"), Png.Num() > Before, true);
	TestTrue(TEXT("the chunk sits in front of IEND"), Png.Num() >= Prefix.Num() + UE_ARRAY_COUNT(Iend));
	bool bPrefix = Png.Num() >= Prefix.Num();
	for (int32 Index = 0; bPrefix && Index < Prefix.Num(); ++Index)
	{
		bPrefix = Png[Index] == Prefix[Index];
	}
	TestTrue(TEXT("bytes before the chunk stay"), bPrefix);
	TestEqual(TEXT("the lines read back"), SolidPlayshot::ReadHud(Png), Text);

	TArray<uint8> NotPng;
	NotPng.Add(1);
	TestFalse(TEXT("a file that is not a PNG is left alone"), SolidPlayshot::StampHud(NotPng, Text));
	TestEqual(TEXT("that buffer stays one byte"), NotPng.Num(), 1);
	TestTrue(TEXT("an empty read has no HUD text"), SolidPlayshot::ReadHud(NotPng).IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
