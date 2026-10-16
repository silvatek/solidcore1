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

#endif // WITH_DEV_AUTOMATION_TESTS
