#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidItem.h"
#include "Inventory/SolidItemType.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidRaptor.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterWeaponTest,
	"SolidCore1.Monster.Weapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterWeaponTest::RunTest(const FString& Parameters)
{
	const ESolidMonsterSize Sizes[] = {
		ESolidMonsterSize::Tiny,
		ESolidMonsterSize::Small,
		ESolidMonsterSize::Medium,
		ESolidMonsterSize::Large,
	};
	const ESolidItemQuality Grades[] = {
		ESolidItemQuality::Inferior,
		ESolidItemQuality::Common,
		ESolidItemQuality::Superior,
		ESolidItemQuality::Exquisite,
	};
	const double Draws[] = { -1.0, 0.0, 1.0 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Sizes); ++Index)
	{
		for (const double Draw : Draws)
		{
			TestEqual(
				TEXT("that size uses the item roll"),
				SolidMonster::PowerFor(Sizes[Index], Draw),
				SolidItemType::PowerFor(Grades[Index], Draw));
		}
	}

	TestEqual(
		TEXT("Huge matches Legendary at the middle"),
		SolidMonster::PowerFor(ESolidMonsterSize::Huge, 0.0),
		SolidItemType::PowerFor(ESolidItemQuality::Legendary, 0.0));
	TestTrue(
		TEXT("Huge keeps the wide high side"),
		SolidMonster::PowerFor(ESolidMonsterSize::Huge, 1.0)
			> SolidItemType::PowerFor(ESolidItemQuality::Legendary, 1.0));

	const int32 ImmenseMiddle = SolidMonster::PowerFor(ESolidMonsterSize::Immense, 0.0);
	const int32 ImmenseUp = SolidMonster::PowerFor(ESolidMonsterSize::Immense, 1.0);
	const int32 HugeMiddle = SolidMonster::PowerFor(ESolidMonsterSize::Huge, 0.0);
	const int32 HugeUp = SolidMonster::PowerFor(ESolidMonsterSize::Huge, 1.0);
	const int32 LegendaryMiddle = SolidItemType::PowerFor(ESolidItemQuality::Legendary, 0.0);
	const int32 LegendaryUp = SolidItemType::PowerFor(ESolidItemQuality::Legendary, 1.0);
	TestTrue(TEXT("Immense sits above Huge"), ImmenseMiddle > HugeMiddle);
	TestTrue(TEXT("Immense rises above its middle"), ImmenseUp > ImmenseMiddle);
	const double ImmenseRise = static_cast<double>(ImmenseUp) / static_cast<double>(ImmenseMiddle);
	const double LegendaryRise = static_cast<double>(LegendaryUp) / static_cast<double>(LegendaryMiddle);
	const double HugeRise = static_cast<double>(HugeUp) / static_cast<double>(HugeMiddle);
	TestTrue(
		TEXT("Immense narrows the high side"),
		FMath::IsNearlyEqual(ImmenseRise, LegendaryRise, 0.02) && ImmenseRise < HugeRise);

	TestEqual(
		TEXT("an unknown size has no power"),
		SolidMonster::PowerFor(static_cast<ESolidMonsterSize>(255), 0.0),
		0);
	TestEqual(
		TEXT("a draw that is not a number has no power"),
		SolidMonster::PowerFor(ESolidMonsterSize::Small, std::numeric_limits<double>::quiet_NaN()),
		0);

	FSolidRaptor Low;
	Low.Size = ESolidMonsterSize::Small;
	Low.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, 0.0);
	FSolidRaptor High;
	High.Size = ESolidMonsterSize::Small;
	High.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, 1.0);
	TestEqual(
		TEXT("the low claws are the Small roll"),
		Low.Weapon.Power,
		SolidMonster::PowerFor(ESolidMonsterSize::Small, 0.0));
	TestEqual(
		TEXT("the high claws are the Small roll"),
		High.Weapon.Power,
		SolidMonster::PowerFor(ESolidMonsterSize::Small, 1.0));
	TestTrue(TEXT("two Small raptors can differ"), Low.Weapon.Power != High.Weapon.Power);
	TestEqual(TEXT("the weapon is claws"), FString(Low.Weapon.Name), FString(TEXT("Claws")));
	TestEqual(TEXT("the armour is hide"), FString(Low.Armour.Name), FString(TEXT("Hide")));
	TestTrue(TEXT("claws have Speed"), Low.Weapon.Speed > 0.f && FMath::IsFinite(Low.Weapon.Speed));
	TestTrue(TEXT("hide has Strength"), Low.Armour.Strength > 0.f && FMath::IsFinite(Low.Armour.Strength));

	FSolidRaptor Teeth;
	Teeth.Size = ESolidMonsterSize::Small;
	Teeth.Arm(TEXT("Teeth"), TEXT("Hide"), FSolidItem::StartingDraw);
	TestEqual(TEXT("teeth use the same size roll"), Teeth.Weapon.Power, Low.Weapon.Power);
	TestTrue(
		TEXT("teeth and claws differ by name"),
		FCString::Strcmp(Teeth.Weapon.Name, Low.Weapon.Name) != 0);

	TUniquePtr<FSolidMonster> Copy = High.Clone();
	FSolidMonster* Copied = Copy.Get();
	TestNotNull(TEXT("the copy"), Copied);
	if (Copied)
	{
		TestEqual(TEXT("the copy keeps the high claws"), Copied->Weapon.Power, High.Weapon.Power);
		TestEqual(TEXT("the copy keeps the claws"), FString(Copied->Weapon.Name), FString(TEXT("Claws")));
		TestEqual(TEXT("the copy keeps the hide"), FString(Copied->Armour.Name), FString(TEXT("Hide")));
		Copied->Weapon.Power = Low.Weapon.Power;
		TestEqual(
			TEXT("the original keeps its power"),
			High.Weapon.Power,
			SolidMonster::PowerFor(ESolidMonsterSize::Small, 1.0));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
