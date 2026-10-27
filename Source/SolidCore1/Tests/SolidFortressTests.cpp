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

#endif
