#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Strongholds/Tower/SolidTower.h"

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
		float Width = Fortress.WallCm;
		for (int32 Index = 0; Index < Fortress.Buildings.Num(); ++Index)
		{
			Width += Fortress.Buildings[Index].WidthCm() + Fortress.WallCm;
			if (Index + 1 < Fortress.Buildings.Num())
			{
				Width += Fortress.GapCm + Fortress.WallCm;
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
	const SolidStronghold::FPlan Tower;

	TestTrue(TEXT("Humble is supported"), Fortress.Supported(ESolidStrongholdSize::Humble));
	TestFalse(TEXT("Meagre stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Meagre));
	TestFalse(TEXT("Average stays unsupported"), Fortress.Supported(ESolidStrongholdSize::Average));
	TestEqual(TEXT("the wall is the tower wall"), Fortress.WallCm, Tower.WallCm);
	TestEqual(TEXT("the storey is the tower pitch"), Fortress.StoreyCm, Tower.FloorPitchCm);
	TestEqual(TEXT("the opening is the tower door"), Fortress.OpeningCm, Tower.DoorWidthCm);
	TestEqual(TEXT("the parapet is one hex across"), Fortress.ParapetCm, SolidStrongholdGrid::FlatToFlatCm);
	TestEqual(TEXT("the curtain is the storey plus the parapet"), Fortress.CurtainHeightCm(), Fortress.StoreyCm + Fortress.ParapetCm);
	TestEqual(TEXT("Humble has two buildings"), Fortress.Buildings.Num(), 2);
	TestTrue(
		TEXT("the outer width is the buildings, the gap, and the walls"),
		FMath::IsNearlyEqual(Fortress.OuterWidthCm(), LaidWidthCm(Fortress), 0.05f));
	TestTrue(
		TEXT("the outer depth is the rooms, the yard, and the walls"),
		FMath::IsNearlyEqual(
			Fortress.OuterDepthCm(),
			Fortress.WallCm + Fortress.Buildings[0].DepthCm() + Fortress.WallCm + Fortress.YardDepthCm + Fortress.WallCm,
			0.05f));

	const SolidStronghold::FSolidFortressBuilding* Keep = Fortress.Building(SolidStronghold::ESolidFortressBuilding::Keep);
	const SolidStronghold::FSolidFortressBuilding* Annex = Fortress.Building(SolidStronghold::ESolidFortressBuilding::Annex);
	TestNotNull(TEXT("the Keep"), Keep);
	TestNotNull(TEXT("the Annex"), Annex);
	if (!Keep || !Annex)
	{
		return false;
	}

	TArray<SolidStrongholdGrid::FHex> KeepHexes;
	TArray<SolidStrongholdGrid::FHex> AnnexHexes;
	Keep->CollectHexes(KeepHexes);
	Annex->CollectHexes(AnnexHexes);
	TestEqual(
		TEXT("the Keep is the medium room"),
		KeepHexes.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium));
	TestEqual(
		TEXT("the Annex is the small room"),
		AnnexHexes.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TestTrue(
		TEXT("every Keep hex sits inside the Keep"),
		VerticesInside(KeepHexes, Keep->InnerMinCm, FVector2D(Keep->WidthCm(), Keep->DepthCm())));
	TestTrue(
		TEXT("every Annex hex sits inside the Annex"),
		VerticesInside(AnnexHexes, Annex->InnerMinCm, FVector2D(Annex->WidthCm(), Annex->DepthCm())));

	TArray<SolidStrongholdGrid::FHex> Ground;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Humble, Ground);
	TestEqual(
		TEXT("the ground floor is the Humble kit"),
		Ground.Num(),
		SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Humble)));

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
