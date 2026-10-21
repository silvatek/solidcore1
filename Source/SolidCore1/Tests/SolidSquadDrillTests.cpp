#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidSquadDrill.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadDrillLegsTest,
	"SolidCore1.BattlePlan.FormationDrillLegs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadDrillLegsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("leg count"), SolidSquadDrill::NumLegs, 4);
	TestEqual(TEXT("leg duration"), SolidSquadDrill::LegDurationSeconds, 1.5f);
	TestEqual(TEXT("turn duration"), SolidSquadDrill::TurnDurationSeconds, 0.25f);
	TestEqual(TEXT("turn yaw"), SolidSquadDrill::TurnYawDegrees, 90.f);

	TestEqual(TEXT("leg0 → F1"), SolidSquadDrill::PlanSlotForLeg(0), 0);
	TestEqual(TEXT("leg1 → F2"), SolidSquadDrill::PlanSlotForLeg(1), 1);
	TestEqual(TEXT("leg2 → F3"), SolidSquadDrill::PlanSlotForLeg(2), 2);
	TestEqual(TEXT("leg3 → F4"), SolidSquadDrill::PlanSlotForLeg(3), 3);

	TestFalse(TEXT("F1 no turn"), SolidSquadDrill::TurnsBeforeWalk(0));
	TestTrue(TEXT("F2 turns"), SolidSquadDrill::TurnsBeforeWalk(1));
	TestTrue(TEXT("F3 turns"), SolidSquadDrill::TurnsBeforeWalk(2));
	TestTrue(TEXT("F4 turns"), SolidSquadDrill::TurnsBeforeWalk(3));

	// Four sides × 90° = full square (three turns after the opening leg).
	int32 Turns = 0;
	for (int32 Leg = 0; Leg < SolidSquadDrill::NumLegs; ++Leg)
	{
		if (SolidSquadDrill::TurnsBeforeWalk(Leg))
		{
			++Turns;
		}
	}
	TestEqual(TEXT("three clockwise turns"), Turns, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
