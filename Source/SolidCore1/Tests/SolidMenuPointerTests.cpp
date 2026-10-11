#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidHUD.h"
#include "SolidMenuPointer.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMenuPointerRowsTest,
	"SolidCore1.MenuPointer.Rows",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMenuPointerRowsTest::RunTest(const FString& Parameters)
{
	SolidMenuPointer::FMetrics Metrics;
	Metrics.CanvasW = 800.f;
	Metrics.CanvasH = 600.f;
	Metrics.ContentW = 200.f;
	Metrics.LineHeight = 18.f;
	Metrics.ChoiceCount = 4;

	TestEqual(TEXT("title, blank, choices, blank, hint"), SolidMenuPointer::LineCount(4), 8);

	TArray<SolidMenuPointer::FRow> Rows;
	SolidMenuPointer::ChoiceRows(Metrics, Rows);
	TestEqual(TEXT("one row per choice"), Rows.Num(), 4);

	float BoxX = 0.f;
	float BoxY = 0.f;
	float BoxW = 0.f;
	float BoxH = 0.f;
	SolidMenuPointer::PanelBox(Metrics, BoxX, BoxY, BoxW, BoxH);
	TestTrue(TEXT("panel is centered"), FMath::IsNearlyEqual(BoxX, (800.f - BoxW) * 0.5f));
	TestTrue(TEXT("rows span the panel"), FMath::IsNearlyEqual(Rows[0].W, BoxW));

	const float Pitch = Metrics.LineHeight + Metrics.LineGap;
	TestTrue(TEXT("first choice follows the title and the blank line"),
		FMath::IsNearlyEqual(Rows[0].Y, SolidMenuPointer::LineY(BoxY, Metrics, SolidMenuPointer::LeadingLines)));
	TestTrue(TEXT("second choice is the next line"), FMath::IsNearlyEqual(Rows[1].Y, Rows[0].Y + Pitch));
	TestTrue(TEXT("rows meet without a gap"), FMath::IsNearlyEqual(Rows[0].H, Pitch));

	const float InsideX = Rows[0].X + 4.f;
	TestEqual(TEXT("first row"), SolidMenuPointer::HitChoice(Rows, InsideX, Rows[0].Y + 1.f), 0);
	TestEqual(TEXT("second row"), SolidMenuPointer::HitChoice(Rows, InsideX, Rows[1].Y + 1.f), 1);
	TestEqual(TEXT("title is not a choice"), SolidMenuPointer::HitChoice(Rows, InsideX, BoxY + Metrics.BoxPad + 1.f), INDEX_NONE);
	TestEqual(TEXT("outside the panel"), SolidMenuPointer::HitChoice(Rows, 0.f, 0.f), INDEX_NONE);
	TestEqual(TEXT("past the right edge"), SolidMenuPointer::HitChoice(Rows, Rows[0].X + Rows[0].W, Rows[0].Y + 1.f), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMenuPointerHudTest,
	"SolidCore1.MenuPointer.Hud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMenuPointerHudTest::RunTest(const FString& Parameters)
{
	SolidMenuPointer::FMetrics Metrics;
	Metrics.CanvasW = 800.f;
	Metrics.CanvasH = 600.f;
	Metrics.ContentW = 200.f;
	Metrics.LineHeight = 18.f;
	Metrics.ChoiceCount = 4;

	TArray<SolidMenuPointer::FRow> Rows;
	SolidMenuPointer::ChoiceRows(Metrics, Rows);

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	HUD->HandleMenuKey();
	HUD->SetChoicePointerRows(Rows);

	const float X = Rows[2].X + 8.f;
	const float Y = Rows[2].Y + 1.f;
	TestTrue(TEXT("hover selects the row"), HUD->SelectChoiceAt(X, Y));
	TestEqual(TEXT("third entry"), HUD->GetMainMenuIndex(), 2);
	TestTrue(TEXT("hover leaves the menu open"), HUD->IsMainMenuOpen());

	TestFalse(TEXT("a miss keeps the selection"), HUD->SelectChoiceAt(0.f, 0.f));
	TestEqual(TEXT("selection unchanged"), HUD->GetMainMenuIndex(), 2);

	HUD->OpenPortal();
	HUD->SetChoicePointerRows(Rows);
	TestTrue(TEXT("portal hover"), HUD->SelectChoiceAt(Rows[1].X + 8.f, Rows[1].Y + 1.f));
	TestEqual(TEXT("second town"), HUD->GetPortalIndex(), 1);
	TestTrue(TEXT("portal stays open"), HUD->IsPortalOpen());

	HUD->CloseMenuOverlay();
	HUD->SetChoicePointerRows(Rows);
	TestFalse(TEXT("a closed menu ignores the pointer"), HUD->SelectChoiceAt(X, Y));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
