#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Menus/SolidExit.h"
#include "Menus/SolidMainMenu.h"
#include "Menus/SolidMenuPointer.h"
#include "SolidHUD.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidExitMenuTest,
	"SolidCore1.Exit.Menu",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidExitMenuTest::RunTest(const FString& Parameters)
{
	TArray<FSolidMainMenuEntry> Entries;
	SolidMainMenu::CollectEntries(Entries);
	int32 ExitIndex = INDEX_NONE;
	int32 CreditsIndex = INDEX_NONE;
	int32 QuitRows = 0;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].Label == TEXT("Exit Game"))
		{
			ExitIndex = Index;
		}
		if (Entries[Index].Item == ESolidMainMenuItem::Credits)
		{
			CreditsIndex = Index;
		}
		if (SolidMainMenu::ActionForItem(Entries[Index].Item) == ESolidMainMenuAction::Quit)
		{
			++QuitRows;
		}
	}

	TestTrue(TEXT("Exit Game is listed"), ExitIndex != INDEX_NONE);
	TestEqual(TEXT("Exit Game is last"), ExitIndex, Entries.Num() - 1);
	TestEqual(TEXT("Credits is the row before Exit Game"), CreditsIndex, ExitIndex - 1);
	TestEqual(TEXT("only Exit Game quits"), QuitRows, 1);
	if (ExitIndex == INDEX_NONE)
	{
		return false;
	}

	TestEqual(
		TEXT("exit item"),
		static_cast<uint8>(Entries[ExitIndex].Item),
		static_cast<uint8>(ESolidMainMenuItem::ExitGame));
	TestEqual(TEXT("prompt"), FString(SolidExit::Prompt()), FString(TEXT("Exit the game?")));
	TestEqual(TEXT("yes"), FString(SolidExit::Label(SolidExit::YesIndex)), FString(TEXT("Yes")));
	TestEqual(TEXT("no"), FString(SolidExit::Label(SolidExit::NoIndex)), FString(TEXT("No")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidExitRequestTest,
	"SolidCore1.Exit.Request",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidExitRequestTest::RunTest(const FString& Parameters)
{
	const SolidExit::FRequest Pie = SolidExit::RequestForYes(true, true);
	TestTrue(TEXT("a PIE yes asks to quit"), Pie.bQuitGame);
	TestTrue(TEXT("a PIE yes also asks to close the editor"), Pie.bCloseEditor);

	const SolidExit::FRequest PieBlocked = SolidExit::RequestForYes(true, false);
	TestTrue(TEXT("a PIE yes still asks to quit when the editor cannot close"), PieBlocked.bQuitGame);
	TestFalse(TEXT("a blocked editor is not closed"), PieBlocked.bCloseEditor);

	const SolidExit::FRequest Game = SolidExit::RequestForYes(false, true);
	TestTrue(TEXT("a game outside the editor asks to quit"), Game.bQuitGame);
	TestFalse(TEXT("a game outside the editor does not close the editor"), Game.bCloseEditor);

	const SolidExit::FRequest Plain = SolidExit::RequestForYes(false, false);
	TestTrue(TEXT("yes asks to quit"), Plain.bQuitGame);
	TestFalse(TEXT("yes does not close the editor"), Plain.bCloseEditor);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidExitConfirmTest,
	"SolidCore1.Exit.Confirm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidExitConfirmTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	TestFalse(TEXT("confirmation starts closed"), HUD->IsExitConfirmOpen());
	HUD->HandleMenuKey();
	TestTrue(TEXT("menu opens"), HUD->IsMainMenuOpen());
	HUD->OpenExitConfirm();
	TestTrue(TEXT("Exit Game asks"), HUD->IsExitConfirmOpen());
	TestFalse(TEXT("the menu is not left open"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("the confirmation is not a message"), HUD->IsMessageVisible());
	TestTrue(TEXT("the journal is not written"), HUD->GetMessageId().IsEmpty());
	TestEqual(TEXT("Yes is selected"), HUD->GetExitConfirmIndex(), SolidExit::YesIndex);

	HUD->MoveExitConfirm(1);
	TestEqual(TEXT("down selects No"), HUD->GetExitConfirmIndex(), SolidExit::NoIndex);
	HUD->MoveExitConfirm(1);
	TestEqual(TEXT("down wraps to Yes"), HUD->GetExitConfirmIndex(), SolidExit::YesIndex);
	HUD->MoveExitConfirm(-1);
	TestEqual(TEXT("up wraps to No"), HUD->GetExitConfirmIndex(), SolidExit::NoIndex);
	HUD->MoveExitConfirm(-1);
	TestEqual(TEXT("up wraps to Yes"), HUD->GetExitConfirmIndex(), SolidExit::YesIndex);

	SolidMenuPointer::FMetrics Metrics;
	Metrics.CanvasW = 800.f;
	Metrics.CanvasH = 600.f;
	Metrics.ContentW = 200.f;
	Metrics.LineHeight = 18.f;
	Metrics.ChoiceCount = SolidExit::ChoiceCount;
	TArray<SolidMenuPointer::FRow> Rows;
	SolidMenuPointer::ChoiceRows(Metrics, Rows);
	HUD->SetChoicePointerRows(Rows);
	TestTrue(TEXT("a click selects No"), HUD->SelectChoiceAt(Rows[1].X + 8.f, Rows[1].Y + 1.f));
	TestEqual(TEXT("No is selected"), HUD->GetExitConfirmIndex(), SolidExit::NoIndex);
	TestFalse(TEXT("No leaves the game running"), HUD->AcceptExitConfirm());
	TestFalse(TEXT("No closes the confirmation"), HUD->IsExitConfirmOpen());
	TestFalse(TEXT("No does not open the menu"), HUD->IsMainMenuOpen());
	TestFalse(TEXT("No writes no message"), HUD->IsMessageVisible());

	HUD->OpenExitConfirm();
	HUD->SetExitConfirmIndex(SolidExit::YesIndex);
	TestTrue(TEXT("key 1 confirms Yes"), HUD->AcceptExitConfirm());
	TestFalse(TEXT("Yes closes the confirmation"), HUD->IsExitConfirmOpen());
	TestFalse(TEXT("Yes writes no message"), HUD->IsMessageVisible());

	HUD->OpenExitConfirm();
	HUD->SetExitConfirmIndex(SolidExit::NoIndex);
	TestFalse(TEXT("key 2 confirms No"), HUD->AcceptExitConfirm());

	HUD->OpenExitConfirm();
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc leaves the game running"), HUD->IsExitConfirmOpen());
	TestFalse(TEXT("Esc does not open the menu"), HUD->IsMainMenuOpen());

	HUD->OpenExitConfirm();
	HUD->HandleMenuKey();
	TestFalse(TEXT("the menu key leaves the game running"), HUD->IsExitConfirmOpen());
	TestFalse(TEXT("the menu key does not open the menu"), HUD->IsMainMenuOpen());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
