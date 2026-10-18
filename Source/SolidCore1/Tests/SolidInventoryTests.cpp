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

	SolidInventory::FSlotHit WornHit(const SolidInventory::ESlot Slot)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.Slot = Slot;
		Hit.Index = SolidInventory::WornIndex(Slot);
		return Hit;
	}

	SolidInventory::FSlotHit PackHit(const int32 Column, const int32 Row)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bPack = true;
		Hit.Column = Column;
		Hit.Row = Row;
		Hit.Index = SolidInventory::PackIndex(Column, Row);
		return Hit;
	}

	SolidInventory::FSlotHit ChestHit(const int32 Index)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bChest = true;
		Hit.Index = Index;
		return Hit;
	}

	SolidInventory::FSlotHit CraftHit(const int32 Index)
	{
		SolidInventory::FSlotHit Hit;
		Hit.bHit = true;
		Hit.bCraft = true;
		Hit.Index = Index;
		Hit.Column = Index;
		return Hit;
	}

	const SolidInventory::FBox* FindCraft(const SolidInventory::FPage& Page, const int32 Index)
	{
		for (const SolidInventory::FBox& Box : Page.Boxes)
		{
			if (Box.bCraft && Box.Column == Index)
			{
				return &Box;
			}
		}
		return nullptr;
	}

	/** The first empty chest cell. Later items can fill the old corner and leave another cell empty. */
	int32 EmptyChestIndex(const SolidStorage::FContainer& Chest)
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
	TestTrue(TEXT("the off hand can hold the torch"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::OffHand, SolidItemType::IdTorch));
	TestTrue(TEXT("the belt can hold the spear"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Belt, SolidItemType::IdCrudeSpear));
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

	float DollRight = 0.f;
	const SolidInventory::FBox* Figure[] = { Head, Body, Boots, Off, Main, Belt };
	for (const SolidInventory::FBox* Box : Figure)
	{
		DollRight = FMath::Max(DollRight, Box->X + Box->W);
	}
	int32 PackBoxes = 0;
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		if (Box.bPack)
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
	TestTrue(TEXT("view opens inventory"), SolidInventory::IsToggleKey(EKeys::Gamepad_Special_Left, false));
	TestTrue(TEXT("view stays the toggle with shift"), SolidInventory::IsToggleKey(EKeys::Gamepad_Special_Left, true));
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
	int32 EditorIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Item == ESolidMainMenuItem::CharacterEditor)
		{
			EditorIndex = Index;
			break;
		}
	}
	TestTrue(TEXT("character editor is listed"), EditorIndex != INDEX_NONE);
	if (EditorIndex == INDEX_NONE)
	{
		return false;
	}

	FSolidMainMenuEntry Found;
	TestTrue(TEXT("find the editor"), SolidMainMenu::FindEntry(EditorIndex, Found));
	TestEqual(TEXT("editor label"), Found.Label, FString(TEXT("Character editor")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventorySlotFitTest,
	"SolidCore1.Inventory.SlotFit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventorySlotFitTest::RunTest(const FString& Parameters)
{
	using namespace SolidInventoryTestPrivate;

	const FSolidItemType* Helmet = SolidItemType::FindById(SolidItemType::IdDentedHelmet);
	TestNotNull(TEXT("the helmet is listed"), Helmet);
	if (!Helmet)
	{
		return false;
	}
	TestEqual(
		TEXT("the helmet is a helmet"),
		static_cast<uint8>(Helmet->Type),
		static_cast<uint8>(ESolidItemKind::Helmet));

	const SolidInventory::ESlot WornSlots[] = {
		SolidInventory::ESlot::Head,
		SolidInventory::ESlot::Body,
		SolidInventory::ESlot::Boots,
		SolidInventory::ESlot::OffHand,
		SolidInventory::ESlot::MainHand,
		SolidInventory::ESlot::Belt,
	};
	for (const SolidInventory::ESlot Slot : WornSlots)
	{
		TestEqual(
			TEXT("armor fits only the body"),
			SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Armor),
			Slot == SolidInventory::ESlot::Body);
		TestEqual(
			TEXT("a helmet fits only the head"),
			SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Helmet),
			Slot == SolidInventory::ESlot::Head);
		TestEqual(
			TEXT("boots fit only the boots"),
			SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Boots),
			Slot == SolidInventory::ESlot::Boots);
		TestEqual(
			TEXT("a belt fits only the belt"),
			SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Belt),
			Slot == SolidInventory::ESlot::Belt);
		TestTrue(TEXT("a weapon fits any worn box"), SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Weapon));
		TestTrue(TEXT("a tool fits any worn box"), SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Tool));
		TestTrue(TEXT("food fits any worn box"), SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Food));
		TestTrue(TEXT("a material fits any worn box"), SolidInventory::WornSlotAccepts(Slot, ESolidItemKind::Material));
	}

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	const int32 Head = SolidInventory::WornIndex(SolidInventory::ESlot::Head);
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	const int32 Off = SolidInventory::WornIndex(SolidInventory::ESlot::OffHand);
	const int32 Boots = SolidInventory::WornIndex(SolidInventory::ESlot::Boots);
	const int32 Belt = SolidInventory::WornIndex(SolidInventory::ESlot::Belt);

	TestFalse(TEXT("rags cannot replace the helmet"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Head, SolidItemType::IdRags));
	TestEqual(TEXT("the head still wears the helmet"), HeldId(Loadout.Worn[Head]), FString(SolidItemType::IdDentedHelmet));
	TestFalse(TEXT("rags cannot be held"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::OffHand, SolidItemType::IdRags));
	TestTrue(TEXT("the off hand stays empty"), Loadout.Worn[Off] == nullptr);
	TestFalse(TEXT("rags cannot replace the spear"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::MainHand, SolidItemType::IdRags));
	TestEqual(TEXT("the main hand still holds the spear"), HeldId(Loadout.Worn[Main]), FString(SolidItemType::IdCrudeSpear));
	TestFalse(TEXT("rags cannot be worn as boots"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Boots, SolidItemType::IdRags));
	TestTrue(TEXT("the boots stay empty"), Loadout.Worn[Boots] == nullptr);
	TestFalse(TEXT("rags cannot be worn as a belt"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Belt, SolidItemType::IdRags));
	TestTrue(TEXT("the belt stays empty"), Loadout.Worn[Belt] == nullptr);
	TestTrue(TEXT("rags can be worn on the body"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Body, SolidItemType::IdRags));

	TestFalse(TEXT("the helmet cannot replace rags"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Body, SolidItemType::IdDentedHelmet));
	TestEqual(TEXT("the body still wears rags"), HeldId(Loadout.Worn[Body]), FString(SolidItemType::IdRags));
	TestFalse(TEXT("the helmet cannot be held in the off hand"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::OffHand, SolidItemType::IdDentedHelmet));
	TestFalse(TEXT("the helmet cannot be held in the main hand"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::MainHand, SolidItemType::IdDentedHelmet));
	TestFalse(TEXT("the helmet cannot be worn as boots"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Boots, SolidItemType::IdDentedHelmet));
	TestFalse(TEXT("the helmet cannot be worn as a belt"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Belt, SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the helmet can be worn on the head"), SolidInventory::PlaceWorn(Loadout, SolidInventory::ESlot::Head, SolidItemType::IdDentedHelmet));

	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	TestFalse(
		TEXT("the spear cannot swap rags onto the main hand"),
		SolidInventory::Move(&Chest, Loadout, WornHit(SolidInventory::ESlot::MainHand), WornHit(SolidInventory::ESlot::Body)));
	TestEqual(TEXT("the main hand keeps the spear"), HeldId(Loadout.Worn[Main]), FString(SolidItemType::IdCrudeSpear));
	TestEqual(TEXT("the body keeps rags"), HeldId(Loadout.Worn[Body]), FString(SolidItemType::IdRags));
	TestFalse(
		TEXT("the helmet cannot be dragged onto the off hand"),
		SolidInventory::Move(&Chest, Loadout, WornHit(SolidInventory::ESlot::Head), WornHit(SolidInventory::ESlot::OffHand)));
	TestEqual(TEXT("the head keeps the helmet"), HeldId(Loadout.Worn[Head]), FString(SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the off hand is still empty"), Loadout.Worn[Off] == nullptr);

	const int32 ChestIndex = SolidInventoryTestPrivate::EmptyChestIndex(Chest);
	TestTrue(TEXT("the chest has an empty cell"), ChestIndex != INDEX_NONE);
	if (ChestIndex == INDEX_NONE)
	{
		return false;
	}
	TestTrue(
		TEXT("the helmet can be dragged into the pack"),
		SolidInventory::Move(&Chest, Loadout, WornHit(SolidInventory::ESlot::Head), PackHit(0, 0)));
	TestEqual(
		TEXT("the pack holds the helmet"),
		HeldId(Loadout.Pack[SolidInventory::PackIndex(0, 0)]),
		FString(SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the head is empty"), Loadout.Worn[Head] == nullptr);
	TestTrue(
		TEXT("the helmet can be dragged into the chest"),
		SolidInventory::Move(&Chest, Loadout, PackHit(0, 0), ChestHit(ChestIndex)));
	TestEqual(TEXT("the chest holds the helmet"), HeldId(Chest.Slots[ChestIndex]), FString(SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the pack cell is empty"), Loadout.Pack[SolidInventory::PackIndex(0, 0)] == nullptr);
	TestTrue(
		TEXT("the helmet can be dragged back onto the head"),
		SolidInventory::Move(&Chest, Loadout, ChestHit(ChestIndex), WornHit(SolidInventory::ESlot::Head)));
	TestEqual(TEXT("the head wears the helmet again"), HeldId(Loadout.Worn[Head]), FString(SolidItemType::IdDentedHelmet));
	TestTrue(TEXT("the chest cell is empty"), Chest.Slots[ChestIndex] == nullptr);

	TestTrue(
		TEXT("rags can be dragged into the pack"),
		SolidInventory::Move(&Chest, Loadout, WornHit(SolidInventory::ESlot::Body), PackHit(1, 0)));
	TestEqual(
		TEXT("the pack holds rags"),
		HeldId(Loadout.Pack[SolidInventory::PackIndex(1, 0)]),
		FString(SolidItemType::IdRags));
	TestTrue(TEXT("the body is empty"), Loadout.Worn[Body] == nullptr);
	TestTrue(
		TEXT("rags can be dragged back onto the body"),
		SolidInventory::Move(&Chest, Loadout, PackHit(1, 0), WornHit(SolidInventory::ESlot::Body)));
	TestEqual(TEXT("the body wears rags again"), HeldId(Loadout.Worn[Body]), FString(SolidItemType::IdRags));

	for (int32 Index = 0; Index < SolidItemType::CatalogCount; ++Index)
	{
		const FSolidItemType& Item = SolidItemType::Catalog[Index];
		TestTrue(
			TEXT("any item can be placed in the pack"),
			SolidInventory::PlacePack(Loadout, 0, 0, Item.Id));
		TestEqual(TEXT("the pack cell stores that item"), HeldId(Loadout.Pack[SolidInventory::PackIndex(0, 0)]), FString(Item.Id));
		TestTrue(
			TEXT("any item can be dragged into the chest"),
			SolidInventory::Move(&Chest, Loadout, PackHit(0, 0), ChestHit(ChestIndex)));
		TestEqual(TEXT("the chest cell stores that item"), HeldId(Chest.Slots[ChestIndex]), FString(Item.Id));
		TestTrue(
			TEXT("the item can leave the chest"),
			SolidInventory::Move(&Chest, Loadout, ChestHit(ChestIndex), PackHit(0, 0)));
		TestTrue(TEXT("the chest cell is empty again"), Chest.Slots[ChestIndex] == nullptr);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryCraftSlotsTest,
	"SolidCore1.Inventory.CraftSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryCraftSlotsTest::RunTest(const FString& Parameters)
{
	using namespace SolidInventoryTestPrivate;

	TestEqual(TEXT("four crafting slots"), SolidInventory::CraftCount, 4);
	TestEqual(
		TEXT("one slot for each recipe input"),
		SolidInventory::CraftCount,
		SolidCrafting::MaxInputs);
	TestEqual(TEXT("the button reads Craft"), FString(SolidInventory::CraftButtonLabel), FString(TEXT("Craft")));

	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page);
	TestFalse(TEXT("the Craft button starts disabled"), Page.bCraftEnabled);

	const SolidInventory::FBox* Boots = FindWorn(Page, SolidInventory::ESlot::Boots);
	TestTrue(TEXT("boots box"), Boots != nullptr);
	if (!Boots)
	{
		return false;
	}

	const SolidInventory::FBox* First = FindCraft(Page, 0);
	TestTrue(TEXT("the first crafting slot is on the page"), First != nullptr);
	if (!First)
	{
		return false;
	}
	TestTrue(TEXT("the crafting row is below the boots"), First->Y >= Boots->Y + Boots->H);
	TestTrue(
		TEXT("the Craft button is to the right of the slots"),
		Page.CraftButtonX >= First->X + First->W);
	const float ButtonMidY = Page.CraftButtonY + Page.CraftButtonH * 0.5f;
	const float RowMidY = First->Y + First->H * 0.5f;
	TestTrue(
		TEXT("the Craft button sits on the crafting row"),
		FMath::IsNearlyEqual(ButtonMidY, RowMidY, 1.f));
	TestTrue(
		TEXT("the button meets the right edge"),
		FMath::IsNearlyEqual(Page.CraftButtonX + Page.CraftButtonW, Page.ContentW, 1.f));

	int32 CraftBoxes = 0;
	const SolidInventory::FBox* Previous = nullptr;
	for (int32 Index = 0; Index < SolidInventory::CraftCount; ++Index)
	{
		const SolidInventory::FBox* Slot = FindCraft(Page, Index);
		TestTrue(TEXT("a crafting slot is on the page"), Slot != nullptr);
		if (!Slot)
		{
			return false;
		}
		++CraftBoxes;
		TestTrue(TEXT("a crafting slot starts empty"), Slot->Item.IsEmpty());
		TestTrue(TEXT("crafting slots match"), FMath::IsNearlyEqual(Slot->W, First->W, 1.f) && FMath::IsNearlyEqual(Slot->H, First->H, 1.f));
		if (Previous)
		{
			TestTrue(TEXT("the next crafting slot is to the right"), Slot->X > Previous->X + Previous->W);
			TestTrue(TEXT("the crafting slots share a row"), FMath::IsNearlyEqual(Slot->Y, Previous->Y, 1.f));
		}
		Previous = Slot;
	}
	TestEqual(TEXT("four crafting boxes"), CraftBoxes, SolidInventory::CraftCount);
	TestTrue(
		TEXT("the Craft button follows the last slot"),
		Previous && Page.CraftButtonX + 1.f >= Previous->X + Previous->W);

	const float ButtonX = Page.CraftButtonX + Page.CraftButtonW * 0.5f;
	const float ButtonY = Page.CraftButtonY + Page.CraftButtonH * 0.5f;
	SolidInventory::FSlotHit ButtonHit;
	TestFalse(TEXT("the Craft button is not a slot"), SolidInventory::HitSlot(Page, ButtonX, ButtonY, ButtonHit));

	const float SlotX = First->X + First->W * 0.5f;
	const float SlotY = First->Y + First->H * 0.5f;
	SolidInventory::FSlotHit SlotHit;
	TestTrue(TEXT("the first crafting slot accepts the pointer"), SolidInventory::HitSlot(Page, SlotX, SlotY, SlotHit));
	TestTrue(TEXT("the pointer is a crafting slot"), SlotHit.bCraft);
	TestEqual(TEXT("the pointer is the first slot"), SlotHit.Index, 0);

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	TestTrue(
		TEXT("the spear can be dragged into a crafting slot"),
		SolidInventory::Move(&Chest, Loadout, WornHit(SolidInventory::ESlot::MainHand), CraftHit(0)));
	TestEqual(TEXT("the crafting slot holds the spear"), HeldId(Loadout.Craft[0]), FString(SolidItemType::IdCrudeSpear));
	TestEqual(TEXT("one spear"), Loadout.CraftStack[0], 1);
	TestTrue(TEXT("the main hand is empty"), Loadout.Worn[Main] == nullptr);

	SolidInventory::FPage Holding;
	SolidInventory::Layout(Loadout, Holding, &Chest);
	const SolidInventory::FBox* SpearSlot = FindCraft(Holding, 0);
	TestTrue(TEXT("the filled slot is on the page"), SpearSlot != nullptr);
	if (!SpearSlot)
	{
		return false;
	}
	TestEqual(TEXT("the slot shows the spear"), SpearSlot->Item, FString(TEXT("Crude spear")));
	TestFalse(TEXT("the Craft button stays disabled"), Holding.bCraftEnabled);

	const int32 StickIndex = SolidStorage::Index(1, 1);
	TestTrue(
		TEXT("the stick stack can be dragged into a crafting slot"),
		SolidInventory::Move(&Chest, Loadout, ChestHit(StickIndex), CraftHit(1)));
	TestEqual(TEXT("the second slot holds the sticks"), HeldId(Loadout.Craft[1]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the slot keeps the stack"), Loadout.CraftStack[1], SolidStorage::StartingStickCount);
	TestTrue(TEXT("the chest cell is empty"), Chest.Slots[StickIndex] == nullptr);

	TestTrue(
		TEXT("crafting slots can swap"),
		SolidInventory::Move(&Chest, Loadout, CraftHit(0), CraftHit(1)));
	TestEqual(TEXT("the first slot holds the sticks"), HeldId(Loadout.Craft[0]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the second slot holds the spear"), HeldId(Loadout.Craft[1]), FString(SolidItemType::IdCrudeSpear));

	TestTrue(
		TEXT("a crafting slot can be dragged back to the main hand"),
		SolidInventory::Move(&Chest, Loadout, CraftHit(1), WornHit(SolidInventory::ESlot::MainHand)));
	TestEqual(TEXT("the main hand holds the spear again"), HeldId(Loadout.Worn[Main]), FString(SolidItemType::IdCrudeSpear));
	TestTrue(TEXT("the crafting slot is empty"), Loadout.Craft[1] == nullptr);

	TestFalse(
		TEXT("an empty crafting slot cannot be dragged"),
		SolidInventory::Move(&Chest, Loadout, CraftHit(2), WornHit(SolidInventory::ESlot::OffHand)));
	TestTrue(
		TEXT("the off hand stays empty"),
		Loadout.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::OffHand)] == nullptr);

	SolidInventory::Clear(Loadout);
	for (int32 Index = 0; Index < SolidInventory::CraftCount; ++Index)
	{
		TestTrue(TEXT("clear empties a crafting slot"), Loadout.Craft[Index] == nullptr);
		TestEqual(TEXT("clear leaves no stack"), Loadout.CraftStack[Index], 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryCraftButtonTest,
	"SolidCore1.Inventory.CraftButton",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryCraftButtonTest::RunTest(const FString& Parameters)
{
	using namespace SolidInventoryTestPrivate;

	SolidInventory::FPage Page;
	SolidInventory::Layout(SolidInventory::StartingLoadout(), Page);
	const SolidInventory::FBox* Slot = FindCraft(Page, 0);
	TestTrue(TEXT("a crafting slot is on the page"), Slot != nullptr);
	if (!Slot)
	{
		return false;
	}
	TestTrue(TEXT("the Craft button is shorter than a slot"), Page.CraftButtonH < Slot->H);
	TestTrue(TEXT("the Craft button is narrower than a slot"), Page.CraftButtonW < Slot->W);
	TestTrue(TEXT("the Craft button starts below the slot top"), Page.CraftButtonY > Slot->Y);
	TestTrue(
		TEXT("the Craft button ends above the slot bottom"),
		Page.CraftButtonY + Page.CraftButtonH < Slot->Y + Slot->H);
	TestFalse(TEXT("the Craft button stays disabled"), Page.bCraftEnabled);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryCraftReadyTest,
	"SolidCore1.Inventory.CraftReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryCraftReadyTest::RunTest(const FString& Parameters)
{
	using namespace SolidInventoryTestPrivate;

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	SolidInventory::FPage Page;
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestFalse(TEXT("an empty crafting row leaves the button off"), Page.bCraftEnabled);

	const int32 StickIndex = SolidStorage::Index(1, 1);
	const int32 PitchIndex = SolidStorage::Index(0, 2);
	TestTrue(
		TEXT("the sticks drop into a crafting slot"),
		SolidInventory::Move(&Chest, Loadout, ChestHit(StickIndex), CraftHit(0)));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestFalse(TEXT("sticks alone leave the button off"), Page.bCraftEnabled);

	TestTrue(
		TEXT("the pitch drops into a crafting slot"),
		SolidInventory::Move(&Chest, Loadout, ChestHit(PitchIndex), CraftHit(1)));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestTrue(TEXT("stick and pitch light the Craft button"), Page.bCraftEnabled);
	TestEqual(TEXT("the stick stack is intact"), Loadout.CraftStack[0], SolidStorage::StartingStickCount);
	TestEqual(TEXT("the pitch stack is intact"), Loadout.CraftStack[1], SolidStorage::StartingPitchCount);

	TestTrue(
		TEXT("the ingredients can swap slots"),
		SolidInventory::Move(&Chest, Loadout, CraftHit(0), CraftHit(1)));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestTrue(TEXT("the recipe still fits after the swap"), Page.bCraftEnabled);

	TestTrue(
		TEXT("the spear can sit beside the ingredients"),
		SolidInventory::Move(&Chest, Loadout, ChestHit(SolidStorage::Index(0, 0)), CraftHit(2)));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestTrue(TEXT("an extra item does not block the recipe"), Page.bCraftEnabled);

	TestTrue(
		TEXT("the pitch can leave the crafting row"),
		SolidInventory::Move(&Chest, Loadout, CraftHit(0), PackHit(0, 0)));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestFalse(TEXT("the button goes dim when the recipe no longer fits"), Page.bCraftEnabled);

	TestTrue(
		TEXT("the pitch can return"),
		SolidInventory::Move(&Chest, Loadout, PackHit(0, 0), CraftHit(0)));
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		const int32 Column = Index % SolidInventory::PackColumns;
		const int32 Row = Index / SolidInventory::PackColumns;
		TestTrue(TEXT("a pack cell can be filled"), SolidInventory::PlacePack(Loadout, Column, Row, SolidItemType::IdRags));
	}
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestFalse(TEXT("a full pack keeps the button off"), Page.bCraftEnabled);

	TestTrue(TEXT("one pack cell can be cleared"), SolidInventory::PlacePack(Loadout, 0, 0, nullptr));
	SolidInventory::Layout(Loadout, Page, &Chest);
	TestTrue(TEXT("a free pack cell lights the button again"), Page.bCraftEnabled);

	FSolidRecipe Hidden = SolidCrafting::Recipes[0];
	Hidden.bKnown = false;
	TestFalse(TEXT("an unknown recipe does not fit"), SolidInventory::RecipeFitsCraftSlots(Loadout, Hidden));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryCraftPressTest,
	"SolidCore1.Inventory.CraftPress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryCraftPressTest::RunTest(const FString& Parameters)
{
	using namespace SolidInventoryTestPrivate;

	SolidInventory::FLoadout Empty = SolidInventory::StartingLoadout();
	TestFalse(TEXT("crafting with the button off does nothing"), SolidInventory::Craft(Empty));
	TestTrue(TEXT("the pack stays empty"), Empty.Pack[0] == nullptr);

	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	SolidInventory::WriteCraft(Loadout, 0, SolidItemType::IdStick, SolidStorage::StartingStickCount);
	SolidInventory::WriteCraft(Loadout, 1, SolidItemType::IdPitch, SolidStorage::StartingPitchCount);
	SolidInventory::WriteCraft(Loadout, 2, SolidItemType::IdRustySword, 1);
	SolidInventory::FPage Page;
	SolidInventory::Layout(Loadout, Page);
	TestTrue(TEXT("the button is lit"), Page.bCraftEnabled);
	const float ButtonX = Page.CraftButtonX + Page.CraftButtonW * 0.5f;
	const float ButtonY = Page.CraftButtonY + Page.CraftButtonH * 0.5f;
	TestTrue(TEXT("the pointer hits the Craft button"), SolidInventory::HitCraftButton(Page, ButtonX, ButtonY));
	const SolidInventory::FBox* StickSlot = FindCraft(Page, 0);
	TestTrue(TEXT("the stick slot is on the page"), StickSlot != nullptr);
	if (!StickSlot)
	{
		return false;
	}
	TestFalse(
		TEXT("a crafting slot is not the Craft button"),
		SolidInventory::HitCraftButton(Page, StickSlot->X + StickSlot->W * 0.5f, StickSlot->Y + StickSlot->H * 0.5f));

	TestTrue(TEXT("the first press crafts a torch"), SolidInventory::Craft(Loadout));
	TestEqual(TEXT("one stick was spent"), Loadout.CraftStack[0], SolidStorage::StartingStickCount - 1);
	TestEqual(TEXT("the stick stack remains"), FString(Loadout.Craft[0]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("one pitch was spent"), Loadout.CraftStack[1], SolidStorage::StartingPitchCount - 1);
	TestEqual(TEXT("the pitch stack remains"), FString(Loadout.Craft[1]), FString(SolidItemType::IdPitch));
	TestEqual(TEXT("the sword stays in its slot"), FString(Loadout.Craft[2]), FString(SolidItemType::IdRustySword));
	TestEqual(
		TEXT("the torch is in the first empty pack cell"),
		FString(Loadout.Pack[0]),
		FString(SolidItemType::IdTorch));
	TestEqual(TEXT("one torch"), Loadout.PackStack[0], 1);
	SolidInventory::Layout(Loadout, Page);
	TestTrue(TEXT("the button stays lit while another torch can be made"), Page.bCraftEnabled);

	TestTrue(TEXT("the second press crafts another torch"), SolidInventory::Craft(Loadout));
	TestTrue(TEXT("the third press crafts another torch"), SolidInventory::Craft(Loadout));
	TestTrue(TEXT("the pitch stack is gone"), Loadout.Craft[1] == nullptr);
	TestEqual(TEXT("the pitch count is zero"), Loadout.CraftStack[1], 0);
	TestEqual(TEXT("two sticks remain"), Loadout.CraftStack[0], SolidStorage::StartingStickCount - SolidStorage::StartingPitchCount);
	TestEqual(
		TEXT("the third torch filled the next pack cell"),
		FString(Loadout.Pack[2]),
		FString(SolidItemType::IdTorch));
	SolidInventory::Layout(Loadout, Page);
	TestFalse(TEXT("the button goes dim when the pitch is gone"), Page.bCraftEnabled);
	TestFalse(TEXT("another press does nothing"), SolidInventory::Craft(Loadout));

	SolidInventory::FLoadout Single = SolidInventory::StartingLoadout();
	SolidInventory::WriteCraft(Single, 0, SolidItemType::IdStick, 1);
	SolidInventory::WriteCraft(Single, 3, SolidItemType::IdPitch, 1);
	TestTrue(TEXT("one of each still crafts"), SolidInventory::Craft(Single));
	TestTrue(TEXT("the stick slot is cleared"), Single.Craft[0] == nullptr);
	TestEqual(TEXT("the stick count is zero"), Single.CraftStack[0], 0);
	TestTrue(TEXT("the pitch slot is cleared"), Single.Craft[3] == nullptr);
	TestEqual(TEXT("the pitch count is zero"), Single.CraftStack[3], 0);
	TestEqual(TEXT("the single torch is in the pack"), FString(Single.Pack[0]), FString(SolidItemType::IdTorch));
	SolidInventory::Layout(Single, Page);
	TestFalse(TEXT("the button goes dim when the slots are empty"), Page.bCraftEnabled);

	SolidInventory::FLoadout LastCell = SolidInventory::StartingLoadout();
	SolidInventory::WriteCraft(LastCell, 0, SolidItemType::IdStick, SolidStorage::StartingStickCount);
	SolidInventory::WriteCraft(LastCell, 1, SolidItemType::IdPitch, SolidStorage::StartingPitchCount);
	for (int32 Index = 0; Index < SolidInventory::PackCount - 1; ++Index)
	{
		const int32 Column = Index % SolidInventory::PackColumns;
		const int32 Row = Index / SolidInventory::PackColumns;
		TestTrue(TEXT("the pack can be filled up to the last cell"), SolidInventory::PlacePack(LastCell, Column, Row, SolidItemType::IdRags));
	}
	TestTrue(TEXT("the last empty cell can take the torch"), SolidInventory::Craft(LastCell));
	const int32 LastPack = SolidInventory::PackCount - 1;
	TestEqual(
		TEXT("the torch took the last pack cell"),
		FString(LastCell.Pack[LastPack]),
		FString(SolidItemType::IdTorch));
	SolidInventory::Layout(LastCell, Page);
	TestFalse(TEXT("a full pack puts the button out"), Page.bCraftEnabled);
	TestEqual(TEXT("sticks remain for another torch"), LastCell.CraftStack[0], SolidStorage::StartingStickCount - 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryInsertPackTest,
	"SolidCore1.Inventory.InsertPack",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryInsertPackTest::RunTest(const FString& Parameters)
{
	SolidInventory::FLoadout Loadout = SolidInventory::StartingLoadout();
	const int32 Body = SolidInventory::WornIndex(SolidInventory::ESlot::Body);
	const TCHAR* WornBody = Loadout.Worn[Body];
	SolidInventory::WriteCraft(Loadout, 0, SolidItemType::IdStick, 4);

	TestFalse(TEXT("an unknown id is refused"), SolidInventory::InsertPack(Loadout, TEXT("missing")));
	TestFalse(TEXT("an empty id is refused"), SolidInventory::InsertPack(Loadout, TEXT("")));
	TestTrue(TEXT("the first stick takes the first cell"), SolidInventory::InsertPack(Loadout, SolidItemType::IdStick));
	TestEqual(TEXT("that cell is a stick"), FString(Loadout.Pack[0]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the stack holds one"), Loadout.PackStack[0], 1);
	TestTrue(TEXT("the second stick joins the stack"), SolidInventory::InsertPack(Loadout, SolidItemType::IdStick));
	TestEqual(TEXT("the stack holds two"), Loadout.PackStack[0], 2);
	TestTrue(TEXT("the second cell is still empty"), Loadout.Pack[1] == nullptr);

	TestTrue(TEXT("pitch starts its own stack"), SolidInventory::InsertPack(Loadout, SolidItemType::IdPitch));
	TestEqual(TEXT("pitch sits in the next cell"), FString(Loadout.Pack[1]), FString(SolidItemType::IdPitch));
	TestEqual(TEXT("the stick stack is unchanged"), Loadout.PackStack[0], 2);

	SolidInventory::WritePack(Loadout, 0, SolidItemType::IdStick, 10);
	TestTrue(TEXT("a full stick stack spills into an empty cell"), SolidInventory::InsertPack(Loadout, SolidItemType::IdStick));
	TestEqual(TEXT("the full stack stays at ten"), Loadout.PackStack[0], 10);
	TestEqual(TEXT("the spill is a stick"), FString(Loadout.Pack[2]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the spill holds one"), Loadout.PackStack[2], 1);

	SolidInventory::FLoadout Full = SolidInventory::StartingLoadout();
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		SolidInventory::WritePack(Full, Index, SolidItemType::IdStick, 10);
	}
	TestFalse(TEXT("a pack of full stick stacks refuses another"), SolidInventory::InsertPack(Full, SolidItemType::IdStick));
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestEqual(TEXT("a refused insert leaves the stack"), Full.PackStack[Index], 10);
	}

	TestEqual(TEXT("the body is untouched"), FString(Loadout.Worn[Body]), FString(WornBody));
	TestEqual(TEXT("the craft slot is untouched"), FString(Loadout.Craft[0]), FString(SolidItemType::IdStick));
	TestEqual(TEXT("the craft count is untouched"), Loadout.CraftStack[0], 4);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
