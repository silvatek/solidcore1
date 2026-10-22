#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"

#include <limits>

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidItemTypeTestPrivate
{
	bool IsHexId(const TCHAR* Id)
	{
		const FString Text(Id ? Id : TEXT(""));
		if (Text.Len() != SolidItemType::IdLength)
		{
			return false;
		}
		for (const TCHAR Char : Text)
		{
			if (!FChar::IsHexDigit(Char))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypeCatalogTest,
	"SolidCore1.Inventory.ItemTypes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypeCatalogTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("the catalog lists the known items"), SolidItemType::CatalogCount >= 4);
	TestTrue(TEXT("the catalog fits"), SolidItemType::CatalogCount <= SolidItemType::MaxItems);

	const FSolidItemType* Rags = SolidItemType::FindById(SolidItemType::IdRags);
	const FSolidItemType* Helmet = SolidItemType::FindById(SolidItemType::IdDentedHelmet);
	const FSolidItemType* Spear = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	const FSolidItemType* Torch = SolidItemType::FindById(SolidItemType::IdTorch);
	TestNotNull(TEXT("rags are listed"), Rags);
	TestNotNull(TEXT("the helmet is listed"), Helmet);
	TestNotNull(TEXT("the spear is listed"), Spear);
	TestNotNull(TEXT("the torch is listed"), Torch);
	TestNull(TEXT("an unknown id is empty"), SolidItemType::FindById(TEXT("missing")));
	TestNull(TEXT("an empty id is empty"), SolidItemType::FindById(nullptr));
	TestEqual(TEXT("an unknown name is empty"), FString(SolidItemType::NameFor(TEXT("missing"))), FString(TEXT("")));

	if (!Rags || !Helmet || !Spear || !Torch)
	{
		return false;
	}

	TestEqual(TEXT("rags name"), FString(Rags->Name), FString(TEXT("Rags")));
	TestEqual(TEXT("helmet name"), FString(Helmet->Name), FString(TEXT("Dented helmet")));
	TestEqual(TEXT("spear name"), FString(Spear->Name), FString(TEXT("Crude spear")));
	TestEqual(TEXT("torch name"), FString(Torch->Name), FString(TEXT("Torch")));
	TestEqual(
		TEXT("rags are armor"),
		static_cast<uint8>(Rags->Type),
		static_cast<uint8>(ESolidItemKind::Armor));
	TestEqual(
		TEXT("the spear is a weapon"),
		static_cast<uint8>(Spear->Type),
		static_cast<uint8>(ESolidItemKind::Weapon));
	TestEqual(
		TEXT("the torch is a tool"),
		static_cast<uint8>(Torch->Type),
		static_cast<uint8>(ESolidItemKind::Tool));
	TestEqual(
		TEXT("the torch is common"),
		static_cast<uint8>(Torch->Quality),
		static_cast<uint8>(ESolidItemQuality::Common));
	TestEqual(TEXT("the torch stacks to one"), Torch->MaxStacked, 1);

	const ESolidItemQuality Grades[] = {
		ESolidItemQuality::Inferior,
		ESolidItemQuality::Common,
		ESolidItemQuality::Superior,
		ESolidItemQuality::Exquisite,
		ESolidItemQuality::Legendary,
		ESolidItemQuality::NA,
	};
	TestEqual(TEXT("six grades"), static_cast<int32>(UE_ARRAY_COUNT(Grades)), 6);
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Grades); ++Index)
	{
		for (int32 Other = Index + 1; Other < UE_ARRAY_COUNT(Grades); ++Other)
		{
			TestTrue(TEXT("grades differ"), Grades[Index] != Grades[Other]);
		}
	}
	const TCHAR* InferiorIds[] = {
		SolidItemType::IdRags,
		SolidItemType::IdDentedHelmet,
		SolidItemType::IdCrudeSpear,
		SolidItemType::IdRustySword,
	};
	for (const TCHAR* Id : InferiorIds)
	{
		const FSolidItemType* Item = SolidItemType::FindById(Id);
		TestNotNull(TEXT("an inferior item is listed"), Item);
		if (!Item)
		{
			return false;
		}
		TestEqual(
			TEXT("the item is inferior"),
			static_cast<uint8>(Item->Quality),
			static_cast<uint8>(ESolidItemQuality::Inferior));
		TestEqual(TEXT("the item stacks to one"), Item->MaxStacked, 1);
	}

	const FSolidItemType* Listed[] = { Rags, Helmet, Spear, Torch };
	for (const FSolidItemType* Item : Listed)
	{
		TestTrue(TEXT("item id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Item->Id));
		TestEqual(TEXT("the page name follows the id"), FString(SolidItemType::NameFor(Item->Id)), FString(Item->Name));
	}
	TestTrue(
		TEXT("the four ids differ"),
		FCString::Strcmp(Rags->Id, Helmet->Id) != 0
			&& FCString::Strcmp(Rags->Id, Spear->Id) != 0
			&& FCString::Strcmp(Rags->Id, Torch->Id) != 0
			&& FCString::Strcmp(Helmet->Id, Spear->Id) != 0
			&& FCString::Strcmp(Helmet->Id, Torch->Id) != 0
			&& FCString::Strcmp(Spear->Id, Torch->Id) != 0);

	TestEqual(TEXT("rags are first"), FString(SolidItemType::Catalog[0].Id), FString(SolidItemType::IdRags));
	TestEqual(TEXT("the helmet is second"), FString(SolidItemType::Catalog[1].Id), FString(SolidItemType::IdDentedHelmet));
	TestEqual(TEXT("the spear is third"), FString(SolidItemType::Catalog[2].Id), FString(SolidItemType::IdCrudeSpear));
	TestEqual(TEXT("the torch is fourth"), FString(SolidItemType::Catalog[3].Id), FString(SolidItemType::IdTorch));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypeBootsAndCoinsTest,
	"SolidCore1.Inventory.BootsAndCoins",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypeBootsAndCoinsTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Boots = SolidItemType::FindById(SolidItemType::IdOldBoots);
	const FSolidItemType* Coins = SolidItemType::FindById(SolidItemType::IdCoins);
	TestNotNull(TEXT("old boots are listed"), Boots);
	TestNotNull(TEXT("coins are listed"), Coins);
	if (!Boots || !Coins)
	{
		return false;
	}

	TestEqual(TEXT("old boots name"), FString(Boots->Name), FString(TEXT("Old boots")));
	TestEqual(
		TEXT("old boots are boots"),
		static_cast<uint8>(Boots->Type),
		static_cast<uint8>(ESolidItemKind::Boots));
	TestEqual(
		TEXT("old boots are inferior"),
		static_cast<uint8>(Boots->Quality),
		static_cast<uint8>(ESolidItemQuality::Inferior));
	TestEqual(TEXT("old boots stack to one"), Boots->MaxStacked, 1);

	TestEqual(TEXT("coins name"), FString(Coins->Name), FString(TEXT("Coins")));
	TestEqual(
		TEXT("coins are a material"),
		static_cast<uint8>(Coins->Type),
		static_cast<uint8>(ESolidItemKind::Material));
	TestEqual(
		TEXT("coins have no grade"),
		static_cast<uint8>(Coins->Quality),
		static_cast<uint8>(ESolidItemQuality::NA));
	TestEqual(TEXT("coins stack to 999"), Coins->MaxStacked, 999);
	TestTrue(TEXT("old boots id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Boots->Id));
	TestTrue(TEXT("coins id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Coins->Id));
	TestTrue(TEXT("the new ids differ"), FCString::Strcmp(Boots->Id, Coins->Id) != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypeStickAndPitchTest,
	"SolidCore1.Inventory.StickAndPitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypeStickAndPitchTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Stick = SolidItemType::FindById(SolidItemType::IdStick);
	const FSolidItemType* Pitch = SolidItemType::FindById(SolidItemType::IdPitch);
	TestNotNull(TEXT("a stick is listed"), Stick);
	TestNotNull(TEXT("pitch is listed"), Pitch);
	if (!Stick || !Pitch)
	{
		return false;
	}

	TestEqual(TEXT("stick name"), FString(Stick->Name), FString(TEXT("Stick")));
	TestEqual(
		TEXT("a stick is a material"),
		static_cast<uint8>(Stick->Type),
		static_cast<uint8>(ESolidItemKind::Material));
	TestEqual(
		TEXT("a stick is inferior"),
		static_cast<uint8>(Stick->Quality),
		static_cast<uint8>(ESolidItemQuality::Inferior));
	TestEqual(TEXT("a stick stacks to 10"), Stick->MaxStacked, 10);

	TestEqual(TEXT("pitch name"), FString(Pitch->Name), FString(TEXT("Pitch")));
	TestEqual(
		TEXT("pitch is a material"),
		static_cast<uint8>(Pitch->Type),
		static_cast<uint8>(ESolidItemKind::Material));
	TestEqual(
		TEXT("pitch is common"),
		static_cast<uint8>(Pitch->Quality),
		static_cast<uint8>(ESolidItemQuality::Common));
	TestEqual(TEXT("pitch stacks to 10"), Pitch->MaxStacked, 10);

	TestTrue(TEXT("stick id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Stick->Id));
	TestTrue(TEXT("pitch id is eight hex digits"), SolidItemTypeTestPrivate::IsHexId(Pitch->Id));
	TestTrue(TEXT("the new ids differ"), FCString::Strcmp(Stick->Id, Pitch->Id) != 0);
	TestEqual(TEXT("the page names a stick"), FString(SolidItemType::NameFor(Stick->Id)), FString(TEXT("Stick")));
	TestEqual(TEXT("the page names pitch"), FString(SolidItemType::NameFor(Pitch->Id)), FString(TEXT("Pitch")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemTypePowerTest,
	"SolidCore1.Inventory.Power",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemTypePowerTest::RunTest(const FString& Parameters)
{
	static_assert(SolidItemType::PowerMedian == 12.0, "Inferior median");
	static_assert(SolidItemType::PowerMedianRatio == 1.6, "grade step");
	static_assert(SolidItemType::PowerSpread == 0.28, "log step");
	static_assert(SolidItemType::LegendaryUpside == 0.14, "legendary high side");
	static_assert(SolidItemType::PowerGlide == 0.4, "legendary ease width");

	TestEqual(TEXT("NA has no power"), SolidItemType::PowerFor(ESolidItemQuality::NA, 0.0), 0);
	TestEqual(TEXT("NA has no power above the middle"), SolidItemType::PowerFor(ESolidItemQuality::NA, 2.0), 0);
	TestEqual(
		TEXT("a draw that is not a number has no power"),
		SolidItemType::PowerFor(ESolidItemQuality::Inferior, std::numeric_limits<double>::quiet_NaN()),
		0);

	const ESolidItemQuality Grades[] = {
		ESolidItemQuality::Inferior,
		ESolidItemQuality::Common,
		ESolidItemQuality::Superior,
		ESolidItemQuality::Exquisite,
		ESolidItemQuality::Legendary,
	};
	const int32 Middle[] = { 12, 19, 31, 49, 79 };
	const int32 OneUp[] = { 16, 25, 41, 65, 94 };
	const int32 OneDown[] = { 9, 15, 23, 37, 62 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Grades); ++Index)
	{
		TestEqual(TEXT("the middle of the grade"), SolidItemType::PowerFor(Grades[Index], 0.0), Middle[Index]);
		TestEqual(TEXT("one deviation up"), SolidItemType::PowerFor(Grades[Index], 1.0), OneUp[Index]);
		TestEqual(TEXT("one deviation down"), SolidItemType::PowerFor(Grades[Index], -1.0), OneDown[Index]);
	}

	TestEqual(TEXT("legendary two deviations up"), SolidItemType::PowerFor(ESolidItemQuality::Legendary, 2.0), 108);
	TestTrue(
		TEXT("legendary's high side is shorter than the full spread"),
		SolidItemType::PowerFor(ESolidItemQuality::Legendary, 1.0) < 104
			&& SolidItemType::PowerFor(ESolidItemQuality::Legendary, 2.0) < 138);
	TestEqual(TEXT("a far low roll stays at 1"), SolidItemType::PowerFor(ESolidItemQuality::Inferior, -20.0), 1);
	TestEqual(TEXT("a far high roll fits an int32"), SolidItemType::PowerFor(ESolidItemQuality::Inferior, 80.0), MAX_int32);

	for (const ESolidItemQuality Grade : Grades)
	{
		int32 Previous = 0;
		for (int32 Step = -16; Step <= 16; ++Step)
		{
			const double Z = static_cast<double>(Step) * 0.25;
			const int32 Power = SolidItemType::PowerFor(Grade, Z);
			TestTrue(TEXT("a higher draw is not a lower power"), Power >= Previous);
			Previous = Power;
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidWeaponAndArmourTest,
	"SolidCore1.Inventory.WeaponAndArmour",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidWeaponAndArmourTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Spear = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	const FSolidItemType* Sword = SolidItemType::FindById(SolidItemType::IdRustySword);
	const FSolidItemType* Rags = SolidItemType::FindById(SolidItemType::IdRags);
	const FSolidItemType* Helmet = SolidItemType::FindById(SolidItemType::IdDentedHelmet);
	const FSolidItemType* Boots = SolidItemType::FindById(SolidItemType::IdOldBoots);
	TestNotNull(TEXT("the spear is listed"), Spear);
	TestNotNull(TEXT("the sword is listed"), Sword);
	TestNotNull(TEXT("rags are listed"), Rags);
	TestNotNull(TEXT("the helmet is listed"), Helmet);
	TestNotNull(TEXT("old boots are listed"), Boots);
	if (!Spear || !Sword || !Rags || !Helmet || !Boots)
	{
		return false;
	}

	const FSolidItem SpearItem = FSolidItem::Make(*Spear, 1, FSolidItem::StartingDraw);
	const FSolidItem SwordItem = FSolidItem::Make(*Sword, 1, FSolidItem::StartingDraw);
	const FSolidItem RagsItem = FSolidItem::Make(*Rags, 1, FSolidItem::StartingDraw);
	const FSolidItem HelmetItem = FSolidItem::Make(*Helmet, 1, FSolidItem::StartingDraw);
	const FSolidItem BootsItem = FSolidItem::Make(*Boots, 1, FSolidItem::StartingDraw);
	TestTrue(TEXT("the spear is a weapon"), SpearItem.IsWeapon());
	TestTrue(TEXT("the sword is a weapon"), SwordItem.IsWeapon());
	TestTrue(TEXT("the spear has Speed"), SpearItem.Weapon.Speed > 0.f && FMath::IsFinite(SpearItem.Weapon.Speed));
	TestEqual(TEXT("the spear's Power is its grade"), SpearItem.Power(), SolidItemType::PowerFor(Spear->Quality, FSolidItem::StartingDraw));
	TestTrue(TEXT("the sword has Speed"), SwordItem.Weapon.Speed > 0.f && FMath::IsFinite(SwordItem.Weapon.Speed));
	TestEqual(TEXT("the sword's Power is its grade"), SwordItem.Power(), SolidItemType::PowerFor(Sword->Quality, FSolidItem::StartingDraw));
	TestEqual(TEXT("the spear's weapon is the crude spear"), FString(SpearItem.Weapon.Name), FString(TEXT("Crude spear")));
	TestEqual(TEXT("the sword's weapon is the rusty sword"), FString(SwordItem.Weapon.Name), FString(TEXT("Rusty sword")));
	TestEqual(TEXT("a weapon's count is one"), SpearItem.Count, 1);

	TestTrue(TEXT("rags are armour"), RagsItem.IsArmour());
	TestTrue(TEXT("rags have Strength"), RagsItem.Armour.Strength > 0.f && FMath::IsFinite(RagsItem.Armour.Strength));
	TestEqual(TEXT("rags have no Power"), RagsItem.Power(), 0);
	TestFalse(TEXT("the helmet adds no Strength"), HelmetItem.IsArmour());
	TestFalse(TEXT("boots add no Strength"), BootsItem.IsArmour());

	const ESolidItemKind NoStrength[] = {
		ESolidItemKind::Helmet,
		ESolidItemKind::Boots,
		ESolidItemKind::Belt,
	};
	for (const ESolidItemKind Kind : NoStrength)
	{
		TestTrue(TEXT("a helmet, boots, and a belt are not armour"), Kind != ESolidItemKind::Armor);
	}
	for (int32 Index = 0; Index < SolidItemType::CatalogCount; ++Index)
	{
		const FSolidItemType& Item = SolidItemType::Catalog[Index];
		const FSolidItem Made = FSolidItem::Make(Item, Item.MaxStacked, FSolidItem::StartingDraw);
		if (Item.Type == ESolidItemKind::Weapon)
		{
			TestTrue(TEXT("a weapon has Speed and Power"), Made.IsWeapon() && Made.Power() > 0);
			TestEqual(TEXT("a weapon does not stack"), Made.Count, 1);
		}
		else
		{
			TestFalse(TEXT("anything else is not a weapon"), Made.IsWeapon());
			TestEqual(TEXT("anything else has no Power"), Made.Power(), 0);
		}
		if (Item.Type == ESolidItemKind::Armor)
		{
			TestTrue(TEXT("armour has Strength"), Made.IsArmour() && Made.Armour.Strength > 0.f);
		}
		else
		{
			TestFalse(TEXT("a helmet, boots, a belt, and the rest add no Strength"), Made.IsArmour());
		}
		if (Item.MaxStacked > 1)
		{
			TestEqual(TEXT("a stack keeps its count"), Made.Count, Item.MaxStacked);
		}
	}

	const FString SpearSpeed = FString::SanitizeFloat(SpearItem.Weapon.Speed);
	const FString SpearPower = FString::FromInt(SpearItem.Power());
	const FString RagsStrength = FString::SanitizeFloat(RagsItem.Armour.Strength);
	TestEqual(TEXT("the page names the spear"), FString(SolidItemType::PageLabel(Spear->Id, 1)), FString(Spear->Name));
	TestEqual(TEXT("the page names the sword"), FString(SolidItemType::PageLabel(Sword->Id, 1)), FString(Sword->Name));
	TestEqual(TEXT("the page names rags"), FString(SolidItemType::PageLabel(Rags->Id, 1)), FString(Rags->Name));

	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page);
	bool bSawBody = false;
	bool bSawSpear = false;
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		TestFalse(TEXT("the page hides Speed"), Box.Item.Contains(TEXT("Speed")) || Box.Item.Contains(SpearSpeed));
		TestFalse(TEXT("the page hides Power"), Box.Item.Contains(TEXT("Power")) || Box.Item.Contains(SpearPower));
		TestFalse(TEXT("the page hides Strength"), Box.Item.Contains(TEXT("Strength")) || Box.Item.Contains(RagsStrength));
		if (Box.bPack || Box.bChest || Box.bCraft)
		{
			continue;
		}
		if (Box.Slot == SolidInventory::ESlot::Body)
		{
			bSawBody = true;
			TestEqual(TEXT("the body reads Rags"), Box.Item, FString(TEXT("Rags")));
		}
		if (Box.Slot == SolidInventory::ESlot::MainHand)
		{
			bSawSpear = true;
			TestEqual(TEXT("the main hand reads Crude spear"), Box.Item, FString(TEXT("Crude spear")));
		}
	}
	TestTrue(TEXT("the body box is on the page"), bSawBody);
	TestTrue(TEXT("the main hand is on the page"), bSawSpear);

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	TestTrue(TEXT("the sword can be set down in the pack"), SolidInventory::PlacePack(Loadout, 0, 0, SolidItemType::IdRustySword));
	SolidInventory::FSlotHit FromPack;
	FromPack.bHit = true;
	FromPack.bPack = true;
	FromPack.Column = 0;
	FromPack.Row = 0;
	FromPack.Index = SolidInventory::PackIndex(0, 0);
	SolidInventory::FSlotHit ToHand;
	ToHand.bHit = true;
	ToHand.Slot = SolidInventory::ESlot::MainHand;
	ToHand.Index = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	TestTrue(TEXT("the captain swaps the sword onto the spear"), SolidInventory::Move(&Chest, Loadout, FromPack, ToHand));
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Pack = SolidInventory::PackIndex(0, 0);
	TestEqual(TEXT("the main hand holds the sword"), FString(Loadout.Worn[Main].Id()), FString(SolidItemType::IdRustySword));
	TestEqual(TEXT("the pack holds the spear"), FString(Loadout.Pack[Pack].Id()), FString(SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("the swapped sword keeps its Speed"), Loadout.Worn[Main].Weapon.Speed == SwordItem.Weapon.Speed);
	TestEqual(TEXT("the swapped sword keeps its Power"), Loadout.Worn[Main].Power(), SwordItem.Power());
	TestTrue(TEXT("the spear keeps its Speed in the pack"), Loadout.Pack[Pack].Weapon.Speed == SpearItem.Weapon.Speed);
	TestEqual(TEXT("the spear keeps its Power in the pack"), Loadout.Pack[Pack].Power(), SpearItem.Power());

	SolidInventory::Layout(Loadout, Page, &Chest);
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		if (Box.bPack || Box.bChest || Box.bCraft)
		{
			TestFalse(TEXT("a pack or chest cell hides Speed"), Box.Item.Contains(TEXT("Speed")));
			TestFalse(TEXT("a pack or chest cell hides Power"), Box.Item.Contains(TEXT("Power")));
			TestFalse(TEXT("a pack or chest cell hides Strength"), Box.Item.Contains(TEXT("Strength")));
			continue;
		}
		TestFalse(TEXT("a worn box hides Speed"), Box.Item.Contains(TEXT("Speed")));
		TestFalse(TEXT("a worn box hides Power"), Box.Item.Contains(TEXT("Power")));
		TestFalse(TEXT("a worn box hides Strength"), Box.Item.Contains(TEXT("Strength")));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
