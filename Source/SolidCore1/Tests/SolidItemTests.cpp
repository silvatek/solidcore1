#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidItemStackTest,
	"SolidCore1.Inventory.Item",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidItemStackTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Stick = SolidItemType::FindById(SolidItemType::IdStick);
	const FSolidItemType* Sword = SolidItemType::FindById(SolidItemType::IdRustySword);
	const FSolidItemType* Coins = SolidItemType::FindById(SolidItemType::IdCoins);
	TestNotNull(TEXT("the stick is listed"), Stick);
	TestNotNull(TEXT("the sword is listed"), Sword);
	TestNotNull(TEXT("coins are listed"), Coins);
	if (!Stick || !Sword || !Coins)
	{
		return false;
	}

	const FSolidItem Stack = FSolidItem::Make(*Stick, SolidStorage::StartingStickCount, FSolidItem::StartingDraw);
	TestEqual(TEXT("five sticks are one item"), Stack.Count, SolidStorage::StartingStickCount);
	TestFalse(TEXT("a stack has no Power"), Stack.HasPower());
	TestEqual(TEXT("a stack's Power is zero"), Stack.Power(), 0);
	TestFalse(TEXT("a stick is not a weapon"), Stack.IsWeapon());
	TestFalse(TEXT("a stick is not armour"), Stack.IsArmour());

	const FSolidItem Purse = FSolidItem::Make(*Coins, SolidStorage::StartingCoinCount, FSolidItem::StartingDraw);
	TestEqual(TEXT("twenty coins are one item"), Purse.Count, SolidStorage::StartingCoinCount);
	TestFalse(TEXT("coins have no Power"), Purse.HasPower());

	const FSolidItem Low = FSolidItem::Make(*Sword, 4, 0.0);
	const FSolidItem High = FSolidItem::Make(*Sword, 4, 1.0);
	TestEqual(TEXT("a sword stays one"), Low.Count, 1);
	TestEqual(TEXT("another sword stays one"), High.Count, 1);
	TestEqual(TEXT("the low roll is PowerFor"), Low.Power(), SolidItemType::PowerFor(Sword->Quality, 0.0));
	TestEqual(TEXT("the high roll is PowerFor"), High.Power(), SolidItemType::PowerFor(Sword->Quality, 1.0));
	TestTrue(TEXT("two swords can differ"), Low.Power() != High.Power());

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	const int32 Pack = SolidInventory::PackIndex(0, 0);
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 SpearPower = Loadout.Worn[Main].Power();
	SolidInventory::WritePack(Loadout, Pack, High);
	SolidInventory::FSlotHit From;
	From.bHit = true;
	From.bPack = true;
	From.Column = 0;
	From.Row = 0;
	From.Index = Pack;
	SolidInventory::FSlotHit To;
	To.bHit = true;
	To.Slot = SolidInventory::ESlot::MainHand;
	To.Index = Main;
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	TestTrue(TEXT("the high sword swaps onto the spear"), SolidInventory::Move(&Chest, Loadout, From, To));
	TestEqual(TEXT("the hand keeps that sword's Power"), Loadout.Worn[Main].Power(), High.Power());
	TestEqual(TEXT("the pack keeps the spear's Power"), Loadout.Pack[Pack].Power(), SpearPower);

	TestTrue(TEXT("a sword does not join a stack"), SolidInventory::InsertPack(Loadout, SolidItemType::IdRustySword));
	TestEqual(TEXT("the spear's cell is unchanged"), Loadout.Pack[Pack].Power(), SpearPower);
	TestEqual(TEXT("the spear's count stays one"), Loadout.Pack[Pack].Count, 1);
	const int32 Next = SolidInventory::PackIndex(1, 0);
	TestTrue(TEXT("the new sword takes the next cell"), Loadout.Pack[Next].Is(SolidItemType::IdRustySword));
	TestEqual(TEXT("the new sword is one"), Loadout.Pack[Next].Count, 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
