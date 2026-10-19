#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Lairs/SolidLairEntrance.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "HUD/SolidPointer.h"
#include "HUD/SolidHUD.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonPassageHitTest,
	"SolidCore1.Stronghold.CavePassage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonPassageHitTest::RunTest(const FString& Parameters)
{
	const FBox Door(FVector(-50.f, -40.f, 0.f), FVector(50.f, 40.f, 80.f));
	TestTrue(TEXT("a ray through the opening hits"), SolidPointer::RayHitsBox(
		FVector(-200.f, 0.f, 40.f), FVector(1.f, 0.f, 0.f), Door, 1000.f));
	TestFalse(TEXT("a ray past the opening misses"), SolidPointer::RayHitsBox(
		FVector(-200.f, 200.f, 40.f), FVector(1.f, 0.f, 0.f), Door, 1000.f));
	TestTrue(TEXT("a point inside the opening is a hit"), SolidPointer::RayHitsBox(
		FVector(0.f, 0.f, 40.f), FVector(0.f, 1.f, 0.f), Door, 100.f));
	TestFalse(TEXT("a short ray stops before the opening"), SolidPointer::RayHitsBox(
		FVector(-200.f, 0.f, 40.f), FVector(1.f, 0.f, 0.f), Door, 100.f));

	TestTrue(TEXT("a shown pointer can use the opening"), SolidPointer::AcceptsPointer(true, false));
	TestFalse(TEXT("the menu and the drill take the pointer"), SolidPointer::AcceptsPointer(true, true));
	TestFalse(TEXT("a hidden pointer ignores the opening"), SolidPointer::AcceptsPointer(false, false));
	TestFalse(TEXT("hidden during a menu ignores the opening"), SolidPointer::AcceptsPointer(false, true));

	ASolidLairEntrance* Entrance = NewObject<ASolidLairEntrance>();
	TestNotNull(TEXT("entrance"), Entrance);
	if (!Entrance)
	{
		return false;
	}
	Entrance->BuildVisuals();
	UStaticMeshComponent* Mouth = Entrance->GetMouthMesh();
	UStaticMeshComponent* Jamb = Entrance->GetLeftJamb();
	UStaticMeshComponent* Lintel = Entrance->GetLintel();
	TestNotNull(TEXT("outside mouth"), Mouth);
	TestNotNull(TEXT("jamb"), Jamb);
	TestNotNull(TEXT("lintel"), Lintel);
	if (!Mouth || !Jamb || !Lintel)
	{
		return false;
	}
	ASolidDungeonInterior* Interior = NewObject<ASolidDungeonInterior>();
	TestNotNull(TEXT("interior"), Interior);
	if (!Interior)
	{
		return false;
	}
	Interior->BuildVisuals();
	UStaticMeshComponent* InnerMouth = Interior->GetMouthMesh();
	UStaticMeshComponent* Floor = Interior->GetFloorMesh();
	TestNotNull(TEXT("inside mouth"), InnerMouth);
	TestNotNull(TEXT("floor"), Floor);
	if (!InnerMouth || !Floor)
	{
		return false;
	}

	const FVector2D Hotspot(100.f, 120.f);
	const SolidPointer::FTipBox Beside = SolidPointer::PlaceTip(Hotspot, 90.f, 28.f, 800.f, 600.f);
	TestTrue(TEXT("the tip sits to the right of the pointer"), Beside.X >= Hotspot.X + SolidPointer::PointerClearance - 0.1f);
	TestFalse(TEXT("the tip does not cover the hotspot"), SolidPointer::Contains(Beside, Hotspot));

	const FVector2D RightEdge(760.f, 100.f);
	const SolidPointer::FTipBox Flipped = SolidPointer::PlaceTip(RightEdge, 80.f, 30.f, 800.f, 600.f);
	TestTrue(TEXT("the tip stays on screen at the right edge"), Flipped.X + Flipped.W <= 800.f);
	TestTrue(TEXT("the tip flips left of a pointer at the right edge"), Flipped.X + Flipped.W <= RightEdge.X);
	TestFalse(TEXT("a flipped tip does not cover the hotspot"), SolidPointer::Contains(Flipped, RightEdge));

	const FVector2D BottomEdge(100.f, 580.f);
	const SolidPointer::FTipBox Raised = SolidPointer::PlaceTip(BottomEdge, 80.f, 30.f, 800.f, 600.f);
	TestTrue(TEXT("the tip stays on screen at the bottom edge"), Raised.Y + Raised.H <= 600.f);
	TestFalse(TEXT("a raised tip does not cover the hotspot"), SolidPointer::Contains(Raised, BottomEdge));

	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}
	HUD->SetCavePassageTip(SolidDungeonLevel::EnterLabel);
	TestEqual(TEXT("hud keeps the enter tip"), HUD->GetCavePassageTip(), FString(SolidDungeonLevel::EnterLabel));
	HUD->SetCavePassageTip(FString());
	TestTrue(TEXT("an empty tip clears"), HUD->GetCavePassageTip().IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
