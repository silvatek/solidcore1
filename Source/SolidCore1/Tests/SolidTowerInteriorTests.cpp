#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "SolidStronghold.h"
#include "SolidGameInstance.h"
#include "SolidTravel.h"
#include "SolidLair.h"
#include "Strongholds/Tower/Actors/SolidTowerInterior.h"
#include "Strongholds/Tower/Actors/SolidTowerExterior.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidTowerInteriorTestPrivate
{
	float MaxRadius(UStaticMesh* Mesh)
	{
		float Radius = 0.f;
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return -1.f;
		}
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			Radius = FMath::Max(Radius, FVector2D(Position.X, Position.Y).Size());
		}
		return Radius;
	}

	int32 ExpectedGridCount(const SolidStronghold::FSolidTower& Tower)
	{
		const SolidStronghold::FPlan& Plan = Tower.Plan();
		int32 Count = 0;
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			TArray<SolidStrongholdGrid::FHex> Hexes;
			TArray<SolidStrongholdGrid::FSegment> Segments;
			Tower.CollectFloorHexes(Floor, Hexes);
			SolidStrongholdGrid::CollectSegments(Hexes, Segments);
			for (const SolidStrongholdGrid::FSegment& Segment : Segments)
			{
				if ((Segment.B - Segment.A).Size() >= 1.f)
				{
					++Count;
				}
			}
		}
		return Count;
	}

	bool SpawnIsKeptHex(const SolidStronghold::FPlan& Plan, const FVector& Spawn)
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidStronghold::CollectKeptHexes(Plan, 0, Hexes);
		const FVector2D At(Spawn.X, Spawn.Y);
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (Hex.Center.Equals(At, 0.1f))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerTravelTest,
	"SolidCore1.Stronghold.TowerTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerTravelTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("stronghold options request the visit"), SolidTravel::RequestsStronghold(SolidTravel::EnterStrongholdOptions()));
	TestFalse(TEXT("leave options stay in the world"), SolidTravel::RequestsStronghold(SolidTravel::LeaveOptions()));
	TestFalse(TEXT("an empty url is the open world"), SolidTravel::RequestsStronghold(FString()));
	TestTrue(TEXT("a stronghold option with a trailer"), SolidTravel::RequestsStronghold(TEXT("?SolidLevel=Stronghold?listen")));

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}

	TestFalse(TEXT("fresh instance is the open world"), Instance->IsInStronghold());
	TestFalse(TEXT("fresh instance does not stand outside"), Instance->WantsStrongholdExit());

	FSolidTravelSession Inside;
	Inside.bInStronghold = true;
	Inside.bExitStronghold = false;
	Instance->Capture(Inside);
	TestTrue(TEXT("a stronghold session is inside"), Instance->IsInStronghold());
	TestFalse(TEXT("a stronghold session does not stand outside yet"), Instance->WantsStrongholdExit());

	FSolidTravelSession Leave;
	Leave.bInStronghold = false;
	Leave.bExitStronghold = true;
	Instance->Capture(Leave);
	TestTrue(TEXT("leaving stands outside the entrance"), Instance->WantsStrongholdExit());
	TestFalse(TEXT("leaving the stronghold is the open world"), Instance->IsInStronghold());
	Instance->ClearStrongholdExit();
	TestFalse(TEXT("the outside stand is used once"), Instance->WantsStrongholdExit());

	const FIntPoint DoorCell(16, 30);
	const SolidLair::FPassageHit TowerEnter = SolidLair::HitOnStronghold(DoorCell, false);
	const SolidLair::FPassageHit TowerLeave = SolidLair::HitOnStronghold(DoorCell, true);
	const SolidLair::FPassageHit CaveEnter = SolidLair::HitOnStronghold(SolidStronghold::StartingCell(), false);
	TestEqual(TEXT("a tower enter logs Enter stronghold"), FString(SolidLair::PassageTip(TowerEnter)), FString(TEXT("Enter stronghold")));
	TestEqual(TEXT("a tower leave logs Exit stronghold"), FString(SolidLair::PassageTip(TowerLeave)), FString(TEXT("Exit stronghold")));
	TestEqual(TEXT("a cave enter logs Enter stronghold"), FString(SolidLair::PassageTip(CaveEnter)), FString(TEXT("Enter stronghold")));
	const SolidLair::FPassageHit CaveLeaveHit = SolidLair::HitOnStronghold(SolidStronghold::StartingCell(), true);
	TestEqual(TEXT("a cave leave logs Exit stronghold"), FString(SolidLair::PassageTip(CaveLeaveHit)), FString(TEXT("Exit stronghold")));
	TestEqual(
		TEXT("a shut cave still lets the tower door through the tunnel gate"),
		static_cast<uint8>(SolidLair::AllowPassage(TowerEnter, false).Action),
		static_cast<uint8>(SolidLair::EHit::Enter));
	TestEqual(
		TEXT("that click keeps the cell"),
		SolidLair::AllowPassage(TowerEnter, false).StrongholdCell.X,
		static_cast<int32>(16));
	FGameState Closed;
	TestFalse(
		TEXT("a shut stronghold refuses the cave door"),
		Closed.ActiveStronghold->AcceptsEnter(Closed, Closed.ActiveStronghold->Cell));
	FGameState TowerState;
	TowerState.ActiveStronghold = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		DoorCell,
		SolidStronghold::WorldXY(DoorCell));
	TestFalse(
		TEXT("a shut stronghold refuses the tower door"),
		TowerState.ActiveStronghold->AcceptsEnter(TowerState, DoorCell));
	TowerState.bStrongholdsEnabled = true;
	TestTrue(
		TEXT("the active tower accepts enter once strongholds are open"),
		TowerState.ActiveStronghold->AcceptsEnter(TowerState, DoorCell));

	ASolidTowerExterior* Shell = NewObject<ASolidTowerExterior>();
	TestNotNull(TEXT("shell"), Shell);
	if (Shell)
	{
		Shell->SetStrongholdCell(DoorCell);
		TestEqual(TEXT("shell remembers its cell"), Shell->GetStrongholdCell().X, static_cast<int32>(16));
		TestEqual(TEXT("shell remembers its column"), Shell->GetStrongholdCell().Y, static_cast<int32>(30));
	}

	const FVector2D Cell(1000.f, 2000.f);
	SolidStronghold::FSolidTower ImperiousTower;
	ImperiousTower.Size = ESolidStrongholdSize::Imperious;
	ImperiousTower.BindPlan();
	const FVector2D Stand = ImperiousTower.OutsideDoorXY(Cell, 90.f);
	const float Clearance = ImperiousTower.Plan().GroundExteriorRadiusCm
		+ SolidStronghold::DoorProudCm
		+ SolidStronghold::DoorStandClearCm;
	TestTrue(TEXT("north grass stands north of the cell"), FMath::IsNearlyEqual(Stand.X, Cell.X, 0.1f));
	TestTrue(TEXT("north grass adds the stand distance"), FMath::IsNearlyEqual(Stand.Y, Cell.Y + Clearance, 0.1f));
	TestTrue(
		TEXT("the stand point is outside the stone"),
		FVector2D::Distance(Stand, Cell) > ImperiousTower.Plan().GroundExteriorRadiusCm);

	TUniquePtr<FSolidStronghold> Tower = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Imperious,
		FIntPoint(16, 30),
		Cell);
	TestNotNull(TEXT("an Imperious tower"), Tower.Get());
	if (Tower)
	{
		Tower->Orientation = ESolidStrongholdOrientation::North;
		const SolidStronghold::FExitStand NorthExit = SolidStronghold::ExitStand(*Tower);
		TestTrue(TEXT("north exit stands on the north grass"), NorthExit.XY.Equals(Stand, 0.1f));
		TestTrue(TEXT("north exit faces away"), FMath::IsNearlyEqual(NorthExit.YawDeg, 90.f, 0.1f));

		Tower->Orientation = ESolidStrongholdOrientation::East;
		const FVector2D EastDoor = ImperiousTower.OutsideDoorXY(Cell, 0.f);
		const SolidStronghold::FExitStand EastExit = SolidStronghold::ExitStand(*Tower);
		TestTrue(TEXT("east exit stands on the east grass"), EastExit.XY.Equals(EastDoor, 0.1f));
		TestTrue(TEXT("east exit faces away"), FMath::IsNearlyEqual(EastExit.YawDeg, 0.f, 0.1f));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerInteriorVisualsTest,
	"SolidCore1.Stronghold.TowerInterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerInteriorVisualsTest::RunTest(const FString& Parameters)
{
	auto CheckPlan = [this](const SolidStronghold::FSolidTower& Tower, const TCHAR* Label)
	{
		ASolidTowerInterior* Interior = NewObject<ASolidTowerInterior>();
		TestNotNull(TEXT("interior"), Interior);
		if (!Interior)
		{
			return;
		}
		const SolidStronghold::FPlan& Plan = Tower.Plan();
		Interior->SetTower(Tower);
		Interior->BuildVisuals();
		Interior->BuildVisuals();

		UStaticMeshComponent* Door = Interior->GetDoorMesh();
		UInstancedStaticMeshComponent* Grid = Interior->GetFloorGrid();
		TestNotNull(*FString::Printf(TEXT("%s door"), Label), Door);
		TestNotNull(*FString::Printf(TEXT("%s grid"), Label), Grid);
		if (!Door || !Grid)
		{
			return;
		}

		TestEqual(
			*FString::Printf(TEXT("%s yellow grid"), Label),
			Grid->GetInstanceCount(),
			SolidTowerInteriorTestPrivate::ExpectedGridCount(Tower));

		TestTrue(
			*FString::Printf(TEXT("%s door is on local -X"), Label),
			Door->GetRelativeLocation().X < 0.f);
		TestTrue(
			*FString::Printf(TEXT("%s door is the plan opening"), Label),
			FMath::IsNearlyEqual(Door->GetRelativeScale3D().Y * 100.f, Plan.DoorWidthCm, 0.5f)
				&& FMath::IsNearlyEqual(Door->GetRelativeScale3D().Z * 100.f, Plan.DoorHeightCm, 0.5f));
		const float DoorBottom = Door->GetRelativeLocation().Z - Door->GetRelativeScale3D().Z * 50.f;
		TestTrue(
			*FString::Printf(TEXT("%s door meets the ground"), Label),
			FMath::IsNearlyEqual(DoorBottom, 0.f, 0.5f));
		TestTrue(
			*FString::Printf(TEXT("%s door blocks a click"), Label),
			Door->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block);
		const FVector Spawn = Interior->GetCaptainSpawnWorld();
		TestTrue(
			*FString::Printf(TEXT("%s spawn is the cell by the door"), Label),
			SolidTowerInteriorTestPrivate::SpawnIsKeptHex(Plan, Spawn));
		const FVector2D DoorPoint = Tower.DoorOutward()
			* FMath::Max(Tower.GroundInteriorRadiusCm() - 30.f, 0.f);
		const FVector2D SpawnXY(Spawn.X, Spawn.Y);
		TestTrue(
			*FString::Printf(TEXT("%s spawn is nearer the door than the center"), Label),
			FVector2D::DistSquared(SpawnXY, DoorPoint) < DoorPoint.SizeSquared());
		TestTrue(
			*FString::Printf(TEXT("%s spawn looks into the tower"), Label),
			FMath::IsNearlyEqual(Interior->GetCaptainView().Yaw, 0.f, 0.01f));

		UStaticMeshComponent* Cap = Interior->GetCapMesh();
		UStaticMesh* CapMesh = Cap ? Cap->GetStaticMesh() : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s ceiling"), Label), CapMesh);
		if (CapMesh)
		{
			TestTrue(
				*FString::Printf(TEXT("%s ceiling is the top interior"), Label),
				FMath::IsNearlyEqual(
					SolidTowerInteriorTestPrivate::MaxRadius(CapMesh),
					Tower.TopInteriorRadiusCm(),
					1.f));
		}
	};

	auto TowerOf = [](const ESolidStrongholdSize Size)
	{
		SolidStronghold::FSolidTower Tower;
		Tower.Size = Size;
		Tower.BindPlan();
		return Tower;
	};
	const SolidStronghold::FSolidTower ImperiousTower = TowerOf(ESolidStrongholdSize::Imperious);
	const SolidStronghold::FSolidTower StatelyTower = TowerOf(ESolidStrongholdSize::Stately);
	const SolidStronghold::FSolidTower GrandTower = TowerOf(ESolidStrongholdSize::Grand);
	const SolidStronghold::FSolidTower ImpressiveTower = TowerOf(ESolidStrongholdSize::Impressive);
	const SolidStronghold::FSolidTower AverageTower = TowerOf(ESolidStrongholdSize::Average);
	CheckPlan(ImperiousTower, TEXT("Imperious"));
	CheckPlan(StatelyTower, TEXT("Stately"));
	CheckPlan(GrandTower, TEXT("Grand"));
	CheckPlan(ImpressiveTower, TEXT("Impressive"));
	CheckPlan(AverageTower, TEXT("Average"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
