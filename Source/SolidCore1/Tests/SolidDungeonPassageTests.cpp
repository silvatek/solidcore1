#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonEntrance.h"
#include "SolidDungeonInterior.h"
#include "SolidDungeonLevel.h"
#include "SolidDungeonPassage.h"
#include "HUD/SolidHUD.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonPassageHitTest,
	"SolidCore1.Stronghold.CavePassage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonPassageHitTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("the opening trace uses simple collision"), SolidDungeonPassage::bTraceComplexCollision);
	TestTrue(TEXT("a shown pointer can use the opening"), SolidDungeonPassage::AcceptsPointer(true, false));
	TestFalse(TEXT("the menu and the drill take the pointer"), SolidDungeonPassage::AcceptsPointer(true, true));
	TestFalse(TEXT("a hidden pointer ignores the opening"), SolidDungeonPassage::AcceptsPointer(false, false));
	TestFalse(TEXT("hidden during a menu ignores the opening"), SolidDungeonPassage::AcceptsPointer(false, true));

	TestEqual(TEXT("enter tip"), FString(SolidDungeonPassage::TipText(SolidDungeonPassage::EHit::Enter)), FString(SolidDungeonLevel::EnterLabel));
	TestEqual(TEXT("leave tip"), FString(SolidDungeonPassage::TipText(SolidDungeonPassage::EHit::Leave)), FString(SolidDungeonLevel::LeaveLabel));
	TestTrue(TEXT("a miss has no tip"), FString(SolidDungeonPassage::TipText(SolidDungeonPassage::EHit::None)).IsEmpty());

	ASolidDungeonEntrance* Entrance = NewObject<ASolidDungeonEntrance>();
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
	TestTrue(TEXT("the outside opening enters"), SolidDungeonPassage::Classify(Entrance, Mouth) == SolidDungeonPassage::EHit::Enter);
	TestTrue(TEXT("a jamb is not the opening"), SolidDungeonPassage::Classify(Entrance, Jamb) == SolidDungeonPassage::EHit::None);
	TestTrue(TEXT("the lintel is not the opening"), SolidDungeonPassage::Classify(Entrance, Lintel) == SolidDungeonPassage::EHit::None);

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
	TestTrue(TEXT("the inside opening leaves"), SolidDungeonPassage::Classify(Interior, InnerMouth) == SolidDungeonPassage::EHit::Leave);
	TestTrue(TEXT("the floor is not the opening"), SolidDungeonPassage::Classify(Interior, Floor) == SolidDungeonPassage::EHit::None);
	TestTrue(TEXT("the outside mouth is not an exit"), SolidDungeonPassage::Classify(Interior, Mouth) == SolidDungeonPassage::EHit::None);
	TestTrue(TEXT("a null hit is nothing"), SolidDungeonPassage::Classify(nullptr, Mouth) == SolidDungeonPassage::EHit::None);

	const FVector2D Hotspot(100.f, 120.f);
	const SolidDungeonPassage::FTipBox Beside = SolidDungeonPassage::PlaceTip(Hotspot, 90.f, 28.f, 800.f, 600.f);
	TestTrue(TEXT("the tip sits to the right of the pointer"), Beside.X >= Hotspot.X + SolidDungeonPassage::PointerClearance - 0.1f);
	TestFalse(TEXT("the tip does not cover the hotspot"), SolidDungeonPassage::Contains(Beside, Hotspot));

	const FVector2D RightEdge(760.f, 100.f);
	const SolidDungeonPassage::FTipBox Flipped = SolidDungeonPassage::PlaceTip(RightEdge, 80.f, 30.f, 800.f, 600.f);
	TestTrue(TEXT("the tip stays on screen at the right edge"), Flipped.X + Flipped.W <= 800.f);
	TestTrue(TEXT("the tip flips left of a pointer at the right edge"), Flipped.X + Flipped.W <= RightEdge.X);
	TestFalse(TEXT("a flipped tip does not cover the hotspot"), SolidDungeonPassage::Contains(Flipped, RightEdge));

	const FVector2D BottomEdge(100.f, 580.f);
	const SolidDungeonPassage::FTipBox Raised = SolidDungeonPassage::PlaceTip(BottomEdge, 80.f, 30.f, 800.f, 600.f);
	TestTrue(TEXT("the tip stays on screen at the bottom edge"), Raised.Y + Raised.H <= 600.f);
	TestFalse(TEXT("a raised tip does not cover the hotspot"), SolidDungeonPassage::Contains(Raised, BottomEdge));

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
