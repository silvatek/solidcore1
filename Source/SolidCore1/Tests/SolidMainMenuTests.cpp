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
	TestEqual(TEXT("four entries"), Entries.Num(), 4);
	TestEqual(TEXT("entry count"), SolidMainMenu::EntryCount(), 4);

	TestEqual(TEXT("first is Use portal"), Entries[0].Label, FString(TEXT("Use portal")));
	TestEqual(
		TEXT("first item"),
		static_cast<uint8>(Entries[0].Item),
		static_cast<uint8>(ESolidMainMenuItem::UsePortal));
	TestEqual(
		TEXT("portal action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[0].Item)),
		static_cast<uint8>(ESolidMainMenuAction::OpenPortal));

	TestEqual(TEXT("second is Test Drill"), Entries[1].Label, FString(TEXT("Test Drill")));
	TestEqual(
		TEXT("second item"),
		static_cast<uint8>(Entries[1].Item),
		static_cast<uint8>(ESolidMainMenuItem::TestDrill));
	TestEqual(
		TEXT("drill action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[1].Item)),
		static_cast<uint8>(ESolidMainMenuAction::StartTestDrill));

	TestEqual(TEXT("third is Journal"), Entries[2].Label, FString(TEXT("Journal")));
	TestEqual(
		TEXT("third item"),
		static_cast<uint8>(Entries[2].Item),
		static_cast<uint8>(ESolidMainMenuItem::Journal));
	TestEqual(
		TEXT("journal action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[2].Item)),
		static_cast<uint8>(ESolidMainMenuAction::ShowJournal));

	TestEqual(TEXT("fourth is Credits"), Entries[3].Label, FString(TEXT("Credits")));
	TestEqual(
		TEXT("fourth item"),
		static_cast<uint8>(Entries[3].Item),
		static_cast<uint8>(ESolidMainMenuItem::Credits));
	TestEqual(
		TEXT("credits action"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[3].Item)),
		static_cast<uint8>(ESolidMainMenuAction::ShowCredits));

	FSolidMainMenuEntry Found;
	TestTrue(TEXT("find first"), SolidMainMenu::FindEntry(0, Found));
	TestEqual(TEXT("found label"), Found.Label, FString(TEXT("Use portal")));
	TestFalse(TEXT("no fifth entry"), SolidMainMenu::FindEntry(4, Found));

	TestEqual(TEXT("wrap down"), SolidMainMenu::WrapIndex(0, 1), 1);
	TestEqual(TEXT("wrap to credits"), SolidMainMenu::WrapIndex(2, 1), 3);
	TestEqual(TEXT("wrap past end"), SolidMainMenu::WrapIndex(3, 1), 0);
	TestEqual(TEXT("wrap up"), SolidMainMenu::WrapIndex(0, -1), 3);
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

	HUD->MoveMainMenuSelection(1);
	TestEqual(TEXT("down selects Test Drill"), HUD->GetMainMenuIndex(), 1);
	HUD->MoveMainMenuSelection(1);
	TestEqual(TEXT("down selects Journal"), HUD->GetMainMenuIndex(), 2);
	HUD->MoveMainMenuSelection(1);
	TestEqual(TEXT("down selects Credits"), HUD->GetMainMenuIndex(), 3);
	HUD->MoveMainMenuSelection(1);
	TestEqual(TEXT("down wraps to Use portal"), HUD->GetMainMenuIndex(), 0);
	HUD->MoveMainMenuSelection(-1);
	TestEqual(TEXT("up wraps to Credits"), HUD->GetMainMenuIndex(), 3);

	HUD->SetMainMenuIndex(0);
	TestEqual(TEXT("number key selects Use portal"), HUD->GetMainMenuIndex(), 0);
	HUD->SetMainMenuIndex(1);
	TestEqual(TEXT("number key selects Test Drill"), HUD->GetMainMenuIndex(), 1);
	HUD->SetMainMenuIndex(99);
	TestEqual(TEXT("index clamps"), HUD->GetMainMenuIndex(), 3);

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
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
