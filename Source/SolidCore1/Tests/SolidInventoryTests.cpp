#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidInventory.h"
#include "Menus/SolidMainMenu.h"
#include "SolidGameState.h"
#include "SolidHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidInventoryTestPrivate
{
	const SolidInventory::FBox* FindWorn(const SolidInventory::FPage& Page, const SolidInventory::ESlot Slot)
	{
		for (const SolidInventory::FBox& Box : Page.Boxes)
		{
			if (!Box.bPack && Box.Slot == Slot)
			{
				return &Box;
			}
		}
		return nullptr;
	}

	const SolidInventory::FBox* FindPack(const SolidInventory::FPage& Page, const int32 Column, const int32 Row)
	{
		for (const SolidInventory::FBox& Box : Page.Boxes)
		{
			if (Box.bPack && Box.Column == Column && Box.Row == Row)
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

	/** The band from the slot name down to the box. Worn names use it; pack cells do not. */
	bool HeadingHitsBox(const SolidInventory::FBox& Heading, const SolidInventory::FBox& Box)
	{
		SolidInventory::FBox Band;
		Band.X = Heading.X;
		Band.Y = Heading.LabelY;
		Band.W = Heading.W;
		Band.H = Heading.Y - Heading.LabelY;
		return Overlaps(Band, Box);
	}

	FString HeldId(const TCHAR* ItemId)
	{
		return FString(ItemId ? ItemId : TEXT(""));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryStartingKitTest,
	"SolidCore1.Inventory.StartingKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryStartingKitTest::RunTest(const FString& Parameters)
{
	const FGameState State;
	const SolidInventory::FLoadout& Kit = State.Inventory;
	TestEqual(
		TEXT("head"),
		SolidInventoryTestPrivate::HeldId(Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Head)]),
		FString(SolidItemType::IdDentedHelmet));
	TestEqual(
		TEXT("body"),
		SolidInventoryTestPrivate::HeldId(Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)]),
		FString(SolidItemType::IdRags));
	TestEqual(
		TEXT("main hand"),
		SolidInventoryTestPrivate::HeldId(Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::MainHand)]),
		FString(SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("boots start empty"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Boots)] == nullptr);
	TestTrue(TEXT("off hand starts empty"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::OffHand)] == nullptr);
	TestTrue(TEXT("belt starts empty"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Belt)] == nullptr);
	TestEqual(TEXT("pack has six cells"), SolidInventory::PackCount, SolidInventory::PackColumns * SolidInventory::PackRows);
	TestEqual(TEXT("pack is two by three"), SolidInventory::PackColumns, 2);
	TestEqual(TEXT("pack is three rows"), SolidInventory::PackRows, 3);
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestTrue(TEXT("pack starts empty"), Kit.Pack[Index] == nullptr);
	}

	SolidInventory::FLoadout Edited = Kit;
	TestTrue(TEXT("spear can leave the main hand"), SolidInventory::PlaceWorn(Edited, SolidInventory::ESlot::MainHand, nullptr));
	TestTrue(
		TEXT("spear can be placed in the pack"),
		SolidInventory::PlacePack(Edited, 1, 2, SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("main hand is empty after the move"), Edited.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::MainHand)] == nullptr);
	TestEqual(
		TEXT("bottom-right pack cell holds the spear"),
		SolidInventoryTestPrivate::HeldId(Edited.Pack[SolidInventory::PackIndex(1, 2)]),
		FString(SolidItemType::IdCrudeSpear));
	TestFalse(TEXT("a cell outside the pack is refused"), SolidInventory::PlacePack(Edited, 2, 0, SolidItemType::IdCrudeSpear));
	TestFalse(TEXT("an unknown item is refused"), SolidInventory::PlaceWorn(Edited, SolidInventory::ESlot::OffHand, TEXT("missing")));
	TestTrue(TEXT("the off hand stays empty"), Edited.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::OffHand)] == nullptr);
	TestTrue(TEXT("the spear stays inside the pack"), Edited.Pack[SolidInventory::PackIndex(0, 0)] == nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryClearTest,
	"SolidCore1.Inventory.Clear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryClearTest::RunTest(const FString& Parameters)
{
	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	const TCHAR* Fill[] = {
		SolidItemType::IdTorch,
		SolidItemType::IdCrudeSpear,
		SolidItemType::IdDentedHelmet,
		SolidItemType::IdRags,
	};
	TestTrue(TEXT("torch can replace rags"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Body, SolidItemType::IdTorch));
	TestTrue(TEXT("boots can hold the spear"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Boots, SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("the off hand can hold the helmet"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::OffHand, SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the belt can hold rags"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Belt, SolidItemType::IdRags));
	for (int32 Row = 0; Row < SolidInventory::PackRows; ++Row)
	{
		for (int32 Column = 0; Column < SolidInventory::PackColumns; ++Column)
		{
			const int32 FillIndex = SolidInventory::PackIndex(Column, Row);
			TestTrue(
				TEXT("a pack cell can hold an item"),
				SolidInventory::PlacePack(Loadout, Column, Row, Fill[FillIndex % UE_ARRAY_COUNT(Fill)]));
		}
	}

	SolidInventory::Clear(Loadout);
	for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
	{
		const SolidInventory::ESlot Slot = static_cast<SolidInventory::ESlot>(Index);
		if (Slot == SolidInventory::ESlot::Body)
		{
			TestEqual(
				TEXT("body wears rags"),
				SolidInventoryTestPrivate::HeldId(Loadout.Worn[Index]),
				FString(SolidItemType::IdRags));
		}
		else
		{
			TestTrue(TEXT("every other worn box is empty"), Loadout.Worn[Index] == nullptr);
		}
	}
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestTrue(TEXT("the pack is empty"), Loadout.Pack[Index] == nullptr);
	}

	SolidInventory::Clear(Loadout);
	TestEqual(
		TEXT("clearing again leaves rags"),
		SolidInventoryTestPrivate::HeldId(Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)]),
		FString(SolidItemType::IdRags));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryLayoutTest,
	"SolidCore1.Inventory.Layout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryLayoutTest::RunTest(const FString& Parameters)
{
	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page);

	const SolidInventory::FBox* Head = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Head);
	const SolidInventory::FBox* Body = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Body);
	const SolidInventory::FBox* Boots = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Boots);
	const SolidInventory::FBox* Off = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::OffHand);
	const SolidInventory::FBox* Main = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::MainHand);
	const SolidInventory::FBox* Belt = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Belt);
	TestTrue(TEXT("head box"), Head != nullptr);
	TestTrue(TEXT("body box"), Body != nullptr);
	TestTrue(TEXT("boots box"), Boots != nullptr);
	TestTrue(TEXT("off hand box"), Off != nullptr);
	TestTrue(TEXT("main hand box"), Main != nullptr);
	TestTrue(TEXT("belt box"), Belt != nullptr);
	if (!Head || !Body || !Boots || !Off || !Main || !Belt)
	{
		return false;
	}

	TestEqual(TEXT("head label"), Head->Label, FString(TEXT("Head")));
	TestEqual(TEXT("body label"), Body->Label, FString(TEXT("Body")));
	TestEqual(TEXT("boots label"), Boots->Label, FString(TEXT("Boots")));
	TestEqual(TEXT("off hand label"), Off->Label, FString(TEXT("Off hand")));
	TestEqual(TEXT("main hand label"), Main->Label, FString(TEXT("Main hand")));
	TestEqual(TEXT("belt label"), Belt->Label, FString(TEXT("Belt")));
	const FSolidItemType* Helmet = SolidItemType::FindById(SolidItemType::IdDentedHelmet);
	const FSolidItemType* Rags = SolidItemType::FindById(SolidItemType::IdRags);
	const FSolidItemType* Spear = SolidItemType::FindById(SolidItemType::IdCrudeSpear);
	TestEqual(TEXT("head wears the helmet"), Head->Item, FString(Helmet ? Helmet->Name : TEXT("")));
	TestEqual(TEXT("body wears rags"), Body->Item, FString(Rags ? Rags->Name : TEXT("")));
	TestTrue(TEXT("boots are empty"), Boots->Item.IsEmpty());
	TestEqual(TEXT("main hand holds the spear"), Main->Item, FString(Spear ? Spear->Name : TEXT("")));
	TestTrue(TEXT("off hand box is empty"), Off->Item.IsEmpty());
	TestTrue(TEXT("belt box is empty"), Belt->Item.IsEmpty());

	TestTrue(TEXT("head is above the body"), Head->Y + Head->H < Body->Y);
	TestTrue(
		TEXT("head is centered on the body"),
		FMath::IsNearlyEqual(Head->X + Head->W * 0.5f, Body->X + Body->W * 0.5f, 1.f));
	TestTrue(TEXT("head is narrower than the body"), Head->W < Body->W);
	TestTrue(TEXT("off hand is left of the body"), Off->X + Off->W < Body->X);
	TestTrue(TEXT("main hand is right of the body"), Main->X > Body->X + Body->W);
	TestTrue(
		TEXT("off hand meets the body in height"),
		Off->Y < Body->Y + Body->H && Off->Y + Off->H > Body->Y);
	TestTrue(
		TEXT("main hand meets the body in height"),
		Main->Y < Body->Y + Body->H && Main->Y + Main->H > Body->Y);
	TestTrue(
		TEXT("hands share a height"),
		FMath::IsNearlyEqual(Off->Y, Main->Y, 1.f) && FMath::IsNearlyEqual(Off->H, Main->H, 1.f));
	TestTrue(TEXT("belt is below the body"), Body->Y + Body->H < Belt->Y);
	TestTrue(TEXT("boots are below the belt"), Belt->Y + Belt->H < Boots->Y);
	TestTrue(TEXT("belt is narrower than the body"), Belt->W < Body->W);
	TestTrue(
		TEXT("belt is centered on the body"),
		FMath::IsNearlyEqual(Belt->X + Belt->W * 0.5f, Body->X + Body->W * 0.5f, 1.f));
	TestTrue(
		TEXT("boots share the body's column"),
		FMath::IsNearlyEqual(Boots->X, Body->X, 1.f) && FMath::IsNearlyEqual(Boots->W, Body->W, 1.f));

	int32 PackBoxes = 0;
	float DollRight = 0.f;
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		if (!Box.bPack)
		{
			DollRight = FMath::Max(DollRight, Box.X + Box.W);
		}
		else
		{
			++PackBoxes;
		}
	}
	TestEqual(TEXT("six pack boxes"), PackBoxes, SolidInventory::PackCount);

	const SolidInventory::FBox* First = SolidInventoryTestPrivate::FindPack(Page, 0, 0);
	TestTrue(TEXT("first pack cell"), First != nullptr);
	if (!First)
	{
		return false;
	}
	TestTrue(TEXT("pack is right of the figure"), First->X > DollRight);
	TestTrue(TEXT("pack heading is above the grid"), Page.PackLabelY < First->Y);
	TestTrue(TEXT("pack heading shares the grid's left edge"), FMath::IsNearlyEqual(Page.PackX, First->X, 1.f));

	const float HeadingGap = First->Y - Page.PackLabelY;
	const SolidInventory::FBox* WornNames[] = { Head, Body, Boots, Off, Main, Belt };
	for (const SolidInventory::FBox* Named : WornNames)
	{
		TestTrue(TEXT("slot name is above its box"), Named->LabelY < Named->Y);
		TestTrue(
			TEXT("slot name uses the pack heading gap"),
			FMath::IsNearlyEqual(Named->Y - Named->LabelY, HeadingGap, 1.f));
		for (const SolidInventory::FBox& Box : Page.Boxes)
		{
			TestFalse(TEXT("a slot name does not cover a box"), SolidInventoryTestPrivate::HeadingHitsBox(*Named, Box));
		}
	}
	TestTrue(TEXT("the body name is below the head"), Body->LabelY >= Head->Y + Head->H);
	TestTrue(TEXT("the belt name is below the body"), Belt->LabelY >= Body->Y + Body->H);
	TestTrue(TEXT("the boots name is below the belt"), Boots->LabelY >= Belt->Y + Belt->H);
	TestTrue(TEXT("the hand names share a line"), FMath::IsNearlyEqual(Off->LabelY, Main->LabelY, 1.f));

	for (int32 Row = 0; Row < SolidInventory::PackRows; ++Row)
	{
		for (int32 Column = 0; Column < SolidInventory::PackColumns; ++Column)
		{
			const SolidInventory::FBox* Cell = SolidInventoryTestPrivate::FindPack(Page, Column, Row);
			TestTrue(TEXT("pack cell"), Cell != nullptr);
			if (!Cell)
			{
				return false;
			}
			TestTrue(TEXT("pack cells match"), FMath::IsNearlyEqual(Cell->W, First->W, 1.f) && FMath::IsNearlyEqual(Cell->H, First->W, 1.f));
			if (Column == 0)
			{
				TestTrue(TEXT("left column"), FMath::IsNearlyEqual(Cell->X, First->X, 1.f));
			}
			else
			{
				const SolidInventory::FBox* Beside = SolidInventoryTestPrivate::FindPack(Page, Column - 1, Row);
				TestTrue(TEXT("next column is to the right"), Beside && Cell->X > Beside->X + Beside->W);
				TestTrue(TEXT("a row shares a height"), Beside && FMath::IsNearlyEqual(Cell->Y, Beside->Y, 1.f));
			}
			if (Row == 0)
			{
				TestTrue(TEXT("top row"), FMath::IsNearlyEqual(Cell->Y, First->Y, 1.f));
			}
			else
			{
				const SolidInventory::FBox* Above = SolidInventoryTestPrivate::FindPack(Page, Column, Row - 1);
				TestTrue(TEXT("next row is below"), Above && Cell->Y > Above->Y + Above->H);
				TestTrue(TEXT("a column stays aligned"), Above && FMath::IsNearlyEqual(Cell->X, Above->X, 1.f));
			}
			TestTrue(TEXT("pack starts empty"), Cell->Item.IsEmpty());
		}
	}

	for (int32 Index = 0; Index < Page.Boxes.Num(); ++Index)
	{
		for (int32 Other = Index + 1; Other < Page.Boxes.Num(); ++Other)
		{
			TestFalse(TEXT("boxes do not overlap"), SolidInventoryTestPrivate::Overlaps(Page.Boxes[Index], Page.Boxes[Other]));
		}
	}

	TestEqual(TEXT("page name"), FString(SolidInventory::Title), FString(TEXT("Inventory")));
	TestEqual(TEXT("pack heading"), FString(SolidInventory::PackTitle), FString(TEXT("Pack")));
	TestEqual(TEXT("close hint"), FString(SolidInventory::CloseHint), FString(TEXT("/ to close")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryToggleTest,
	"SolidCore1.Inventory.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryToggleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("slash opens inventory"), SolidInventory::IsToggleKey(EKeys::Slash, false));
	TestFalse(TEXT("shift slash is not the toggle"), SolidInventory::IsToggleKey(EKeys::Slash, true));
	TestFalse(TEXT("3 stays a menu key"), SolidInventory::IsToggleKey(EKeys::Three, true));
	TestEqual(TEXT("debug label"), FString(SolidInventory::ToggleKeyLabel), FString(TEXT("/")));

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	TestFalse(TEXT("inventory starts closed"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();
	TestTrue(TEXT("/ opens the inventory"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();
	TestFalse(TEXT("/ closes the inventory"), HUD->IsInventoryVisible());

	HUD->ToggleInventory();
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the inventory"), HUD->IsInventoryVisible());
	TestFalse(TEXT("F10 does not open the menu"), HUD->IsMainMenuOpen());

	HUD->ToggleInventory();
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the inventory"), HUD->IsInventoryVisible());

	HUD->HandleMenuKey();
	TestTrue(TEXT("menu is open"), HUD->IsMainMenuOpen());
	HUD->ToggleInventory();
	TestTrue(TEXT("inventory replaces the menu"), HUD->IsInventoryVisible());
	TestFalse(TEXT("menu closed"), HUD->IsMainMenuOpen());
	HUD->ToggleInventory();

	HUD->SetJournalVisible(true);
	HUD->ToggleInventory();
	TestFalse(TEXT("journal closed"), HUD->IsJournalVisible());
	TestTrue(TEXT("inventory opened over the journal"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();

	HUD->OpenCharacterEditor();
	HUD->ToggleInventory();
	TestFalse(TEXT("character editor closed"), HUD->IsCharacterEditorOpen());
	TestTrue(TEXT("inventory opened over the editor"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();

	HUD->ShowMessage(TEXT("00000001"), TEXT("You are killed by monsters"));
	HUD->ToggleInventory();
	TestTrue(TEXT("a message stays up"), HUD->IsMessageVisible());
	TestFalse(TEXT("/ waits until the message closes"), HUD->IsInventoryVisible());
	HUD->CloseMessage();
	HUD->ToggleInventory();
	TestTrue(TEXT("/ opens after the message"), HUD->IsInventoryVisible());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryMenuTest,
	"SolidCore1.Inventory.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryMenuTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	int32 InventoryIndex = INDEX_NONE;
	int32 EditorIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Item == ESolidMainMenuItem::Inventory)
		{
			InventoryIndex = Index;
		}
		if (Entries[Index].Item == ESolidMainMenuItem::CharacterEditor)
		{
			EditorIndex = Index;
		}
	}
	TestTrue(TEXT("inventory is listed"), InventoryIndex != INDEX_NONE);
	TestTrue(TEXT("inventory follows the character editor"), EditorIndex != INDEX_NONE && InventoryIndex == EditorIndex + 1);
	if (InventoryIndex == INDEX_NONE)
	{
		return false;
	}

	FSolidMainMenuEntry Found;
	TestTrue(TEXT("find inventory"), SolidMainMenu::FindEntry(InventoryIndex, Found));
	TestEqual(TEXT("inventory label"), Found.Label, FString(TEXT("Inventory")));
	TestEqual(
		TEXT("inventory action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Found.Item)),
		static_cast<uint8>(ESolidMainMenuAction::ShowInventory));
	TestEqual(TEXT("8 selects inventory"), InventoryIndex, 7);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
