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
	TestEqual(TEXT("the chest has four cells"), SolidStorage::Count, SolidStorage::Columns * SolidStorage::Rows);
	TestEqual(
		TEXT("the first cell holds the sword"),
		FString(Chest.Slots[SolidStorage::Index(0, 0)]),
		FString(SolidItemType::IdRustySword));
	for (int32 Index = 1; Index < SolidStorage::Count; ++Index)
	{
		TestTrue(TEXT("the other chest cells start empty"), Chest.Slots[Index] == nullptr);
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
			SolidInventory::ESlot::RightHand,
			0,
			0));
	TestEqual(
		TEXT("the hand holds the sword"),
		FString(Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::RightHand)]),
		FString(SolidItemType::IdRustySword));
	TestEqual(
		TEXT("the chest holds the spear"),
		FString(Held.Slots[0]),
		FString(SolidItemType::IdCrudeSpear));
	TestFalse(
		TEXT("a cell outside the chest is refused"),
		SolidInventory::TakeFromChest(Held, Loadout, -1, false, SolidInventory::ESlot::LeftHand, 0, 0));
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
	const SolidInventory::FBox* EmptyChest = SolidStorageTestPrivate::FindChest(Page, 1, 1);
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
	TestEqual(TEXT("four chest cells"), ChestBoxes, SolidStorage::Count);
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
	const int32 Right = SolidInventory::WornIndex(SolidInventory::ESlot::RightHand);
	const int32 Left = SolidInventory::WornIndex(SolidInventory::ESlot::LeftHand);
	const int32 Head = SolidInventory::WornIndex(SolidInventory::ESlot::Head);
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);

	TestTrue(
		TEXT("the spear moves into the empty left hand"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::RightHand), Worn(SolidInventory::ESlot::LeftHand)));
	TestEqual(TEXT("the left hand holds the spear"), FString(Loadout.Worn[Left]), FString(SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("the right hand is empty"), Loadout.Worn[Right] == nullptr);

	TestTrue(
		TEXT("the helmet swaps with the spear"),
		SolidInventory::Move(&Chest, Loadout, Worn(SolidInventory::ESlot::Head), Worn(SolidInventory::ESlot::LeftHand)));
	TestEqual(TEXT("the left hand holds the helmet"), FString(Loadout.Worn[Left]), FString(SolidItemType::IdDentedHelmet));
	TestEqual(TEXT("the head holds the spear"), FString(Loadout.Worn[Head]), FString(SolidItemType::IdCrudeSpear));

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
	TestTrue(
		TEXT("the sword goes back into an empty chest cell"),
		SolidInventory::Move(&Chest, Loadout, Pack(0, 1), ChestCell(SolidStorage::Index(1, 0))));
	TestEqual(
		TEXT("the chest cell holds the sword"),
		FString(Chest.Slots[SolidStorage::Index(1, 0)]),
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
	const FString SpearBefore = Loadout.Worn[Head];
	TestFalse(
		TEXT("a chest cell is refused when the chest is closed"),
		SolidInventory::Move(nullptr, Loadout, Worn(SolidInventory::ESlot::Head), ChestCell(SolidStorage::Index(1, 1))));
	TestEqual(TEXT("the spear stays on the head"), FString(Loadout.Worn[Head]), SpearBefore);
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

#endif // WITH_DEV_AUTOMATION_TESTS
