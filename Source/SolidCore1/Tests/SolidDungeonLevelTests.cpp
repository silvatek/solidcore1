#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonInterior.h"
#include "SolidDungeonLevel.h"
#include "SolidGameInstance.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonLevelTravelTest,
	"SolidCore1.Stronghold.DungeonTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonLevelTravelTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("companions stay out of the cave"), SolidDungeonLevel::BringsCompanions());
	TestEqual(TEXT("enter label"), FString(SolidDungeonLevel::MenuLabel(false)), FString(SolidDungeonLevel::EnterLabel));
	TestEqual(TEXT("leave label"), FString(SolidDungeonLevel::MenuLabel(true)), FString(SolidDungeonLevel::LeaveLabel));

	TestTrue(TEXT("enter options request the cave"), SolidDungeonLevel::RequestsDungeon(SolidDungeonLevel::EnterOptions()));
	TestFalse(TEXT("leave options stay in the world"), SolidDungeonLevel::RequestsDungeon(SolidDungeonLevel::LeaveOptions()));
	TestFalse(TEXT("a plain play session is the open world"), SolidDungeonLevel::RequestsDungeon(FString()));
	TestFalse(TEXT("another option is not the cave"), SolidDungeonLevel::RequestsDungeon(TEXT("?SolidLevel=World")));
	TestTrue(TEXT("dungeon option with a trailer"), SolidDungeonLevel::RequestsDungeon(TEXT("?SolidLevel=Dungeon?listen")));

	TestTrue(TEXT("captain spawn is inside the room"), SolidDungeonLevel::ContainsLocal(SolidDungeonLevel::CaptainSpawnLocal()));
	TestFalse(TEXT("past the mouth is outside"), SolidDungeonLevel::ContainsLocal(SolidDungeonLevel::PastMouthLocal()));
	const FVector2D Notch = SolidStrongholdGrid::HexCenter(-1, 1, SolidStrongholdGrid::MeagreOrigin());
	TestFalse(TEXT("the open notch is outside"), SolidDungeonLevel::ContainsLocal(FVector(Notch.X, Notch.Y, 100.f)));
	TestFalse(TEXT("above the ceiling is outside"), SolidDungeonLevel::ContainsLocal(FVector(0.f, 0.f, SolidDungeonLevel::RoomHeightCm + 10.f)));

	TestTrue(
		TEXT("return yaw looks back at a north-facing mouth"),
		FMath::IsNearlyEqual(SolidDungeonLevel::CaptainReturnYaw(90.f), -90.f, 0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonTravelSessionTest,
	"SolidCore1.Stronghold.DungeonTravelSession",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonTravelSessionTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}

	TestFalse(TEXT("fresh instance is the open world"), Instance->IsInDungeon());
	TestFalse(TEXT("fresh instance stays at the start town"), Instance->WantsDungeonMouthSpawn());

	FSolidTravelSession Enter;
	Enter.bInDungeon = true;
	Enter.bReturnToDungeonMouth = false;
	Enter.SelectedCompanionIndices = { 0, 1 };
	Enter.ActiveBattlePlanSlot = 2;
	Enter.Sight = ESolidSight::Raven;
	Instance->Capture(Enter);

	TestTrue(TEXT("session kept"), Instance->HasSession());
	TestTrue(TEXT("captain is in the cave"), Instance->IsInDungeon());
	TestFalse(TEXT("cave trip does not spawn at the mouth yet"), Instance->WantsDungeonMouthSpawn());
	TestEqual(TEXT("party selection kept"), Instance->GetSession().SelectedCompanionIndices.Num(), 2);
	TestEqual(TEXT("battle plan kept"), Instance->GetSession().ActiveBattlePlanSlot, 2);
	TestEqual(
		TEXT("sight kept"),
		static_cast<uint8>(Instance->GetSession().Sight),
		static_cast<uint8>(ESolidSight::Raven));

	FSolidTravelSession Leave;
	Leave.bInDungeon = false;
	Leave.bReturnToDungeonMouth = true;
	Instance->Capture(Leave);
	TestFalse(TEXT("leaving the cave is the open world"), Instance->IsInDungeon());
	TestTrue(TEXT("leaving stands at the mouth"), Instance->WantsDungeonMouthSpawn());

	Instance->ClearDungeonMouthSpawn();
	TestFalse(TEXT("mouth spawn is used once"), Instance->WantsDungeonMouthSpawn());
	TestTrue(TEXT("session remains after the mouth spawn"), Instance->HasSession());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidDungeonInteriorVisualsTest,
	"SolidCore1.Stronghold.DungeonInteriorVisuals",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidDungeonInteriorVisualsTest::RunTest(const FString& Parameters)
{
	ASolidDungeonInterior* Interior = NewObject<ASolidDungeonInterior>();
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

	UStaticMesh* FloorAsset = Floor->GetStaticMesh();
	UStaticMesh* MouthAsset = Mouth->GetStaticMesh();
	TestNotNull(TEXT("floor mesh"), FloorAsset);
	TestNotNull(TEXT("mouth cube"), MouthAsset);
	if (FloorAsset)
	{
		const FBoxSphereBounds FloorBounds = FloorAsset->GetBounds();
		const float FloorTop = FloorBounds.Origin.Z + FloorBounds.BoxExtent.Z;
		const float FloorBottom = FloorBounds.Origin.Z - FloorBounds.BoxExtent.Z;
		TestTrue(TEXT("walk surface is the top of the floor"), FMath::IsNearlyZero(FloorTop, 1.f));
		TestTrue(TEXT("floor hangs below the walk"), FloorBottom < -10.f);
	}
	TestTrue(TEXT("mouth is on the near wall"), Mouth->GetRelativeLocation().X < SolidDungeonLevel::CaptainSpawnLocal().X);
	TestTrue(TEXT("light hangs inside"), Light->GetRelativeLocation().Z > 0.f && Light->GetRelativeLocation().Z < SolidDungeonLevel::RoomHeightCm);
	TestTrue(TEXT("spawn feet are inside"), SolidDungeonLevel::ContainsLocal(SolidDungeonLevel::CaptainSpawnLocal()));
	TestTrue(TEXT("captain yaw faces the mouth"), FMath::IsNearlyEqual(Interior->GetCaptainYaw(), 180.f, 0.1f));
	TestFalse(
		TEXT("room is not a template block"),
		ASolidDungeonInterior::StaticClass()->IsChildOf(AStaticMeshActor::StaticClass()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
