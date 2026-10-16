#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/SolidInventory.h"
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryStartingKitTest,
	"SolidCore1.Inventory.StartingKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryStartingKitTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("helmet name"), FString(SolidInventory::DentedHelmet), FString(TEXT("Dented helmet")));
	TestEqual(TEXT("shirt name"), FString(SolidInventory::RagShirt), FString(TEXT("Rag shirt")));
	TestEqual(TEXT("trews name"), FString(SolidInventory::RagTrews), FString(TEXT("Rag trews")));
	TestEqual(TEXT("spear name"), FString(SolidInventory::CrudeSpear), FString(TEXT("Crude spear")));

	const FGameState State;
	const SolidInventory::FLoadout& Kit = State.Inventory;
	TestEqual(TEXT("head"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Head)], FString(SolidInventory::DentedHelmet));
	TestEqual(TEXT("body"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Body)], FString(SolidInventory::RagShirt));
	TestEqual(TEXT("legs"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Legs)], FString(SolidInventory::RagTrews));
	TestEqual(TEXT("right hand"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::RightHand)], FString(SolidInventory::CrudeSpear));
	TestTrue(TEXT("left hand starts empty"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::LeftHand)].IsEmpty());
	TestTrue(TEXT("belt starts empty"), Kit.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::Belt)].IsEmpty());
	TestEqual(TEXT("pack has six cells"), SolidInventory::PackCount, SolidInventory::PackColumns * SolidInventory::PackRows);
	TestEqual(TEXT("pack is two by three"), SolidInventory::PackColumns, 2);
	TestEqual(TEXT("pack is three rows"), SolidInventory::PackRows, 3);
	for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
	{
		TestTrue(TEXT("pack starts empty"), Kit.Pack[Index].IsEmpty());
	}

	SolidInventory::FLoadout Edited = Kit;
	TestTrue(TEXT("spear can leave the hand"), SolidInventory::PlaceWorn(Edited, SolidInventory::ESlot::RightHand, FString()));
	TestTrue(
		TEXT("spear can be placed in the pack"),
		SolidInventory::PlacePack(Edited, 1, 2, SolidInventory::CrudeSpear));
	TestTrue(TEXT("hand is empty after the move"), Edited.Worn[SolidInventory::WornIndex(SolidInventory::ESlot::RightHand)].IsEmpty());
	TestEqual(
		TEXT("bottom-right pack cell holds the spear"),
		Edited.Pack[SolidInventory::PackIndex(1, 2)],
		FString(SolidInventory::CrudeSpear));
	TestFalse(TEXT("a cell outside the pack is refused"), SolidInventory::PlacePack(Edited, 2, 0, SolidInventory::CrudeSpear));
	TestTrue(TEXT("the spear stays inside the pack"), Edited.Pack[SolidInventory::PackIndex(0, 0)].IsEmpty());
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
	const SolidInventory::FBox* Legs = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Legs);
	const SolidInventory::FBox* Left = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::LeftHand);
	const SolidInventory::FBox* Right = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::RightHand);
	const SolidInventory::FBox* Belt = SolidInventoryTestPrivate::FindWorn(Page, SolidInventory::ESlot::Belt);
	TestTrue(TEXT("head box"), Head != nullptr);
	TestTrue(TEXT("body box"), Body != nullptr);
	TestTrue(TEXT("legs box"), Legs != nullptr);
	TestTrue(TEXT("left hand box"), Left != nullptr);
	TestTrue(TEXT("right hand box"), Right != nullptr);
	TestTrue(TEXT("belt box"), Belt != nullptr);
	if (!Head || !Body || !Legs || !Left || !Right || !Belt)
	{
		return false;
	}

	TestEqual(TEXT("head label"), Head->Label, FString(TEXT("Head")));
	TestEqual(TEXT("body label"), Body->Label, FString(TEXT("Body")));
	TestEqual(TEXT("legs label"), Legs->Label, FString(TEXT("Legs")));
	TestEqual(TEXT("left hand label"), Left->Label, FString(TEXT("Left hand")));
	TestEqual(TEXT("right hand label"), Right->Label, FString(TEXT("Right hand")));
	TestEqual(TEXT("belt label"), Belt->Label, FString(TEXT("Belt")));
	TestEqual(TEXT("head wears the helmet"), Head->Item, FString(SolidInventory::DentedHelmet));
	TestEqual(TEXT("body wears the shirt"), Body->Item, FString(SolidInventory::RagShirt));
	TestEqual(TEXT("legs wear the trews"), Legs->Item, FString(SolidInventory::RagTrews));
	TestEqual(TEXT("right hand holds the spear"), Right->Item, FString(SolidInventory::CrudeSpear));
	TestTrue(TEXT("left hand box is empty"), Left->Item.IsEmpty());
	TestTrue(TEXT("belt box is empty"), Belt->Item.IsEmpty());

	TestTrue(TEXT("head is above the body"), Head->Y + Head->H < Body->Y);
	TestTrue(
		TEXT("head is centered on the body"),
		FMath::IsNearlyEqual(Head->X + Head->W * 0.5f, Body->X + Body->W * 0.5f, 1.f));
	TestTrue(TEXT("head is narrower than the body"), Head->W < Body->W);
	TestTrue(TEXT("left hand is left of the body"), Left->X + Left->W < Body->X);
	TestTrue(TEXT("right hand is right of the body"), Right->X > Body->X + Body->W);
	TestTrue(
		TEXT("left hand meets the body in height"),
		Left->Y < Body->Y + Body->H && Left->Y + Left->H > Body->Y);
	TestTrue(
		TEXT("right hand meets the body in height"),
		Right->Y < Body->Y + Body->H && Right->Y + Right->H > Body->Y);
	TestTrue(
		TEXT("hands share a height"),
		FMath::IsNearlyEqual(Left->Y, Right->Y, 1.f) && FMath::IsNearlyEqual(Left->H, Right->H, 1.f));
	TestTrue(TEXT("belt is below the body"), Body->Y + Body->H < Belt->Y);
	TestTrue(TEXT("legs are below the belt"), Belt->Y + Belt->H < Legs->Y);
	TestTrue(TEXT("belt is narrower than the body"), Belt->W < Body->W);
	TestTrue(
		TEXT("belt is centered on the body"),
		FMath::IsNearlyEqual(Belt->X + Belt->W * 0.5f, Body->X + Body->W * 0.5f, 1.f));
	TestTrue(
		TEXT("legs share the body's column"),
		FMath::IsNearlyEqual(Legs->X, Body->X, 1.f) && FMath::IsNearlyEqual(Legs->W, Body->W, 1.f));

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
	TestEqual(TEXT("close hint"), FString(SolidInventory::CloseHint), FString(TEXT("# to close")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidInventoryToggleTest,
	"SolidCore1.Inventory.Toggle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidInventoryToggleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("UK hash key"), SolidInventory::IsToggleKey(EKeys::Backslash, false));
	TestFalse(TEXT("shifted UK hash is not #"), SolidInventory::IsToggleKey(EKeys::Backslash, true));
	TestTrue(TEXT("US hash is shift 3"), SolidInventory::IsToggleKey(EKeys::Three, true));
	TestFalse(TEXT("3 alone is the menu key"), SolidInventory::IsToggleKey(EKeys::Three, false));
	TestEqual(TEXT("debug label"), FString(SolidInventory::ToggleKeyLabel), FString(TEXT("#")));

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	TestFalse(TEXT("inventory starts closed"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();
	TestTrue(TEXT("# opens the inventory"), HUD->IsInventoryVisible());
	HUD->ToggleInventory();
	TestFalse(TEXT("# closes the inventory"), HUD->IsInventoryVisible());

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
	TestFalse(TEXT("# waits until the message closes"), HUD->IsInventoryVisible());
	HUD->CloseMessage();
	HUD->ToggleInventory();
	TestTrue(TEXT("# opens after the message"), HUD->IsInventoryVisible());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
