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
		TEXT("the curtain is the storey plus the parapet"),
		Fortress.CurtainHeightCm(),
		FSolidStronghold::StoreyCm + Fortress.ParapetCm);
	TestEqual(TEXT("Humble has two buildings"), Fortress.Buildings.Num(), 2);
	TestTrue(
		TEXT("the outer width is the buildings and the walls"),
		FMath::IsNearlyEqual(Fortress.OuterWidthCm(), LaidWidthCm(Fortress), 0.05f));
	TestTrue(
		TEXT("the outer depth is the rooms, the yard, and the walls"),
		FMath::IsNearlyEqual(
			Fortress.OuterDepthCm(),
			FSolidStronghold::WallCm + Fortress.Buildings[0].DepthCm() + FSolidStronghold::WallCm + Fortress.YardDepthCm + FSolidStronghold::WallCm,
			0.05f));

	const SolidStronghold::FSolidFortressBuilding* Keep = Fortress.Keep();
	const SolidStronghold::FSolidFortressBuilding* Other = nullptr;
	int32 Keeps = 0;
	for (const SolidStronghold::FSolidFortressBuilding& One : Fortress.Buildings)
	{
		if (One.bKeep)
		{
			++Keeps;
		}
		else
		{
			Other = &One;
		}
	}
	TestEqual(TEXT("one building is the keep"), Keeps, 1);
	TestNotNull(TEXT("the Keep"), Keep);
	TestNotNull(TEXT("the other building"), Other);
	if (!Keep || !Other)
	{
		return false;
	}

	TArray<SolidStrongholdGrid::FHex> KeepHexes;
	TArray<SolidStrongholdGrid::FHex> OtherHexes;
	Keep->CollectHexes(KeepHexes);
	Other->CollectHexes(OtherHexes);
	TestEqual(
		TEXT("the Keep is the medium room"),
		KeepHexes.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium));
	TestEqual(
		TEXT("the other building is the small room"),
		OtherHexes.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TestTrue(
		TEXT("every Keep hex sits inside the Keep"),
		VerticesInside(KeepHexes, Keep->InnerMinCm, FVector2D(Keep->WidthCm(), Keep->DepthCm())));
	TestTrue(
		TEXT("every other hex sits inside that building"),
		VerticesInside(OtherHexes, Other->InnerMinCm, FVector2D(Other->WidthCm(), Other->DepthCm())));
	TestTrue(
		TEXT("the Annex meets the Keep"),
		FMath::IsNearlyEqual(
			Other->InnerMinCm.X + Other->WidthCm() + 2.f * FSolidStronghold::WallCm,
			Keep->InnerMinCm.X,
			0.05f));

	TArray<SolidStrongholdGrid::FHex> Ground;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Humble, Ground);
	TestEqual(
		TEXT("the ground floor is the Humble kit"),
		Ground.Num(),
		SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Humble)));
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
	Room.Columns = 4;
	Room.Rows = 3;
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
	Fortress.Place(ESolidStrongholdSize::Average);
	TestFalse(TEXT("Average stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Average));
	TestEqual(TEXT("Average has three rooms"), Fortress.Buildings.Num(), 3);

	const SolidStronghold::FSolidFortressBuilding* Keep = Fortress.Keep();
	TestNotNull(TEXT("the keep"), Keep);
	if (!Keep || Fortress.Buildings.Num() != 3)
	{
		return false;
	}

	const SolidStronghold::FSolidFortressBuilding& West = Fortress.Buildings[0];
	const SolidStronghold::FSolidFortressBuilding& East = Fortress.Buildings[2];
	TestTrue(TEXT("the middle room is the keep"), Keep == &Fortress.Buildings[1]);
	TestFalse(TEXT("the west room is not the keep"), West.bKeep);
	TestFalse(TEXT("the east room is not the keep"), East.bKeep);

	TArray<SolidStrongholdGrid::FHex> WestHexes;
	TArray<SolidStrongholdGrid::FHex> KeepHexes;
	TArray<SolidStrongholdGrid::FHex> EastHexes;
	West.CollectHexes(WestHexes);
	Keep->CollectHexes(KeepHexes);
	East.CollectHexes(EastHexes);
	TestEqual(TEXT("the west room is small"), WestHexes.Num(), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TestEqual(TEXT("the keep is medium"), KeepHexes.Num(), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium));
	TestEqual(TEXT("the east room is small"), EastHexes.Num(), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));

	const float Wall = FSolidStronghold::WallCm;
	TestTrue(
		TEXT("the keep is in the south-west corner"),
		Keep->InnerMinCm.Equals(FVector2D(Wall, Wall), 0.05f));
	TestTrue(
		TEXT("the west room meets the keep's north wall"),
		FMath::IsNearlyEqual(West.InnerMinCm.Y, Keep->InnerMinCm.Y + Keep->DepthCm() + 2.f * Wall, 0.05f));
	TestTrue(
		TEXT("the east room meets the keep's east wall"),
		FMath::IsNearlyEqual(East.InnerMinCm.X, Keep->InnerMinCm.X + Keep->WidthCm() + 2.f * Wall, 0.05f)
		&& FMath::IsNearlyEqual(East.InnerMinCm.Y, Keep->InnerMinCm.Y, 0.05f));
	TestTrue(
		TEXT("the outer wall is square"),
		FMath::IsNearlyEqual(Fortress.OuterWidthCm(), Fortress.OuterDepthCm(), 0.05f));

	TArray<SolidStrongholdGrid::FHex> Empty;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Average, Empty);
	TestEqual(TEXT("Average has no ground hexes yet"), Empty.Num(), 0);
	return true;
}

#endif
