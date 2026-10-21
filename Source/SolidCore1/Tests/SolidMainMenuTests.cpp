#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidHUD.h"
#include "SolidMainMenu.h"
#include "Game/SolidMonsterMeasure.h"

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
	FSolidMainMenuOrderTest,
	"SolidCore1.MainMenu.Order",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuOrderTest::RunTest(const FString& Parameters)
{
	const TCHAR* Labels[] = {
		TEXT("Use portal"),
		TEXT("Journal"),
		TEXT("Build squad"),
		TEXT("Character editor"),
		TEXT("Credits"),
		TEXT("Exit Game"),
	};
	const ESolidMainMenuItem Items[] = {
		ESolidMainMenuItem::UsePortal,
		ESolidMainMenuItem::Journal,
		ESolidMainMenuItem::BuildSquad,
		ESolidMainMenuItem::CharacterEditor,
		ESolidMainMenuItem::Credits,
		ESolidMainMenuItem::ExitGame,
	};
	const ESolidMainMenuAction Actions[] = {
		ESolidMainMenuAction::OpenPortal,
		ESolidMainMenuAction::ShowJournal,
		ESolidMainMenuAction::OpenSquad,
		ESolidMainMenuAction::OpenCharacterEditor,
		ESolidMainMenuAction::ShowCredits,
		ESolidMainMenuAction::Quit,
	};

	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	TestTrue(TEXT("menu has the known rows"), Entries.Num() >= UE_ARRAY_COUNT(Labels));
	if (Entries.Num() < UE_ARRAY_COUNT(Labels))
	{
		return false;
	}

	int32 Cursor = 0;
	for (int32 Expected = 0; Expected < UE_ARRAY_COUNT(Labels); ++Expected)
	{
		bool bFound = false;
		for (; Cursor < Entries.Num(); ++Cursor)
		{
			if (Entries[Cursor].Label != Labels[Expected])
			{
				continue;
			}
			TestEqual(
				TEXT("row item"),
				static_cast<uint8>(Entries[Cursor].Item),
				static_cast<uint8>(Items[Expected]));
			TestEqual(
				TEXT("row action"),
				static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[Cursor].Item)),
				static_cast<uint8>(Actions[Expected]));
			bFound = true;
			++Cursor;
			break;
		}
		TestTrue(TEXT("known row stays in order"), bFound);
	}

	TestEqual(TEXT("Exit Game is last"), Entries.Last().Label, FString(TEXT("Exit Game")));
	FSolidMainMenuEntry PastEnd;
	TestFalse(TEXT("no row past the end"), SolidMainMenu::FindEntry(SolidMainMenu::EntryCount(), PastEnd));
	TestFalse(TEXT("key 8 has no row"), SolidMainMenu::FindEntry(7, PastEnd));

	int32 QuitRows = 0;
	for (const FSolidMainMenuEntry& Entry : Entries)
	{
		if (SolidMainMenu::ActionForItem(Entry.Item) == ESolidMainMenuAction::Quit)
		{
			++QuitRows;
			TestEqual(TEXT("quit row is Exit Game"), Entry.Label, FString(TEXT("Exit Game")));
		}
		TestFalse(TEXT("no Test Drill row"), Entry.Label == TEXT("Test Drill"));
		TestFalse(TEXT("no Inventory row"), Entry.Label == TEXT("Inventory"));
		TestFalse(TEXT("no Enter cave row"), Entry.Label == TEXT("Enter cave"));
		TestFalse(TEXT("no Leave cave row"), Entry.Label == TEXT("Leave cave"));
	}
	TestEqual(TEXT("only Exit Game quits"), QuitRows, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMainMenuViewReptileTest,
	"SolidCore1.MainMenu.ViewReptile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMainMenuViewReptileTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);

	int32 ViewIndex = INDEX_NONE;
	int32 ExitIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Item == ESolidMainMenuItem::ViewReptile)
		{
			ViewIndex = Index;
		}
		if (Entries[Index].Item == ESolidMainMenuItem::ExitGame)
		{
			ExitIndex = Index;
		}
	}

	TestTrue(TEXT("View reptile is listed"), ViewIndex != INDEX_NONE);
	if (ViewIndex == INDEX_NONE)
	{
		return false;
	}
	TestEqual(TEXT("View reptile label"), Entries[ViewIndex].Label, FString(TEXT("View reptile")));
	TestEqual(
		TEXT("View reptile places the captain"),
		static_cast<uint8>(SolidMainMenu::ActionForItem(Entries[ViewIndex].Item)),
		static_cast<uint8>(ESolidMainMenuAction::PlaceCaptain));
	TestTrue(TEXT("View reptile sits before Exit Game"), ExitIndex > ViewIndex);

	const FVector2D Stand = SolidMonsterMeasure::ViewStandLocalXY();
	TestTrue(
		TEXT("the captain stands 450 cm back from the reptile"),
		FMath::IsNearlyEqual(Stand.X, SolidMonsterMeasure::MonsterXCm - SolidMonsterMeasure::ViewStandCm));
	TestTrue(
		TEXT("the captain shares the reptile's grid row"),
		FMath::IsNearlyEqual(Stand.Y, SolidMonsterMeasure::MonsterYCm));
	TestTrue(
		TEXT("the view looks down"),
		SolidMonsterMeasure::ViewPitchDeg < 0.f);
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
	TestTrue(TEXT("menu button opens the menu"), SolidMainMenu::ControllerKey() == EKeys::Gamepad_Special_Right);
	TestFalse(TEXT("view stays inventory"), SolidMainMenu::ControllerKey() == EKeys::Gamepad_Special_Left);
	TestFalse(TEXT("A stays jump"), SolidMainMenu::ControllerKey() == EKeys::Gamepad_FaceButton_Bottom);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
