#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/SolidStronghold.h"
#include "SolidGameState.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressInteriorTest,
	"SolidCore1.Stronghold.FortressInterior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressInteriorTest::RunTest(const FString& Parameters)
{
	const FIntPoint HumbleCell(35, 15);
	TUniquePtr<FSolidStronghold> Keep = SolidStronghold::Make(
		ESolidStrongholdType::Fortress,
		ESolidStrongholdSize::Humble,
		HumbleCell,
		SolidStronghold::WorldXY(HumbleCell));
	TestNotNull(TEXT("a Humble fortress"), Keep.Get());
	if (!Keep)
	{
		return false;
	}

	const SolidStronghold::FSolidFortress* Fortress = static_cast<const SolidStronghold::FSolidFortress*>(Keep.Get());
	TestTrue(
		TEXT("Humble names the fortress interior"),
		Keep->InteriorClass() == ASolidFortressInterior::StaticClass());

	ASolidFortressInterior* Room = NewObject<ASolidFortressInterior>();
	TestNotNull(TEXT("fortress interior"), Room);
	if (!Room)
	{
		return false;
	}
	Keep->ConfigureInterior(Room);
	TestEqual(TEXT("the visit stands on the fortress column"), Room->GetStrongholdCell().X, HumbleCell.X);
	TestEqual(TEXT("the visit stands on the fortress row"), Room->GetStrongholdCell().Y, HumbleCell.Y);
	TestEqual(TEXT("the opening is the gate"), Room->GetPassageMesh(), Room->GetGateMesh());
	TestNotNull(TEXT("the gate"), Room->GetGateMesh());
	TestEqual(TEXT("the outer wall is six stretches"), Room->GetCurtainMeshes().Num(), 6);
	if (!Room->GetGateMesh() || Room->GetCurtainMeshes().Num() != 6)
	{
		return false;
	}

	const float HalfWidth = Fortress->OuterWidthCm() * 0.5f;
	const float HalfDepth = Fortress->OuterDepthCm() * 0.5f;
	float MinX = TNumericLimits<float>::Max();
	float MaxX = -MinX;
	float MinY = MinX;
	float MaxY = -MinX;
	float TopZ = 0.f;
	for (const TObjectPtr<UStaticMeshComponent>& Part : Room->GetCurtainMeshes())
	{
		if (!Part)
		{
			continue;
		}
		const FVector Location = Part->GetRelativeLocation();
		const FVector Scale = Part->GetRelativeScale3D();
		MinX = FMath::Min(MinX, Location.X - Scale.X * 50.f);
		MaxX = FMath::Max(MaxX, Location.X + Scale.X * 50.f);
		MinY = FMath::Min(MinY, Location.Y - Scale.Y * 50.f);
		MaxY = FMath::Max(MaxY, Location.Y + Scale.Y * 50.f);
		TopZ = FMath::Max(TopZ, Location.Z + Scale.Z * 50.f);
	}

	TestTrue(
		TEXT("the ring matches the outer face"),
		FMath::IsNearlyEqual(MinX, -HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MaxX, HalfDepth, 0.5f)
			&& FMath::IsNearlyEqual(MinY, -HalfWidth, 0.5f)
			&& FMath::IsNearlyEqual(MaxY, HalfWidth, 0.5f)
			&& FMath::IsNearlyEqual(TopZ, Fortress->CurtainHeightCm(), 0.5f));

	const UStaticMeshComponent* Gate = Room->GetGateMesh();
	TestTrue(
		TEXT("the gate is the storey opening on local -X"),
		Gate->GetRelativeLocation().X < 0.f
			&& FMath::IsNearlyZero(Gate->GetRelativeLocation().Y, 0.5f)
			&& FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Y * 100.f, FSolidStronghold::OpeningCm, 0.5f)
			&& FMath::IsNearlyEqual(Gate->GetRelativeScale3D().Z * 100.f, FSolidStronghold::StoreyCm, 0.5f)
			&& FMath::IsNearlyZero(Gate->GetRelativeLocation().Z - Gate->GetRelativeScale3D().Z * 50.f, 0.5f));

	const FVector Spawn = Room->GetCaptainSpawnWorld();
	TestTrue(
		TEXT("the captain starts one hex inside the gate"),
		FMath::IsNearlyEqual(
			Spawn.X,
			FSolidStronghold::WallCm + SolidStrongholdGrid::FlatToFlatCm - HalfDepth,
			0.5f)
			&& FMath::IsNearlyZero(Spawn.Y, 0.5f)
			&& FMath::IsNearlyZero(Spawn.Z, 0.5f));
	TestTrue(TEXT("the arrival looks into the yard"), FMath::IsNearlyZero(Room->GetCaptainView().Yaw, 0.1f));

	USolidGameState& State = *NewObject<USolidGameState>();
	TestFalse(TEXT("a shut fortress refuses the gate"), Keep->AcceptsEnter(State, Keep.Get(), HumbleCell));
	State.bStrongholdsEnabled = true;
	TestTrue(TEXT("the open fortress accepts its gate"), Keep->AcceptsEnter(State, Keep.Get(), HumbleCell));
	TestFalse(
		TEXT("another cell is not the gate"),
		Keep->AcceptsEnter(State, Keep.Get(), FIntPoint(1, 1)));

	Keep->RememberExit(nullptr);
	const SolidStronghold::FExitStand Leave = SolidStronghold::ExitStand(*Keep);
	TestTrue(
		TEXT("leaving stands one hex outside the gate"),
		Leave.XY.Equals(Fortress->OutsideGateXY(nullptr), 0.5f));
	TestTrue(TEXT("leaving faces away along the grass"), FMath::IsNearlyEqual(Leave.YawDeg, 0.f, 0.1f));

	TestTrue(TEXT("the buildings are drawn"), Room->GetBuildingMeshes().Num() >= 2);
	TestEqual(TEXT("the gate is still the way out"), Room->GetPassageMesh(), Room->GetGateMesh());

	auto Covers = [](const UStaticMeshComponent* Part, const FVector& Point)
	{
		const FVector Location = Part->GetRelativeLocation();
		const FVector Half = Part->GetRelativeScale3D() * 50.f;
		return Point.X >= Location.X - Half.X - 0.5f
			&& Point.X <= Location.X + Half.X + 0.5f
			&& Point.Y >= Location.Y - Half.Y - 0.5f
			&& Point.Y <= Location.Y + Half.Y + 0.5f
			&& Point.Z >= Location.Z - Half.Z - 0.5f
			&& Point.Z <= Location.Z + Half.Z + 0.5f;
	};

	const float Width = Fortress->OuterWidthCm();
	const float Depth = Fortress->OuterDepthCm();
	const float Wall = FSolidStronghold::WallCm;
	int32 Gaps = 0;
	float WestGapY = TNumericLimits<float>::Max();
	float EastGapY = -WestGapY;
	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
	{
		float FaceMinX = One.InnerMinCm.X - Wall;
		float FaceMaxX = One.InnerMinCm.X + One.WidthCm() + Wall;
		float FaceMinY = One.InnerMinCm.Y - Wall;
		float FaceMaxY = One.InnerMinCm.Y + One.DepthCm() + Wall;
		if (FMath::IsNearlyEqual(FaceMinX, 0.f, 0.1f))
		{
			FaceMinX += Wall;
		}
		if (FMath::IsNearlyEqual(FaceMaxX, Width, 0.1f))
		{
			FaceMaxX -= Wall;
		}
		if (FMath::IsNearlyEqual(FaceMinY, 0.f, 0.1f))
		{
			FaceMinY += Wall;
		}
		if (FMath::IsNearlyEqual(FaceMaxY, Depth, 0.1f))
		{
			FaceMaxY -= Wall;
		}

		const float DoorX = (FaceMinX + FaceMaxX) * 0.5f;
		const FVector Opening(
			Depth * 0.5f - (FaceMaxY - Wall * 0.5f),
			DoorX - Width * 0.5f,
			FSolidStronghold::StoreyCm * 0.5f);
		const FVector Inside(
			Depth * 0.5f - (FaceMaxY - Wall - 30.f),
			DoorX - Width * 0.5f,
			100.f);
		bool bOpeningBlocked = false;
		bool bInsideBlocked = false;
		for (const TObjectPtr<UStaticMeshComponent>& Part : Room->GetBuildingMeshes())
		{
			if (!Part)
			{
				continue;
			}
			bOpeningBlocked = bOpeningBlocked || Covers(Part, Opening);
			bInsideBlocked = bInsideBlocked || Covers(Part, Inside);
		}
		TestFalse(TEXT("the doorway is a gap"), bOpeningBlocked);
		TestFalse(TEXT("the building inside is open"), bInsideBlocked);
		const float RoomX = One.InnerMinCm.X + One.WidthCm() * 0.5f;
		const float RoomY = One.InnerMinCm.Y + One.DepthCm() * 0.5f;
		const FVector Headroom(
			Depth * 0.5f - RoomY,
			RoomX - Width * 0.5f,
			FSolidStronghold::StoreyCm - 30.f);
		const FVector Ceiling(
			Depth * 0.5f - RoomY,
			RoomX - Width * 0.5f,
			FSolidStronghold::StoreyCm + 10.f);
		bool bHeadroomBlocked = false;
		bool bCeilingPresent = false;
		for (const TObjectPtr<UStaticMeshComponent>& Part : Room->GetBuildingMeshes())
		{
			if (!Part)
			{
				continue;
			}
			bHeadroomBlocked = bHeadroomBlocked || Covers(Part, Headroom);
			bCeilingPresent = bCeilingPresent || Covers(Part, Ceiling);
		}
		TestFalse(TEXT("the room stays open under the ceiling"), bHeadroomBlocked);
		TestTrue(TEXT("a ceiling closes the room at one storey"), bCeilingPresent);
		WestGapY = FMath::Min(WestGapY, Opening.Y);
		EastGapY = FMath::Max(EastGapY, Opening.Y);
		++Gaps;
	}
	TestEqual(TEXT("Humble has two doorways"), Gaps, 2);
	TestTrue(TEXT("the Keep doorway is east of the Annex"), EastGapY > WestGapY + 1.f);

	int32 ExpectedLines = 0;
	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
	{
		TArray<SolidStrongholdGrid::FHex> Hexes;
		TArray<SolidStrongholdGrid::FSegment> Segments;
		One.CollectHexes(Hexes);
		SolidStrongholdGrid::CollectSegments(Hexes, Segments);
		ExpectedLines += Segments.Num();
	}
	UInstancedStaticMeshComponent* Grid = Room->GetFloorGrid();
	TestNotNull(TEXT("the floor grid"), Grid);
	if (!Grid)
	{
		return false;
	}
	TestEqual(TEXT("the rooms draw their hex edges"), Grid->GetInstanceCount(), ExpectedLines);
	const float LineZ = SolidStrongholdGrid::LineLiftCm + SolidStrongholdGrid::LineHeightCm * 0.5f;
	bool bLinesOnTheFloor = ExpectedLines > 0;
	bool bLinesInTheRooms = ExpectedLines > 0;
	for (int32 Index = 0; Index < Grid->GetInstanceCount(); ++Index)
	{
		FTransform Line;
		if (!Grid->GetInstanceTransform(Index, Line, false))
		{
			bLinesOnTheFloor = false;
			bLinesInTheRooms = false;
			continue;
		}
		const FVector Location = Line.GetLocation();
		bLinesOnTheFloor = bLinesOnTheFloor && FMath::IsNearlyEqual(Location.Z, LineZ, 0.1f);
		const float PlanX = Location.Y + Width * 0.5f;
		const float PlanY = Depth * 0.5f - Location.X;
		bool bInRoom = false;
		for (const SolidStronghold::FSolidFortressBuilding& One : Fortress->Buildings)
		{
			if (PlanX >= One.InnerMinCm.X - 1.f
				&& PlanX <= One.InnerMinCm.X + One.WidthCm() + 1.f
				&& PlanY >= One.InnerMinCm.Y - 1.f
				&& PlanY <= One.InnerMinCm.Y + One.DepthCm() + 1.f)
			{
				bInRoom = true;
				break;
			}
		}
		bLinesInTheRooms = bLinesInTheRooms && bInRoom;
	}
	TestTrue(TEXT("the hex lines sit on the floor"), bLinesOnTheFloor);
	TestTrue(TEXT("the hex lines stay inside the rooms"), bLinesInTheRooms);

	TestEqual(TEXT("each room has a light"), Room->GetRoomLights().Num(), Fortress->Buildings.Num());
	TArray<int32> LitRooms;
	for (const TObjectPtr<UPointLightComponent>& Light : Room->GetRoomLights())
	{
		if (!Light)
		{
			continue;
		}
		const FVector Location = Light->GetRelativeLocation();
		TestTrue(
			TEXT("the light hangs in the room"),
			Location.Z > 50.f && Location.Z < FSolidStronghold::StoreyCm - 10.f);
		const FLinearColor Color = Light->GetLightColor();
		TestTrue(
			TEXT("the light is the room color"),
			FMath::IsNearlyEqual(Color.R, 1.f, 0.01f)
				&& FMath::IsNearlyEqual(Color.G, 0.82f, 0.01f)
				&& FMath::IsNearlyEqual(Color.B, 0.55f, 0.01f));
		const float PlanX = Location.Y + Width * 0.5f;
		const float PlanY = Depth * 0.5f - Location.X;
		int32 RoomIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Fortress->Buildings.Num(); ++Index)
		{
			const SolidStronghold::FSolidFortressBuilding& One = Fortress->Buildings[Index];
			if (PlanX >= One.InnerMinCm.X
				&& PlanX <= One.InnerMinCm.X + One.WidthCm()
				&& PlanY >= One.InnerMinCm.Y
				&& PlanY <= One.InnerMinCm.Y + One.DepthCm())
			{
				RoomIndex = Index;
				TestTrue(
					TEXT("the light reaches the far wall"),
					Light->AttenuationRadius >= FMath::Max(One.WidthCm(), One.DepthCm()));
				break;
			}
		}
		TestTrue(TEXT("the light stands in its room"), RoomIndex != INDEX_NONE);
		LitRooms.AddUnique(RoomIndex);
	}
	TestEqual(TEXT("the two rooms are both lit"), LitRooms.Num(), 2);

	TestEqual(TEXT("the fortress has one floor to arrange"), Room->ArrangeFloorCount(), 1);
	TArray<SolidStrongholdGrid::FHex> ArrangeCells;
	TestTrue(TEXT("arrange uses the ground hexes"), Room->CollectArrangeFloor(0, ArrangeCells));
	TArray<SolidStrongholdGrid::FHex> Ground;
	Fortress->CollectGroundHexes(Fortress->Size, Ground);
	TestEqual(TEXT("every ground hex can take a piece"), ArrangeCells.Num(), Ground.Num());
	if (ArrangeCells.Num() > 0)
	{
		const FVector HexPoint(ArrangeCells[0].Center.X, ArrangeCells[0].Center.Y, 0.f);
		const FVector InRoom = Room->HexPointInRoom(HexPoint);
		const FVector Back = Room->RoomPointOnHexGrid(InRoom);
		TestTrue(TEXT("a hex turns into the room and back"), Back.Equals(HexPoint, 0.1f));
		const FVector OnTheLine(
			HalfDepth - ArrangeCells[0].Center.Y,
			ArrangeCells[0].Center.X - HalfWidth,
			0.f);
		TestTrue(TEXT("the hex sits where the floor line sits"), InRoom.Equals(OnTheLine, 0.1f));
		TestTrue(
			TEXT("the pointer hits that hex"),
			SolidStrongholdGrid::PointInHex(FVector2D(Back.X, Back.Y), ArrangeCells[0].Center));
	}
	TestTrue(
		TEXT("looking into the yard looks across the hex grid"),
		Room->RoomDirectionOnHexGrid(FVector(1.f, 0.f, 0.f)).Equals(FVector(0.f, -1.f, 0.f), 0.1f));
	TestTrue(TEXT("a hex facing turns with the room"), FMath::IsNearlyEqual(Room->HexFacingInRoom(0.f), 90.f));
	return true;
}

#endif
