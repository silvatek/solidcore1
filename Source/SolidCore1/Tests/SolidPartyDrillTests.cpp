#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Party/SolidPartyDrill.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartyDrillLegsTest,
	"SolidCore1.BattlePlan.FormationDrillLegs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartyDrillLegsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("leg count"), SolidPartyDrill::NumLegs, 4);
	TestEqual(TEXT("leg duration"), SolidPartyDrill::LegDurationSeconds, 1.5f);
	TestEqual(TEXT("turn yaw"), SolidPartyDrill::TurnYawDegrees, 90.f);

	TestEqual(TEXT("leg0 → F1"), SolidPartyDrill::PlanSlotForLeg(0), 0);
	TestEqual(TEXT("leg1 → F2"), SolidPartyDrill::PlanSlotForLeg(1), 1);
	TestEqual(TEXT("leg2 → F3"), SolidPartyDrill::PlanSlotForLeg(2), 2);
	TestEqual(TEXT("leg3 → F4"), SolidPartyDrill::PlanSlotForLeg(3), 3);

	TestFalse(TEXT("F1 no turn"), SolidPartyDrill::TurnsBeforeWalk(0));
	TestTrue(TEXT("F2 turns"), SolidPartyDrill::TurnsBeforeWalk(1));
	TestTrue(TEXT("F3 turns"), SolidPartyDrill::TurnsBeforeWalk(2));
	TestTrue(TEXT("F4 turns"), SolidPartyDrill::TurnsBeforeWalk(3));

	// Four sides × 90° = full square (three turns after the opening leg).
	int32 Turns = 0;
	for (int32 Leg = 0; Leg < SolidPartyDrill::NumLegs; ++Leg)
	{
		if (SolidPartyDrill::TurnsBeforeWalk(Leg))
		{
			++Turns;
		}
	}
	TestEqual(TEXT("three clockwise turns"), Turns, 3);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
