#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidHUD.h"
#include "SolidMainMenu.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMainMenuEntriesTest,
	"SolidCore1.MainMenu.Entries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuEntriesTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TestTrue(TEXT("menu lists rows"), Entries.Num() >= 1);
	TestEqual(TEXT("entry count"), Entries.Num(), SolidMainMenu::EntryCount());

	TestEqual(TEXT("first is Use portal"), Entries[0].Label, FString(TEXT("Use portal")));
	TestEqual(
		TEXT("first item"),
		static_cast<uint8>(Entries[0].Item),
		static_cast<uint8>(ESolidMainMenuItem::UsePortal));
	TestEqual(
		TEXT("portal action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[0].Item)),
		static_cast<uint8>(ESolidMainMenuAction::OpenPortal));

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		FSolidMainMenuEntry Found;
		TestTrue(TEXT("find row"), SolidMainMenu::FindEntry(Index, Found));
		TestEqual(TEXT("found label"), Found.Label, Entries[Index].Label);
		TestEqual(
			TEXT("found item"),
			static_cast<uint8>(Found.Item),
			static_cast<uint8>(Entries[Index].Item));
	}

	FSolidMainMenuEntry Missing;
	TestFalse(TEXT("no entry past the end"), SolidMainMenu::FindEntry(SolidMainMenu::EntryCount(), Missing));

	TArray<FSolidMainMenuEntry> InCave;
	SolidMainMenu::CollectEntries(InCave, true);
	for (const FSolidMainMenuEntry& Entry : InCave)
	{
		if (Entry.Item == ESolidMainMenuItem::Dungeon)
		{
			TestEqual(TEXT("cave row leaves"), Entry.Label, FString(TEXT("Leave cave")));
		}
	}

	const int32 Count = SolidMainMenu::EntryCount();
	const int32 Last = Count - 1;
	TestEqual(TEXT("wrap down"), SolidMainMenu::WrapIndex(0, 1), Count > 1 ? 1 : 0);
	TestEqual(TEXT("wrap past end"), SolidMainMenu::WrapIndex(Last, 1), 0);
	TestEqual(TEXT("wrap up"), SolidMainMenu::WrapIndex(0, -1), Last);
	if (Count > 2)
	{
		TestEqual(TEXT("wrap follows the list"), SolidMainMenu::WrapIndex(1, 1), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMainMenuHudTest,
	"SolidCore1.MainMenu.Hud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuHudTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	TestFalse(TEXT("menu closed"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("credits closed"), HUD->IsCreditsVisible());
	TestEqual(TEXT("selection starts at Use portal"), HUD->GetMainMenuIndex(), 0);

	HUD->HandleMenuKey();
	TestTrue(TEXT("F10 opens menu"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("opening menu does not show credits"), HUD->IsCreditsVisible());

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	for (int32 Index = 1; Index < Entries.Num(); ++Index)
	{
		HUD->MoveMainMenuSelection(1);
		TestEqual(TEXT("down follows the list"), HUD->GetMainMenuIndex(), Index);
	}
	HUD->MoveMainMenuSelection(1);
	TestEqual(TEXT("down wraps to Use portal"), HUD->GetMainMenuIndex(), 0);
	HUD->MoveMainMenuSelection(-1);
	TestEqual(TEXT("up wraps to the last row"), HUD->GetMainMenuIndex(), Entries.Num() - 1);

	HUD->SetMainMenuIndex(0);
	TestEqual(TEXT("number key selects Use portal"), HUD->GetMainMenuIndex(), 0);
	if (Entries.Num() > 1)
	{
		HUD->SetMainMenuIndex(1);
		TestEqual(TEXT("number key selects the next row"), HUD->GetMainMenuIndex(), 1);
	}
	HUD->SetMainMenuIndex(99);
	TestEqual(TEXT("index clamps"), HUD->GetMainMenuIndex(), Entries.Num() - 1);

	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes menu"), HUD->IsMainMenuOpen());

	HUD->SetCreditsVisible(true);
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes credits"), HUD->IsCreditsVisible());
	TestFalse(TEXT("closing credits does not open the menu"), HUD->IsMainMenuOpen());

	HUD->HandleMenuKey();
	HUD->SetCreditsVisible(true);
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes menu"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("Esc closes credits"), HUD->IsCreditsVisible());

	HUD->SetJournalVisible(true);
	TestTrue(TEXT("journal opens"), HUD->IsJournalVisible());
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes journal"), HUD->IsJournalVisible());
	TestFalse(TEXT("closing journal does not open the menu"), HUD->IsMainMenuOpen());

	HUD->SetJournalVisible(true);
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes journal"), HUD->IsJournalVisible());

	HUD->OpenSquadBuilder();
	TestTrue(TEXT("squad builder opens"), HUD->IsSquadBuilderOpen());
	TestFalse(TEXT("squad builder closes the menu"), HUD->IsMainMenuOpen());
	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 closes the squad builder"), HUD->IsSquadBuilderOpen());
	TestFalse(TEXT("closing the squad builder does not open the menu"), HUD->IsMainMenuOpen());
	HUD->OpenSquadBuilder();
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc closes the squad builder"), HUD->IsSquadBuilderOpen());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMainMenuCaveRowTest,
	"SolidCore1.MainMenu.Cave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuCaveRowTest::RunTest(const FString& Parameters)
{
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMainMenuControllerKeyTest,
	"SolidCore1.MainMenu.Controller",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuControllerKeyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("menu button opens the menu"), SolidMainMenu::IsControllerKey(EKeys::Gamepad_Special_Right));
	TestFalse(TEXT("view stays inventory"), SolidMainMenu::IsControllerKey(EKeys::Gamepad_Special_Left));
	TestFalse(TEXT("A stays jump"), SolidMainMenu::IsControllerKey(EKeys::Gamepad_FaceButton_Bottom));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
