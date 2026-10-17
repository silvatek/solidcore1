#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidDungeonLevel.h"
#include "SolidGameInstance.h"
#include "SolidLair.h"
#include "SolidTowerInterior.h"
#include "SolidTowerShell.h"
#include "SolidTowerStronghold.h"
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

	bool HoleStaysOpen(UStaticMesh* Mesh, const SolidTowerStronghold::FWedge& Gap, const float InnerRadius)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description || !Gap.bOpen)
		{
			return false;
		}
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const float Margin = 0.25f;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			const float Along = SolidTowerStronghold::WrapTau(
				FMath::Atan2(Position.Y, Position.X) - Gap.StartRad);
			if (Along <= Margin || Along >= Gap.SweepRad - Margin)
			{
				continue;
			}
			if (FVector2D(Position.X, Position.Y).Size() > InnerRadius + 1.f)
			{
				return false;
			}
		}
		return true;
	}

	bool DoorGapIsOpen(UStaticMesh* Mesh, const SolidTowerStronghold::FPlan& Plan)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}
		const float Radius = FMath::Max(Plan.GroundInteriorRadiusCm(), 1.f);
		const float Half = FMath::Asin(FMath::Clamp((Plan.DoorWidthCm * 0.5f) / Radius, 0.f, 1.f));
		const float Gap0 = SolidTowerStronghold::WrapTau(Plan.DoorAngleRad - Half);
		const float Gap1 = Gap0 + Half * 2.f;
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			const float Angle = SolidTowerStronghold::WrapTau(FMath::Atan2(Position.Y, Position.X));
			if (Angle > Gap0 + 0.02f
				&& Angle < Gap1 - 0.02f
				&& Position.Z > 5.f
				&& Position.Z < Plan.DoorHeightCm - 5.f)
			{
				return false;
			}
		}
		return true;
	}

	int32 ExpectedGridCount(const SolidTowerStronghold::FPlan& Plan)
	{
		int32 Count = 0;
		for (int32 Floor = 0; Floor < Plan.FloorCount; ++Floor)
		{
			TArray<SolidStrongholdGrid::FHex> Hexes;
			TArray<SolidStrongholdGrid::FSegment> Segments;
			SolidTowerStronghold::CollectKeptHexes(Plan, Floor, Hexes);
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

	/** The landing annulus keeps a top face. The shared edge with the hole does not count. */
	bool LandingStaysFloored(
		UStaticMesh* Mesh,
		const SolidTowerStronghold::FWedge& Landing,
		const float InnerRadius,
		const float ZTop)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description || !Landing.bOpen || Landing.SweepRad <= 0.1f)
		{
			return false;
		}
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const float Margin = 0.04f;
		const float InnerStart = Landing.StartRad + Margin;
		const float InnerSweep = Landing.SweepRad - Margin * 2.f;
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			if (FMath::Abs(Position.Z - ZTop) > 0.5f)
			{
				continue;
			}
			if (FVector2D(Position.X, Position.Y).Size() <= InnerRadius + 1.f)
			{
				continue;
			}
			if (SolidTowerStronghold::AngleInSweep(
				FMath::Atan2(Position.Y, Position.X),
				InnerStart,
				InnerSweep))
			{
				return true;
			}
		}
		return false;
	}

	bool CenterStaysFloored(UStaticMesh* Mesh, const float ZTop, const float InnerRadius)
	{
		FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!Description)
		{
			return false;
		}
		FStaticMeshAttributes Attributes(*Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		for (const FVertexID VertexID : Description->Vertices().GetElementIDs())
		{
			const FVector3f Position = Positions[VertexID];
			if (FMath::Abs(Position.Z - ZTop) > 0.5f)
			{
				continue;
			}
			if (FVector2D(Position.X, Position.Y).Size() < InnerRadius - 1.f)
			{
				return true;
			}
		}
		return false;
	}

	bool SpawnIsKeptHex(const SolidTowerStronghold::FPlan& Plan, const FVector& Spawn)
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidTowerStronghold::CollectKeptHexes(Plan, 0, Hexes);
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
	TestTrue(TEXT("tower options request the tower"), SolidDungeonLevel::RequestsTower(SolidDungeonLevel::TowerEnterOptions()));
	TestFalse(TEXT("tower options are not the cave"), SolidDungeonLevel::RequestsDungeon(SolidDungeonLevel::TowerEnterOptions()));
	TestTrue(TEXT("cave options still request the cave"), SolidDungeonLevel::RequestsDungeon(SolidDungeonLevel::EnterOptions()));
	TestFalse(TEXT("cave options are not the tower"), SolidDungeonLevel::RequestsTower(SolidDungeonLevel::EnterOptions()));
	TestFalse(TEXT("an empty url is not the tower"), SolidDungeonLevel::RequestsTower(FString()));
	TestFalse(TEXT("an empty url is not the cave"), SolidDungeonLevel::RequestsDungeon(FString()));

	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	TestNotNull(TEXT("game instance"), Instance);
	if (!Instance)
	{
		return false;
	}

	TestFalse(TEXT("fresh instance is not in a tower"), Instance->IsInTower());
	TestFalse(TEXT("fresh instance does not return to a tower door"), Instance->WantsTowerDoorSpawn());
	TestFalse(TEXT("fresh instance is the open world"), Instance->IsInDungeon());

	FSolidTravelSession Inside;
	Inside.bInTower = true;
	Inside.bInDungeon = false;
	Inside.bReturnToDungeonMouth = false;
	Inside.TowerCell = FIntPoint(16, 30);
	Inside.Tower = { ESolidStrongholdType::Tower, ESolidStrongholdSize::Imperious };
	Instance->Capture(Inside);
	TestTrue(TEXT("a tower session is inside"), Instance->IsInTower());
	TestFalse(TEXT("a tower session is not the cave"), Instance->IsInDungeon());
	TestFalse(TEXT("a tower session does not spawn at the cave mouth"), Instance->WantsDungeonMouthSpawn());
	TestFalse(TEXT("a tower session does not stand outside yet"), Instance->WantsTowerDoorSpawn());

	FSolidTravelSession Leave;
	Leave.bInTower = false;
	Leave.bReturnToTowerDoor = true;
	Leave.bReturnToDungeonMouth = false;
	Leave.TowerCell = FIntPoint(16, 30);
	Instance->Capture(Leave);
	TestTrue(TEXT("leaving stands at the tower door"), Instance->WantsTowerDoorSpawn());
	TestFalse(TEXT("leaving the tower is not a cave spawn"), Instance->WantsDungeonMouthSpawn());
	TestFalse(TEXT("leaving the tower is the open world"), Instance->IsInTower());
	Instance->ClearTowerDoorSpawn();
	TestFalse(TEXT("tower door spawn is used once"), Instance->WantsTowerDoorSpawn());

	FSolidTravelSession CaveLeave;
	CaveLeave.bInDungeon = false;
	CaveLeave.bReturnToDungeonMouth = true;
	Instance->Capture(CaveLeave);
	TestFalse(TEXT("a cave leave is not a tower spawn"), Instance->WantsTowerDoorSpawn());
	TestTrue(TEXT("a cave leave still stands at the mouth"), Instance->WantsDungeonMouthSpawn());

	const SolidLair::FPassageHit TowerEnter = SolidLair::HitOnTower(FIntPoint(16, 30), false);
	const SolidLair::FPassageHit TowerLeave = SolidLair::HitOnTower(FIntPoint(16, 30), true);
	const SolidLair::FPassageHit CaveEnter = SolidLair::HitOnMouth(false, 0, false);
	TestEqual(TEXT("tower enter tip"), FString(SolidLair::PassageTip(TowerEnter)), FString(SolidTowerStronghold::EnterLabel));
	TestEqual(TEXT("tower leave tip"), FString(SolidLair::PassageTip(TowerLeave)), FString(SolidTowerStronghold::LeaveLabel));
	TestEqual(TEXT("cave tip stays Enter cave"), FString(SolidLair::PassageTip(CaveEnter)), FString(SolidDungeonLevel::EnterLabel));
	TestEqual(
		TEXT("a shut cave still lets the tower door open"),
		static_cast<uint8>(SolidLair::AllowPassage(TowerEnter, false, false).Action),
		static_cast<uint8>(SolidDungeonPassage::EHit::Enter));
	TestEqual(
		TEXT("that click keeps the cell"),
		SolidLair::AllowPassage(TowerEnter, false, false).TowerCell.X,
		static_cast<int32>(16));
	TestEqual(
		TEXT("a shut cave still drops the cave door"),
		static_cast<uint8>(SolidLair::AllowPassage(CaveEnter, false, true).Action),
		static_cast<uint8>(SolidDungeonPassage::EHit::None));

	ASolidTowerShell* Shell = NewObject<ASolidTowerShell>();
	TestNotNull(TEXT("shell"), Shell);
	if (Shell)
	{
		Shell->SetTowerCell(FIntPoint(16, 30));
		TestEqual(TEXT("shell remembers its cell"), Shell->GetTowerCell().X, static_cast<int32>(16));
		TestEqual(TEXT("shell remembers its column"), Shell->GetTowerCell().Y, static_cast<int32>(30));
	}

	const FVector2D Cell(1000.f, 2000.f);
	const SolidTowerStronghold::FPlan& Imperious = SolidTowerStronghold::Imperious;
	const FVector2D Stand = SolidTowerStronghold::OutsideDoorXY(Cell, 90.f, Imperious);
	const float Clearance = Imperious.GroundExteriorRadiusCm
		+ SolidTowerStronghold::DoorProudCm
		+ SolidTowerStronghold::DoorStandClearCm;
	TestTrue(TEXT("north grass stands north of the cell"), FMath::IsNearlyEqual(Stand.X, Cell.X, 0.1f));
	TestTrue(TEXT("north grass adds the stand distance"), FMath::IsNearlyEqual(Stand.Y, Cell.Y + Clearance, 0.1f));
	TestTrue(
		TEXT("the stand point is outside the stone"),
		FVector2D::Distance(Stand, Cell) > Imperious.GroundExteriorRadiusCm);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTowerInteriorVisualsTest,
	"SolidCore1.Stronghold.TowerInterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTowerInteriorVisualsTest::RunTest(const FString& Parameters)
{
	auto CheckPlan = [this](const SolidTowerStronghold::FPlan& Plan, const TCHAR* Label)
	{
		ASolidTowerInterior* Interior = NewObject<ASolidTowerInterior>();
		TestNotNull(TEXT("interior"), Interior);
		if (!Interior)
		{
			return;
		}
		Interior->SetPlan(Plan);
		Interior->BuildVisuals();

		TestEqual(*FString::Printf(TEXT("%s floor count"), Label), Interior->GetFloorCount(), Plan.FloorCount);
		TestEqual(*FString::Printf(TEXT("%s wall count"), Label), Interior->GetWallCount(), Plan.FloorCount);

		UStaticMeshComponent* Ground = Interior->GetFloor(0);
		UStaticMeshComponent* Door = Interior->GetDoorMesh();
		UInstancedStaticMeshComponent* Grid = Interior->GetFloorGrid();
		UInstancedStaticMeshComponent* Stairs = Interior->GetStairs();
		UStaticMesh* GroundMesh = Ground ? Ground->GetStaticMesh() : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s ground"), Label), Ground);
		TestNotNull(*FString::Printf(TEXT("%s ground mesh"), Label), GroundMesh);
		TestNotNull(*FString::Printf(TEXT("%s door"), Label), Door);
		TestNotNull(*FString::Printf(TEXT("%s grid"), Label), Grid);
		TestNotNull(*FString::Printf(TEXT("%s stairs"), Label), Stairs);
		if (!Ground || !GroundMesh || !Door || !Grid || !Stairs)
		{
			return;
		}

		TestTrue(
			*FString::Printf(TEXT("%s ground reaches the interior wall"), Label),
			FMath::IsNearlyEqual(
				SolidTowerInteriorTestPrivate::MaxRadius(GroundMesh),
				Plan.GroundInteriorRadiusCm(),
				1.f));

		const int32 Upper = 1;
		const int32 Top = Plan.FloorCount - 1;
		UStaticMeshComponent* UpperFloor = Interior->GetFloor(Upper);
		UStaticMeshComponent* TopFloor = Interior->GetFloor(Top);
		UStaticMesh* UpperMesh = UpperFloor ? UpperFloor->GetStaticMesh() : nullptr;
		UStaticMesh* TopMesh = TopFloor ? TopFloor->GetStaticMesh() : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s upper floor"), Label), UpperMesh);
		TestNotNull(*FString::Printf(TEXT("%s top floor"), Label), TopMesh);
		if (UpperMesh && TopMesh)
		{
			const float UpperInner = SolidTowerStronghold::InteriorRadiusCm(Plan, SolidTowerStronghold::FloorZCm(Plan, Upper))
				- Plan.StairWidthCm;
			const float TopInner = SolidTowerStronghold::InteriorRadiusCm(Plan, SolidTowerStronghold::FloorZCm(Plan, Top))
				- Plan.StairWidthCm;
			TestTrue(
				*FString::Printf(TEXT("%s arrival hole stays open"), Label),
				SolidTowerInteriorTestPrivate::HoleStaysOpen(UpperMesh, SolidTowerStronghold::GapWedge(Plan, Upper), UpperInner));
			TestTrue(
				*FString::Printf(TEXT("%s top hole stays open"), Label),
				SolidTowerInteriorTestPrivate::HoleStaysOpen(TopMesh, SolidTowerStronghold::GapWedge(Plan, Top), TopInner));
			const float UpperZ = SolidTowerStronghold::FloorZCm(Plan, Upper);
			const float TopZ = SolidTowerStronghold::FloorZCm(Plan, Top);
			TestTrue(
				*FString::Printf(TEXT("%s landing stays floored"), Label),
				SolidTowerInteriorTestPrivate::LandingStaysFloored(
					UpperMesh,
					SolidTowerStronghold::LandingWedge(Plan, Upper),
					UpperInner,
					UpperZ));
			TestTrue(
				*FString::Printf(TEXT("%s top landing stays floored"), Label),
				SolidTowerInteriorTestPrivate::LandingStaysFloored(
					TopMesh,
					SolidTowerStronghold::LandingWedge(Plan, Top),
					TopInner,
					TopZ));
			TestTrue(
				*FString::Printf(TEXT("%s middle stays floored"), Label),
				SolidTowerInteriorTestPrivate::CenterStaysFloored(UpperMesh, UpperZ, UpperInner));
			TestTrue(
				*FString::Printf(TEXT("%s top middle stays floored"), Label),
				SolidTowerInteriorTestPrivate::CenterStaysFloored(TopMesh, TopZ, TopInner));
		}

		TestEqual(
			*FString::Printf(TEXT("%s yellow grid"), Label),
			Grid->GetInstanceCount(),
			SolidTowerInteriorTestPrivate::ExpectedGridCount(Plan));
		TestEqual(
			*FString::Printf(TEXT("%s treads"), Label),
			Stairs->GetInstanceCount(),
			(Plan.FloorCount - 1) * Plan.StairTreadCount);

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
		TestTrue(
			*FString::Printf(TEXT("%s floor blocks"), Label),
			Ground->GetCollisionEnabled() == ECollisionEnabled::QueryAndPhysics);

		UStaticMeshComponent* GroundWall = Interior->GetWall(0);
		UStaticMesh* GroundWallMesh = GroundWall ? GroundWall->GetStaticMesh() : nullptr;
		TestNotNull(*FString::Printf(TEXT("%s ground wall"), Label), GroundWallMesh);
		if (GroundWallMesh)
		{
			TestTrue(
				*FString::Printf(TEXT("%s door gap is open"), Label),
				SolidTowerInteriorTestPrivate::DoorGapIsOpen(GroundWallMesh, Plan));
		}

		const FVector Spawn = Interior->GetCaptainSpawnWorld();
		TestTrue(
			*FString::Printf(TEXT("%s spawn is a kept hex"), Label),
			SolidTowerInteriorTestPrivate::SpawnIsKeptHex(Plan, Spawn));
		const FVector2D DoorPoint = SolidTowerStronghold::DoorOutward(Plan)
			* FMath::Max(Plan.GroundInteriorRadiusCm() - 30.f, 0.f);
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
					Plan.TopInteriorRadiusCm(),
					1.f));
		}
	};

	CheckPlan(SolidTowerStronghold::Imperious, TEXT("Imperious"));
	CheckPlan(SolidTowerStronghold::Average, TEXT("Average"));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
