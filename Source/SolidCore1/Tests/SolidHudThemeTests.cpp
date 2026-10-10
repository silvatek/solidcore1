#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "HUD/SolidHudTheme.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidHudThemeOakFrameTest,
	"SolidCore1.HudTheme.OakAndBronze",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidHudThemeOakFrameTest::RunTest(const FString& Parameters)
{
	const SolidHudTheme::FFrame Menu = SolidHudTheme::MenuFrame();
	const SolidHudTheme::FFrame Hud = SolidHudTheme::HudFrame();
	const float MenuBand = SolidHudTheme::BandThickness(Menu);
	const float HudBand = SolidHudTheme::BandThickness(Hud);

	TestTrue(TEXT("menu band is the three strips"), FMath::IsNearlyEqual(MenuBand, Menu.Outer + Menu.Gap + Menu.Inner));
	TestTrue(TEXT("menu binding is thicker than the hud binding"), MenuBand > HudBand);
	TestTrue(TEXT("menu corner covers the outer band"), Menu.Corner > Menu.Outer);
	TestTrue(TEXT("hud corner covers the outer band"), Hud.Corner > Hud.Outer);
	TestTrue(TEXT("corners do not meet across a menu"), Menu.Corner * 2.f < 200.f);

	float InnerW = 0.f;
	float InnerH = 0.f;
	TestTrue(TEXT("menu board has an interior"), SolidHudTheme::InnerSize(220.f, 140.f, Menu, InnerW, InnerH));
	TestTrue(TEXT("interior is inset by the band"), FMath::IsNearlyEqual(InnerW, 220.f - MenuBand * 2.f));
	TestTrue(TEXT("interior height is inset by the band"), FMath::IsNearlyEqual(InnerH, 140.f - MenuBand * 2.f));

	float TinyW = 1.f;
	float TinyH = 1.f;
	TestFalse(TEXT("a box thinner than the binding has no board"), SolidHudTheme::InnerSize(MenuBand, 40.f, Menu, TinyW, TinyH));

	const SolidHudTheme::FPalette Palette = SolidHudTheme::OakAndBronze();
	TestTrue(TEXT("board is opaque"), FMath::IsNearlyEqual(Palette.Fill.A, 1.f));
	TestTrue(TEXT("bronze is opaque"), FMath::IsNearlyEqual(Palette.Bronze.A, 1.f));
	const float FillLum = Palette.Fill.R + Palette.Fill.G + Palette.Fill.B;
	const float BronzeLum = Palette.Bronze.R + Palette.Bronze.G + Palette.Bronze.B;
	const float BossLum = Palette.BronzeBright.R + Palette.BronzeBright.G + Palette.BronzeBright.B;
	TestTrue(TEXT("bronze is lighter than the oak"), BronzeLum > FillLum);
	TestTrue(TEXT("corner bosses are lighter than the band"), BossLum > BronzeLum);
	TestTrue(TEXT("oak is brown"), Palette.Fill.R > Palette.Fill.B);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
