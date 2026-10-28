#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool VerticesInside(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const FVector2D& InnerMin,
		const FVector2D& InnerSize)
	{
		constexpr float Slack = 0.05f;
		const float MaxX = InnerMin.X + InnerSize.X;
		const float MaxY = InnerMin.Y + InnerSize.Y;
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			FVector2D Vertices[6];
			SolidStrongholdGrid::HexVertices(Hex.Center, Vertices);
			for (int32 Index = 0; Index < 6; ++Index)
			{
				const FVector2D& Point = Vertices[Index];
				if (Point.X < InnerMin.X - Slack
					|| Point.X > MaxX + Slack
					|| Point.Y < InnerMin.Y - Slack
					|| Point.Y > MaxY + Slack)
				{
					return false;
				}
			}
		}
		return true;
	}

	float LaidWidthCm(const SolidStronghold::FSolidFortress& Fortress)
	{
		float Width = FSolidStronghold::WallCm;
		for (int32 Index = 0; Index < Fortress.Buildings.Num(); ++Index)
		{
			Width += Fortress.Buildings[Index].WidthCm() + FSolidStronghold::WallCm;
			if (Index + 1 < Fortress.Buildings.Num())
			{
				Width += Fortress.GapCm + FSolidStronghold::WallCm;
			}
		}
		return Width;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressHumbleTest,
	"SolidCore1.Stronghold.FortressHumble",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressHumbleTest::RunTest(const FString& Parameters)
{
	const SolidStronghold::FSolidFortress& Fortress = SolidStronghold::FSolidFortress::Instance();
	TestTrue(TEXT("Humble is supported"), Fortress.Supported(ESolidStrongholdSize::Humble));
	TestFalse(TEXT("Meagre stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("Average stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Average));
	TestEqual(TEXT("the parapet is one hex across"), Fortress.ParapetCm, SolidStrongholdGrid::FlatToFlatCm);
	TestEqual(
		TEXT("the outer wall is the storey plus the parapet"),
		Fortress.CurtainHeightCm(),
		FSolidStronghold::StoreyCm + Fortress.ParapetCm);
	TestEqual(TEXT("Humble has one building"), Fortress.Buildings.Num(), 1);
	TestTrue(
		TEXT("the outer width is the building and the walls"),
		FMath::IsNearlyEqual(Fortress.OuterWidthCm(), LaidWidthCm(Fortress), 0.05f));
	TestTrue(
		TEXT("the outer depth is the room, the yard, and the walls"),
		Fortress.Buildings.Num() == 1
			&& FMath::IsNearlyEqual(
				Fortress.OuterDepthCm(),
				FSolidStronghold::WallCm + Fortress.Buildings[0].DepthCm() + FSolidStronghold::WallCm + Fortress.YardDepthCm + FSolidStronghold::WallCm,
				0.05f));

	if (Fortress.Buildings.Num() != 1)
	{
		return false;
	}
	const SolidStronghold::FSolidFortressBuilding& Building = Fortress.Buildings[0];
	TestEqual(TEXT("the building has two floors"), Building.Floors.Num(), 2);
	if (Building.Floors.Num() != 2)
	{
		return false;
	}
	TestTrue(TEXT("the ground floor is small"), Building.Floors[0] == ESolidStrongholdRoom::Small);
	TestTrue(TEXT("the upper floor is medium"), Building.Floors[1] == ESolidStrongholdRoom::Medium);
	TestTrue(TEXT("the doorway faces north"), Building.Facing == ESolidStrongholdOrientation::NorthFace);
	TestTrue(TEXT("the doorway is centered on the landing"), FMath::IsNearlyEqual(Building.DoorFromInnerCm(), 100.f, 0.05f));
	TestTrue(TEXT("a second floor keeps a stair"), Building.HasStair());
	TestTrue(
		TEXT("the inner face is the medium span plus the stair"),
		FMath::IsNearlyEqual(Building.InnerWidthCm, 540.f, 0.05f)
			&& FMath::IsNearlyEqual(Building.InnerDepthCm, Building.SpanDepthCm() + FSolidStronghold::OpeningCm, 0.05f));

	TArray<SolidStrongholdGrid::FHex> Hexes;
	Building.CollectHexes(Hexes);
	TestEqual(
		TEXT("the ground floor is the small room"),
		Hexes.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TestTrue(
		TEXT("the ground hexes stay out of the stair and the spare floor"),
		VerticesInside(Hexes, Building.InnerMinCm, FVector2D(Building.SpanWidthCm(), Building.SpanDepthCm())));

	TArray<SolidStrongholdGrid::FHex> Ground;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Humble, Ground);
	TestEqual(
		TEXT("the ground floor is the small room"),
		Ground.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TSet<uint64> Cells;
	bool bDistinct = true;
	for (const SolidStrongholdGrid::FHex& Hex : Ground)
	{
		const uint64 Key = (static_cast<uint64>(static_cast<uint32>(Hex.Col)) << 32)
			| static_cast<uint32>(Hex.Row);
		if (Cells.Contains(Key))
		{
			bDistinct = false;
		}
		Cells.Add(Key);
	}
	TestTrue(TEXT("each ground hex has its own cell"), bDistinct);

	TArray<SolidStrongholdGrid::FHex> Listed;
	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress.Buildings)
	{
		TArray<SolidStrongholdGrid::FHex> Room;
		One.CollectHexes(Room);
		Listed.Append(Room);
	}
	bool bGroundOrder = Ground.Num() == Listed.Num();
	for (int32 Index = 0; bGroundOrder && Index < Listed.Num(); ++Index)
	{
		bGroundOrder = Ground[Index].Center.Equals(Listed[Index].Center, 0.01f);
	}
	TestTrue(TEXT("the ground floor lists each building in order"), bGroundOrder);

	TArray<SolidStrongholdGrid::FHex> Empty;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Average, Empty);
	TestEqual(TEXT("Average has no ground hexes"), Empty.Num(), 0);
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Meagre, Empty);
	TestEqual(TEXT("Meagre has no ground hexes"), Empty.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressRoomFaceTest,
	"SolidCore1.Stronghold.FortressRoomFace",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressRoomFaceTest::RunTest(const FString& Parameters)
{
	SolidStronghold::FSolidFortressBuilding Room;
	Room.Floors = { ESolidStrongholdRoom::Medium };
	Room.InnerMinCm = FVector2D(50.f, 50.f);

	TArray<SolidStrongholdGrid::FHex> Filled;
	Room.CollectHexes(Filled);
	TestEqual(
		TEXT("the grid fills a tight room"),
		Filled.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium));
	TestTrue(
		TEXT("a tight room's width is the vertex span"),
		FMath::IsNearlyEqual(Room.WidthCm(), Room.SpanWidthCm(), 0.05f));
	TestTrue(
		TEXT("a tight room's depth is the vertex span"),
		FMath::IsNearlyEqual(Room.DepthCm(), Room.SpanDepthCm(), 0.05f));

	const float Spare = 100.f;
	Room.InnerWidthCm = Room.SpanWidthCm() + Spare;
	Room.InnerDepthCm = Room.SpanDepthCm() + Spare;
	TArray<SolidStrongholdGrid::FHex> Wide;
	Room.CollectHexes(Wide);
	TestEqual(TEXT("spare floor adds no hex"), Wide.Num(), Filled.Num());
	TestTrue(
		TEXT("every hex sits inside the wider face"),
		VerticesInside(Wide, Room.InnerMinCm, FVector2D(Room.WidthCm(), Room.DepthCm())));

	const FVector2D Shift(Spare * 0.5f, Spare * 0.5f);
	bool bCentered = Wide.Num() == Filled.Num();
	for (int32 Index = 0; bCentered && Index < Filled.Num(); ++Index)
	{
		bCentered = Wide[Index].Center.Equals(Filled[Index].Center + Shift, 0.05f);
	}
	TestTrue(TEXT("the grid sits in the middle of the spare floor"), bCentered);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFortressAveragePlanTest,
	"SolidCore1.Stronghold.FortressAveragePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFortressAveragePlanTest::RunTest(const FString& Parameters)
{
	SolidStronghold::FSolidFortress Fortress;
	Fortress.AddBuildings(ESolidStrongholdSize::Average);
	TestFalse(TEXT("Average stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Average));
	TestEqual(TEXT("Average has one building"), Fortress.Buildings.Num(), 1);
	if (Fortress.Buildings.Num() != 1)
	{
		return false;
	}

	const SolidStronghold::FSolidFortressBuilding& Building = Fortress.Buildings[0];
	const SolidStronghold::FSolidFortressBuilding& Humble = SolidStronghold::FSolidFortress::Instance().Buildings[0];
	TestEqual(TEXT("the building has two floors"), Building.Floors.Num(), 2);
	if (Building.Floors.Num() != 2)
	{
		return false;
	}
	TestTrue(TEXT("the ground floor is small"), Building.Floors[0] == ESolidStrongholdRoom::Small);
	TestTrue(TEXT("the upper floor is medium"), Building.Floors[1] == ESolidStrongholdRoom::Medium);
	TestTrue(TEXT("the doorway faces north"), Building.Facing == ESolidStrongholdOrientation::NorthFace);
	TestTrue(
		TEXT("Average keeps the Humble building"),
		Building.InnerMinCm.Equals(Humble.InnerMinCm, 0.05f)
			&& FMath::IsNearlyEqual(Building.InnerWidthCm, Humble.InnerWidthCm, 0.05f)
			&& FMath::IsNearlyEqual(Building.InnerDepthCm, Humble.InnerDepthCm, 0.05f)
			&& FMath::IsNearlyEqual(Building.DoorFromInnerCm(), Humble.DoorFromInnerCm(), 0.05f)
			&& Building.HasStair() == Humble.HasStair());

	TArray<SolidStrongholdGrid::FHex> Empty;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Average, Empty);
	TestEqual(TEXT("Average has no ground hexes yet"), Empty.Num(), 0);
	return true;
}

#endif
