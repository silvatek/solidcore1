#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidCaveInterior.h"
#include "SolidCaveLevel.h"
#include "SolidGameInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCaveLevelTravelTest,
	"SolidCore1.Stronghold.CaveTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCaveLevelTravelTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("companions stay out of the cave"), SolidCaveLevel::BringsCompanions());
	TestEqual(TEXT("enter label"), FString(SolidCaveLevel::MenuLabel(false)), FString(SolidCaveLevel::EnterLabel));
	TestEqual(TEXT("leave label"), FString(SolidCaveLevel::MenuLabel(true)), FString(SolidCaveLevel::LeaveLabel));

	TestTrue(TEXT("enter options request the cave"), SolidCaveLevel::RequestsCave(SolidCaveLevel::EnterOptions()));
	TestFalse(TEXT("leave options stay in the world"), SolidCaveLevel::RequestsCave(SolidCaveLevel::LeaveOptions()));
	TestFalse(TEXT("a plain play session is the open world"), SolidCaveLevel::RequestsCave(FString()));
	TestFalse(TEXT("another option is not the cave"), SolidCaveLevel::RequestsCave(TEXT("?SolidLevel=World")));
	TestTrue(TEXT("cave option with a trailer"), SolidCaveLevel::RequestsCave(TEXT("?SolidLevel=Cave?listen")));

	TestTrue(TEXT("captain spawn is inside the room"), SolidCaveLevel::ContainsLocal(SolidCaveLevel::CaptainSpawnLocal()));
	TestFalse(TEXT("past the mouth is outside"), SolidCaveLevel::ContainsLocal(FVector(-10.f, 0.f, 100.f)));
	TestFalse(TEXT("above the ceiling is outside"), SolidCaveLevel::ContainsLocal(FVector(400.f, 0.f, SolidCaveLevel::RoomHeightCm + 10.f)));

	TestTrue(
		TEXT("return yaw looks back at a north-facing mouth"),
		FMath::IsNearlyEqual(SolidCaveLevel::CaptainReturnYaw(90.f), -90.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCaveTravelSessionTest,
	"SolidCore1.Stronghold.CaveTravelSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCaveTravelSessionTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}

	TestFalse(TEXT("fresh instance is the open world"), Instance->IsInCave());
	TestFalse(TEXT("fresh instance stays at the start town"), Instance->WantsCaveMouthSpawn());

	FSolidTravelSession Enter;
	Enter.bInCave = true;
	Enter.bReturnToCaveMouth = false;
	Enter.SelectedCompanionIndices = { 0, 1 };
	Enter.ActiveBattlePlanSlot = 2;
	Enter.Sight = ESolidSight::Raven;
	Instance->Capture(Enter);

	TestTrue(TEXT("session kept"), Instance->HasSession());
	TestTrue(TEXT("captain is in the cave"), Instance->IsInCave());
	TestFalse(TEXT("cave trip does not spawn at the mouth yet"), Instance->WantsCaveMouthSpawn());
	TestEqual(TEXT("party selection kept"), Instance->GetSession().SelectedCompanionIndices.Num(), 2);
	TestEqual(TEXT("battle plan kept"), Instance->GetSession().ActiveBattlePlanSlot, 2);
	TestEqual(
		TEXT("sight kept"),
		static_cast<uint8>(Instance->GetSession().Sight),
		static_cast<uint8>(ESolidSight::Raven));

	FSolidTravelSession Leave;
	Leave.bInCave = false;
	Leave.bReturnToCaveMouth = true;
	Instance->Capture(Leave);
	TestFalse(TEXT("leaving the cave is the open world"), Instance->IsInCave());
	TestTrue(TEXT("leaving stands at the mouth"), Instance->WantsCaveMouthSpawn());

	Instance->ClearCaveMouthSpawn();
	TestFalse(TEXT("mouth spawn is used once"), Instance->WantsCaveMouthSpawn());
	TestTrue(TEXT("session remains after the mouth spawn"), Instance->HasSession());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCaveInteriorVisualsTest,
	"SolidCore1.Stronghold.CaveInteriorVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCaveInteriorVisualsTest::RunTest(const FString& Parameters)
{
	ASolidCaveInterior* Interior = NewObject<ASolidCaveInterior>();
	TestNotNull(TEXT("interior"), Interior);
	Interior->BuildVisuals();

	UStaticMeshComponent* Floor = Interior->GetFloorMesh();
	UStaticMeshComponent* Mouth = Interior->GetMouthMesh();
	UPointLightComponent* Light = Interior->GetRoomLight();
	TestNotNull(TEXT("floor"), Floor);
	TestNotNull(TEXT("mouth"), Mouth);
	TestNotNull(TEXT("light"), Light);
	if (!Floor || !Mouth || !Light)
	{
		return false;
	}

	UStaticMesh* FloorMesh = Floor->GetStaticMesh();
	UStaticMesh* MouthMesh = Mouth->GetStaticMesh();
	TestNotNull(TEXT("floor cube"), FloorMesh);
	TestNotNull(TEXT("mouth cube"), MouthMesh);
	TestTrue(TEXT("floor is under the walk"), Floor->GetRelativeLocation().Z < 0.f);
	TestTrue(TEXT("mouth is on the near wall"), Mouth->GetRelativeLocation().X < SolidCaveLevel::CaptainSpawnLocal().X);
	TestTrue(TEXT("light hangs inside"), Light->GetRelativeLocation().Z > 0.f && Light->GetRelativeLocation().Z < SolidCaveLevel::RoomHeightCm);
	TestTrue(TEXT("spawn feet are inside"), SolidCaveLevel::ContainsLocal(SolidCaveLevel::CaptainSpawnLocal()));
	TestTrue(TEXT("captain yaw faces the mouth"), FMath::IsNearlyEqual(Interior->GetCaptainYaw(), 180.f, 0.1f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
