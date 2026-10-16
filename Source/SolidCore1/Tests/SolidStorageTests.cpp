#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "Inventory/SolidStorage.h"
#include "SolidGameState.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidStorageTestPrivate
{
	const SolidInventory::FBox* FindChest(const SolidInventory::FPage& Page, const int32 Column, const int32 Row)
	{
		for (const SolidInventory::FBox& Box : Page.Boxes)
		{
			if (Box.bChest && Box.Column == Column && Box.Row == Row)
			{
				return &Box;
			}
		}
		return nullptr;
	}

	bool Overlaps(const SolidInventory::FBox& A, const SolidInventory::FBox& B)
	{
		const float Ax1 = A.X + A.W;
		const float Ay1 = A.Y + A.H;
		const float Bx1 = B.X + B.W;
		const float By1 = B.Y + B.H;
		return A.X < Bx1 && B.X < Ax1 && A.Y < By1 && B.Y < Ay1;
	}

	SolidInventory::FSlotHit Worn(const SolidInventory::ESlot Slot)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.Slot = Slot;
		Hit.Index = SolidInventory::WornIndex(Slot);
		return Hit;
	}

	SolidInventory::FSlotHit Pack(const int32 Column, const int32 Row)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bPack = true;
		Hit.Column = Column;
		Hit.Row = Row;
		Hit.Index = SolidInventory::PackIndex(Column, Row);
		return Hit;
	}

	SolidInventory::FSlotHit ChestCell(const int32 Index)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bChest = true;
		Hit.Index = Index;
		return Hit;
	}

	/** The first empty chest cell. Later items can fill the old corner and leave another cell empty. */
	int32 FirstEmptySlot(const SolidStorage::FContainer& Chest)
	{
		for (int32 Index = 0; Index < SolidStorage::Count; ++Index)
		{
			if (Chest.Slots[Index] == nullptr)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStorageCaveChestTest,
	"SolidCore1.Inventory.CaveChest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStorageCaveChestTest::RunTest(const FString& Parameters)
{
	const FSolidItemType* Sword = SolidItemType::FindById(SolidItemType::IdRustySword);
	TestNotNull(TEXT("the rusty sword is listed"), Sword);
	if (!Sword)
	{
		return false;
	}
	TestEqual(TEXT("rusty sword name"), FString(Sword->Name), FString(TEXT("Rusty sword")));
	TestEqual(
		TEXT("the rusty sword is a weapon"),
		static_cast<uint8>(Sword->Type),
		static_cast<uint8>(ESolidItemKind::Weapon));
	TestEqual(TEXT("the page names the sword"), FString(SolidItemType::NameFor(Sword->Id)), FString(Sword->Name));

	const FGameState State;
	const SolidStorage::FContainer& Chest = State.CaveChest;
	TestEqual(TEXT("the chest cell count is the grid"), SolidStorage::Count, SolidStorage::Columns * SolidStorage::Rows);
	TestEqual(
		TEXT("the first cell holds the sword"),
		FString(Chest.Slots[SolidStorage::Index(0, 0)]),
		FString(SolidItemType::IdRustySword));
	const int32 EmptyIndex = SolidStorageTestPrivate::FirstEmptySlot(Chest);
	TestTrue(TEXT("the chest starts with an empty cell"), EmptyIndex != INDEX_NONE);
	if (EmptyIndex == INDEX_NONE)
	{
		return false;
	}

	SolidStorage::FContainer Held = Chest;
	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	TestTrue(
		TEXT("the sword can land in an empty pack cell"),
		SolidInventory::TakeFromChest(
			Held,
			Loadout,
			SolidStorage::Index(0, 0),
			true,
			SolidInventory::ESlot::Head,
			1,
			2));
	TestTrue(TEXT("the chest cell is empty"), Held.Slots[SolidStorage::Index(0, 0)] == nullptr);
	TestEqual(
		TEXT("the pack holds the sword"),
		FString(Loadout.Pack[SolidInventory::PackIndex(1, 2)]),
		FString(SolidItemType::IdRustySword));
	TestFalse(
		TEXT("an empty chest cell cannot be taken"),
		SolidInventory::TakeFromChest(
			Held,
			Loadout,
			SolidStorage::Index(0, 0),
			true,
			SolidInventory::ESlot::Head,
			0,
			0));

	Held = SolidStorage::StartingCaveChest();
	Loadout = SolidInventory::StartingLoadout();
	TestTrue(
		TEXT("the sword can swap with the spear"),
		SolidInventory::TakeFromChest(
			Held,
			Loadout,
			0,
			false,
			SolidInventory::ESlot::MainHand,
			0,
			0));
	TestEqual(
		TEXT("the main hand holds the sword"),
		FString(Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::MainHand)]),
		FString(SolidItemType::IdRustySword));
	TestEqual(
		TEXT("the chest holds the spear"),
		FString(Held.Slots[0]),
		FString(SolidItemType::IdCrudeSpear));
	TestFalse(
		TEXT("a cell outside the chest is refused"),
		SolidInventory::TakeFromChest(Held, Loadout, -1, false, SolidInventory::ESlot::OffHand, 0, 0));
	TestFalse(
		TEXT("a slot that is not worn is refused"),
		SolidInventory::TakeFromChest(Held, Loadout, 0, false, SolidInventory::ESlot::Count, 0, 0));

	SolidInventory::FPage Bare;
	SolidInventory::Layout(Loadout, Bare, nullptr);
	for (const SolidInventory::FBox& Box : Bare.Boxes)
	{
		TestFalse(TEXT("the page without a chest has no chest cells"), Box.bChest);
	}
	TestFalse(TEXT("the chest heading stays off"), Bare.bShowChest);

	SolidInventory::FPage Page;
	const SolidStorage::FContainer Shown = SolidStorage::StartingCaveChest();
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page, &Shown);
	TestTrue(TEXT("the chest heading is on"), Page.bShowChest);
	const SolidInventory::FBox* SwordBox = SolidStorageTestPrivate::FindChest(Page, 0, 0);
	const int32 EmptyColumn = EmptyIndex % SolidStorage::Columns;
	const int32 EmptyRow = EmptyIndex / SolidStorage::Columns;
	const SolidInventory::FBox* EmptyChest = SolidStorageTestPrivate::FindChest(Page, EmptyColumn, EmptyRow);
	TestTrue(TEXT("the sword cell is on the page"), SwordBox != nullptr);
	TestTrue(TEXT("an empty chest cell is on the page"), EmptyChest != nullptr);
	if (!SwordBox || !EmptyChest)
	{
		return false;
	}
	TestEqual(TEXT("the cell shows the sword"), SwordBox->Item, FString(Sword->Name));
	TestTrue(TEXT("the empty chest cell has no item"), EmptyChest->Item.IsEmpty());
	TestTrue(TEXT("the chest sits to the right of the pack"), SwordBox->X > Page.PackX + Page.PackGridW);

	int32 ChestBoxes = 0;
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		if (Box.bChest)
		{
			++ChestBoxes;
			TestTrue(TEXT("a chest cell is not a pack cell"), !Box.bPack);
		}
	}
	TestEqual(TEXT("every chest cell is on the page"), ChestBoxes, SolidStorage::Count);
	for (int32 Index = 0; Index < Page.Boxes.Num(); ++Index)
	{
		for (int32 Other = Index + 1; Other < Page.Boxes.Num(); ++Other)
		{
			TestFalse(
				TEXT("chest cells do not cover the figure or the pack"),
				SolidStorageTestPrivate::Overlaps(Page.Boxes[Index], Page.Boxes[Other]));
		}
	}

	SolidInventory::FSlotHit Hit;
	const float SwordX = SwordBox->X + SwordBox->W * 0.5f;
	const float SwordY = SwordBox->Y + SwordBox->H * 0.5f;
	TestTrue(TEXT("the sword cell accepts the pointer"), SolidInventory::HitSlot(Page, SwordX, SwordY, Hit));
	TestTrue(TEXT("the pointer is on the chest"), Hit.bChest);
	TestEqual(TEXT("the pointer is the first cell"), Hit.Index, SolidStorage::Index(0, 0));
	TestFalse(TEXT("a point outside the page misses"), SolidInventory::HitSlot(Page, -10.f, -10.f, Hit));

	const SolidInventory::FView View = SolidInventory::ContentOrigin(1600.f, 900.f, Page, 16.f);
	const float ScreenX = View.OriginX + SwordX;
	const float ScreenY = View.OriginY + SwordY;
	TestTrue(TEXT("the sword cell is on the board"), ScreenX > 0.f && ScreenY > 0.f && ScreenX < 1600.f && ScreenY < 900.f);
	TestTrue(
		TEXT("a screen point over the sword cell is the chest"),
		SolidInventory::HitSlot(Page, ScreenX - View.OriginX, ScreenY - View.OriginY, Hit) && Hit.bChest && Hit.Index == 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryMoveTest,
	"SolidCore1.Inventory.Move",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryMoveTest::RunTest(const FString& Parameters)
{
	using namespace SolidStorageTestPrivate;

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Off = SolidInventory::WornIndex(SolidInventory::ESlot::OffHand);
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);

	TestTrue(
		TEXT("the spear moves into the empty off hand"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::MainHand), Worn(SolidInventory::ESlot::OffHand)));
	TestEqual(TEXT("the off hand holds the spear"), FString(Loadout.Worn[Off]), FString(SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("the main hand is empty"), Loadout.Worn[Main] == nullptr);

	TestTrue(TEXT("the torch can be held"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::MainHand, SolidItemType::IdTorch));
	TestTrue(
		TEXT("the torch swaps with the spear"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::MainHand), Worn(SolidInventory::ESlot::OffHand)));
	TestEqual(TEXT("the off hand holds the torch"), FString(Loadout.Worn[Off]), FString(SolidItemType::IdTorch));
	TestEqual(TEXT("the main hand holds the spear"), FString(Loadout.Worn[Main]), FString(SolidItemType::IdCrudeSpear));

	TestFalse(
		TEXT("dropping on the same worn box does nothing"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::Body), Worn(SolidInventory::ESlot::Body)));
	TestEqual(TEXT("the body still wears Rags"), FString(Loadout.Worn[Body]), FString(SolidItemType::IdRags));

	TestTrue(
		TEXT("Rags move into an empty pack cell"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::Body), Pack(0, 0)));
	TestEqual(
		TEXT("the pack holds Rags"),
		FString(Loadout.Pack[SolidInventory::PackIndex(0, 0)]),
		FString(SolidItemType::IdRags));
	TestTrue(TEXT("the body is empty"), Loadout.Worn[Body] == nullptr);

	TestTrue(
		TEXT("a pack cell moves to another pack cell"),
		SolidInventory::Move(&Chest, Loadout, Pack(0, 0), Pack(1, 2)));
	TestEqual(
		TEXT("the far pack cell holds Rags"),
		FString(Loadout.Pack[SolidInventory::PackIndex(1, 2)]),
		FString(SolidItemType::IdRags));
	TestTrue(TEXT("the first pack cell is empty"), Loadout.Pack[SolidInventory::PackIndex(0, 0)] == nullptr);

	TestTrue(TEXT("the sword can be set down in the pack"), SolidInventory::PlacePack(Loadout, 0, 1, SolidItemType::IdRustySword));
	const int32 EmptyChest = SolidStorageTestPrivate::FirstEmptySlot(Chest);
	TestTrue(TEXT("the chest has an empty cell for the sword"), EmptyChest != INDEX_NONE);
	if (EmptyChest == INDEX_NONE)
	{
		return false;
	}
	TestTrue(
		TEXT("the sword goes back into an empty chest cell"),
		SolidInventory::Move(&Chest, Loadout, Pack(0, 1), ChestCell(EmptyChest)));
	TestEqual(
		TEXT("the chest cell holds the sword"),
		FString(Chest.Slots[EmptyChest]),
		FString(SolidItemType::IdRustySword));
	TestTrue(TEXT("the pack cell is empty after the return"), Loadout.Pack[SolidInventory::PackIndex(0, 1)] == nullptr);
	TestEqual(
		TEXT("the chest still holds its first sword"),
		FString(Chest.Slots[SolidStorage::Index(0, 0)]),
		FString(SolidItemType::IdRustySword));

	TestTrue(
		TEXT("Rags swap onto the occupied chest cell"),
		SolidInventory::Move(&Chest, Loadout, Pack(1, 2), ChestCell(0)));
	TestEqual(TEXT("the chest holds Rags"), FString(Chest.Slots[0]), FString(SolidItemType::IdRags));
	TestEqual(
		TEXT("the pack holds the sword that was in the chest"),
		FString(Loadout.Pack[SolidInventory::PackIndex(1, 2)]),
		FString(SolidItemType::IdRustySword));

	TestFalse(
		TEXT("an empty worn box cannot be dragged"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::Boots), Pack(0, 0)));
	TestTrue(TEXT("the empty drop leaves the pack cell empty"), Loadout.Pack[SolidInventory::PackIndex(0, 0)] == nullptr);

	SolidStorage::FContainer Untouched = Chest;
	const FString SpearBefore = Loadout.Worn[Main];
	TestFalse(
		TEXT("a chest cell is refused when the chest is closed"),
		SolidInventory::Move(nullptr, Loadout, Worn(SolidInventory::ESlot::MainHand), ChestCell(SolidStorage::Index(1, 1))));
	TestEqual(TEXT("the spear stays in the main hand"), FString(Loadout.Worn[Main]), SpearBefore);
	for (int32 Index = 0; Index < SolidStorage::Count; ++Index)
	{
		TestEqual(
			TEXT("a closed chest is not written"),
			FString(Chest.Slots[Index] ? Chest.Slots[Index] : TEXT("")),
			FString(Untouched.Slots[Index] ? Untouched.Slots[Index] : TEXT("")));
	}
	TestFalse(
		TEXT("a drag out of a closed chest is refused"),
		SolidInventory::Move(nullptr, Loadout, ChestCell(0), Worn(SolidInventory::ESlot::Boots)));
	TestTrue(TEXT("the boots stay empty"), Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Boots)] == nullptr);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStorageChestStacksTest,
	"SolidCore1.Inventory.ChestStacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStorageChestStacksTest::RunTest(const FString& Parameters)
{
	using namespace SolidStorageTestPrivate;

	const SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	const int32 BootsIndex = SolidStorage::Index(1, 0);
	const int32 CoinIndex = SolidStorage::Index(0, 1);
	TestEqual(
		TEXT("old boots sit beside the sword"),
		FString(Chest.Slots[BootsIndex]),
		FString(SolidItemType::IdOldBoots));
	TestEqual(TEXT("one pair of old boots"), Chest.Counts[BootsIndex], 1);
	TestEqual(
		TEXT("coins sit under the sword"),
		FString(Chest.Slots[CoinIndex]),
		FString(SolidItemType::IdCoins));
	TestEqual(TEXT("twenty coins"), Chest.Counts[CoinIndex], SolidStorage::StartingCoinCount);

	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page, &Chest);
	const SolidInventory::FBox* BootsBox = FindChest(Page, 1, 0);
	const SolidInventory::FBox* CoinBox = FindChest(Page, 0, 1);
	TestTrue(TEXT("the boots cell is on the page"), BootsBox != nullptr);
	TestTrue(TEXT("the coin cell is on the page"), CoinBox != nullptr);
	if (!BootsBox || !CoinBox)
	{
		return false;
	}
	TestEqual(TEXT("the page names the boots"), BootsBox->Item, FString(TEXT("Old boots")));
	TestEqual(
		TEXT("the page shows the coin stack"),
		CoinBox->Item,
		FString::Printf(TEXT("Coins %d"), SolidStorage::StartingCoinCount));

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Held = Chest;
	TestTrue(
		TEXT("old boots can be worn"),
		SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Boots, SolidItemType::IdOldBoots));
	TestFalse(
		TEXT("old boots cannot replace rags"),
		SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Body, SolidItemType::IdOldBoots));
	TestEqual(
		TEXT("the body still wears rags"),
		FString(Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)]),
		FString(SolidItemType::IdRags));

	TestTrue(
		TEXT("the coin stack can move into the pack"),
		SolidInventory::Move(&Held, Loadout, ChestCell(CoinIndex), Pack(0, 0)));
	TestEqual(
		TEXT("the pack holds the coins"),
		FString(Loadout.Pack[SolidInventory::PackIndex(0, 0)]),
		FString(SolidItemType::IdCoins));
	TestEqual(
		TEXT("the pack keeps the stack"),
		Loadout.PackStack[SolidInventory::PackIndex(0, 0)],
		SolidStorage::StartingCoinCount);
	TestTrue(TEXT("the chest cell is empty"), Held.Slots[CoinIndex] == nullptr);
	TestEqual(TEXT("the empty cell has no count"), Held.Counts[CoinIndex], 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStorageChestMaterialsTest,
	"SolidCore1.Inventory.ChestMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStorageChestMaterialsTest::RunTest(const FString& Parameters)
{
	using namespace SolidStorageTestPrivate;

	TestEqual(TEXT("the chest is two columns"), SolidStorage::Columns, 2);
	TestEqual(TEXT("the chest is three rows"), SolidStorage::Rows, 3);
	TestEqual(TEXT("five sticks"), SolidStorage::StartingStickCount, 5);
	TestEqual(TEXT("three pitch"), SolidStorage::StartingPitchCount, 3);

	const SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	const int32 StickIndex = SolidStorage::Index(1, 1);
	const int32 PitchIndex = SolidStorage::Index(0, 2);
	const int32 LastIndex = SolidStorage::Index(1, 2);
	TestEqual(
		TEXT("sticks sit beside the coins"),
		FString(Chest.Slots[StickIndex]),
		FString(SolidItemType::IdStick));
	TestEqual(TEXT("the chest holds five sticks"), Chest.Counts[StickIndex], SolidStorage::StartingStickCount);
	TestEqual(
		TEXT("pitch sits under the coins"),
		FString(Chest.Slots[PitchIndex]),
		FString(SolidItemType::IdPitch));
	TestEqual(TEXT("the chest holds three pitch"), Chest.Counts[PitchIndex], SolidStorage::StartingPitchCount);
	TestTrue(TEXT("the last cell starts empty"), Chest.Slots[LastIndex] == nullptr);
	TestEqual(TEXT("the last cell has no count"), Chest.Counts[LastIndex], 0);

	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page, &Chest);
	const SolidInventory::FBox* StickBox = FindChest(Page, 1, 1);
	const SolidInventory::FBox* PitchBox = FindChest(Page, 0, 2);
	TestTrue(TEXT("the stick cell is on the page"), StickBox != nullptr);
	TestTrue(TEXT("the pitch cell is on the page"), PitchBox != nullptr);
	if (!StickBox || !PitchBox)
	{
		return false;
	}
	TestEqual(
		TEXT("the page shows the stick stack"),
		StickBox->Item,
		FString::Printf(TEXT("Stick %d"), SolidStorage::StartingStickCount));
	TestEqual(
		TEXT("the page shows the pitch stack"),
		PitchBox->Item,
		FString::Printf(TEXT("Pitch %d"), SolidStorage::StartingPitchCount));

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Held = Chest;
	TestTrue(
		TEXT("the stick stack can move into the pack"),
		SolidInventory::Move(&Held, Loadout, ChestCell(StickIndex), Pack(0, 0)));
	TestEqual(
		TEXT("the pack holds the sticks"),
		FString(Loadout.Pack[SolidInventory::PackIndex(0, 0)]),
		FString(SolidItemType::IdStick));
	TestEqual(
		TEXT("the pack keeps the stick stack"),
		Loadout.PackStack[SolidInventory::PackIndex(0, 0)],
		SolidStorage::StartingStickCount);
	TestTrue(TEXT("the stick cell is empty"), Held.Slots[StickIndex] == nullptr);
	TestEqual(
		TEXT("pitch stays in the chest"),
		FString(Held.Slots[PitchIndex]),
		FString(SolidItemType::IdPitch));
	TestEqual(TEXT("pitch keeps its stack"), Held.Counts[PitchIndex], SolidStorage::StartingPitchCount);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
