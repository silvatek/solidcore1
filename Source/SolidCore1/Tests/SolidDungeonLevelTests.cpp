#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidGameInstance.h"
#include "SolidTravel.h"
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
	TestEqual(TEXT("enter label"), FString(SolidDungeonLevel::MenuLabel(false)), FString(SolidDungeonLevel::EnterLabel));
	TestEqual(TEXT("leave label"), FString(SolidDungeonLevel::MenuLabel(true)), FString(SolidDungeonLevel::LeaveLabel));

	TestTrue(TEXT("enter options request the stronghold"), SolidTravel::RequestsStronghold(SolidTravel::EnterStrongholdOptions()));
	TestFalse(TEXT("leave options stay in the world"), SolidTravel::RequestsStronghold(SolidTravel::LeaveOptions()));
	TestFalse(TEXT("a plain play session is the open world"), SolidTravel::RequestsStronghold(FString()));
	TestFalse(TEXT("another option is not the stronghold"), SolidTravel::RequestsStronghold(TEXT("?SolidLevel=World")));
	TestTrue(TEXT("stronghold option with a trailer"), SolidTravel::RequestsStronghold(TEXT("?SolidLevel=Stronghold?listen")));

	TestTrue(TEXT("captain spawn is inside the room"), SolidDungeonLevel::ContainsLocal(SolidDungeonLevel::CaptainSpawnLocal()));
	const FVector2D Notch = SolidStrongholdGrid::HexCenter(-2, 1, SolidDungeonLevel::MeagreOrigin());
	TestFalse(TEXT("the open notch is outside"), SolidDungeonLevel::ContainsLocal(FVector(Notch.X, Notch.Y, 100.f)));
	TestTrue(TEXT("the doorway is wider than the captain"), SolidDungeonLevel::DoorwayWidthCm > 110.f);
	TestFalse(TEXT("above the ceiling is outside"), SolidDungeonLevel::ContainsLocal(FVector(0.f, 0.f, SolidDungeonLevel::RoomHeightCm + 10.f)));

	TArray<SolidStrongholdGrid::FCell> Cells;
	SolidDungeonLevel::MeagreCells(Cells);
	TestEqual(TEXT("the Meagre floor is fourteen hexes"), Cells.Num(), 14);

	TArray<SolidStrongholdGrid::FHex> Hexes;
	SolidDungeonLevel::CollectMeagreHexes(Hexes);
	TestEqual(TEXT("each cell has a center"), Hexes.Num(), Cells.Num());

	TMap<int32, int32> RowCounts;
	bool bHasCaptain = false;
	bool bHasEntrance = false;
	bool bHasNotch = false;
	const SolidStrongholdGrid::FCell Captain = SolidDungeonLevel::MeagreCaptainCell();
	const SolidStrongholdGrid::FCell Entrance = SolidDungeonLevel::MeagreEntranceCell();
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		RowCounts.FindOrAdd(Hex.Row) += 1;
		bHasCaptain |= Hex.Col == Captain.Col && Hex.Row == Captain.Row;
		bHasEntrance |= Hex.Col == Entrance.Col && Hex.Row == Entrance.Row;
		bHasNotch |= Hex.Col == -2 && Hex.Row == 1;
		TestTrue(TEXT("cell center is inside its hex"), SolidStrongholdGrid::PointInHex(Hex.Center, Hex.Center));
	}
	TestTrue(TEXT("captain has a cell"), bHasCaptain);
	TestTrue(TEXT("entrance has a cell"), bHasEntrance);
	TestFalse(TEXT("the north-west notch is open"), bHasNotch);

	const FVector2D Origin = SolidDungeonLevel::MeagreOrigin();
	const FVector2D EntranceCenter = SolidStrongholdGrid::HexCenter(Entrance.Col, Entrance.Row, Origin);
	const FVector2D CaptainCenter = SolidStrongholdGrid::HexCenter(Captain.Col, Captain.Row, Origin);
	TestTrue(
		TEXT("entrance is the west neighbor of the captain"),
		EntranceCenter.X < CaptainCenter.X
			&& FMath::IsNearlyEqual(
				FVector2D::Distance(EntranceCenter, CaptainCenter),
				SolidStrongholdGrid::FlatToFlatCm,
				0.05f));
	const FVector2D Mouth = SolidDungeonLevel::MeagreMouthMidpoint();
	TestTrue(
		TEXT("mouth is the west flat of the entrance"),
		FMath::IsNearlyEqual(Mouth.X, EntranceCenter.X - SolidStrongholdGrid::FlatToFlatCm * 0.5f, 0.05f)
			&& FMath::IsNearlyEqual(Mouth.Y, EntranceCenter.Y, 0.05f));

	int32 NarrowestRow = MAX_int32;
	int32 WidestRow = 0;
	for (const TPair<int32, int32>& Row : RowCounts)
	{
		NarrowestRow = FMath::Min(NarrowestRow, Row.Value);
		WidestRow = FMath::Max(WidestRow, Row.Value);
	}
	TestTrue(TEXT("the outline is not a rectangle"), NarrowestRow > 0 && NarrowestRow != WidestRow);

	bool bNotchInside = false;
	for (const SolidStrongholdGrid::FHex& Hex : Hexes)
	{
		bNotchInside |= SolidStrongholdGrid::PointInHex(Notch, Hex.Center);
	}
	TestFalse(TEXT("notch center is outside every cell"), bNotchInside);
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

	TestFalse(TEXT("fresh instance is the open world"), Instance->IsInStronghold());
	TestFalse(TEXT("fresh instance stays at the start town"), Instance->WantsStrongholdExit());

	FSolidTravelSession Enter;
	Enter.bInStronghold = true;
	Enter.bExitStronghold = false;
	Enter.SelectedCompanionIndices = { 0, 1 };
	Enter.ActiveBattlePlanSlot = 2;
	Enter.Sight = ESolidSight::Raven;
	Instance->Capture(Enter);

	TestTrue(TEXT("session kept"), Instance->HasSession());
	TestTrue(TEXT("captain is in the stronghold"), Instance->IsInStronghold());
	TestFalse(TEXT("the trip does not stand outside yet"), Instance->WantsStrongholdExit());
	TestEqual(TEXT("squad selection kept"), Instance->GetSession().SelectedCompanionIndices.Num(), 2);
	TestEqual(TEXT("battle plan kept"), Instance->GetSession().ActiveBattlePlanSlot, 2);
	TestEqual(
		TEXT("sight kept"),
		static_cast<uint8>(Instance->GetSession().Sight),
		static_cast<uint8>(ESolidSight::Raven));

	FSolidTravelSession Leave;
	Leave.bInStronghold = false;
	Leave.bExitStronghold = true;
	Instance->Capture(Leave);
	TestFalse(TEXT("leaving the stronghold is the open world"), Instance->IsInStronghold());
	TestTrue(TEXT("leaving stands outside the entrance"), Instance->WantsStrongholdExit());

	Instance->ClearStrongholdExit();
	TestFalse(TEXT("the outside stand is used once"), Instance->WantsStrongholdExit());
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
	const FVector2D MouthPlan = SolidDungeonLevel::MeagreMouthMidpoint();
	TestTrue(TEXT("lip sits outside the entrance flat"), Mouth->GetRelativeLocation().X < MouthPlan.X - 1.f);
	TestTrue(
		TEXT("doorway is wider than the captain"),
		Mouth->GetRelativeScale3D().X * 100.f > 110.f);
	TestTrue(TEXT("the lip is a thin panel"), Mouth->GetRelativeScale3D().Y * 100.f < 20.f);
	TestTrue(TEXT("light hangs inside"), Light->GetRelativeLocation().Z > 0.f && Light->GetRelativeLocation().Z < SolidDungeonLevel::RoomHeightCm);
	TestTrue(TEXT("spawn feet are inside"), SolidDungeonLevel::ContainsLocal(SolidDungeonLevel::CaptainSpawnLocal()));
	TestTrue(TEXT("captain yaw faces into the cave"), FMath::IsNearlyEqual(Interior->GetCaptainYaw(), 0.f, 0.1f));
	TestTrue(TEXT("arrival camera is level"), FMath::IsNearlyZero(Interior->GetCaptainView().Pitch, 0.1f));
	TestFalse(
		TEXT("room is not a template block"),
		ASolidDungeonInterior::StaticClass()->IsChildOf(AStaticMeshActor::StaticClass()));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
