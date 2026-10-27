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
	TestTrue(
		TEXT("the outer width is the two rooms, the gap, and the walls"),
		FMath::IsNearlyEqual(
			Fortress.OuterWidthCm(),
			Fortress.WallCm + Fortress.RoomWidthCm(Fortress.AnnexColumns) + Fortress.WallCm
				+ Fortress.GapCm + Fortress.WallCm + Fortress.RoomWidthCm(Fortress.KeepColumns) + Fortress.WallCm,
			0.05f));
	TestTrue(
		TEXT("the outer depth is the rooms, the yard, and the walls"),
		FMath::IsNearlyEqual(
			Fortress.OuterDepthCm(),
			Fortress.WallCm + Fortress.RoomDepthCm() + Fortress.WallCm + Fortress.YardDepthCm + Fortress.WallCm,
			0.05f));

	TArray<SolidStrongholdGrid::FHex> Keep;
	TArray<SolidStrongholdGrid::FHex> Annex;
	Fortress.CollectKeepHexes(ESolidStrongholdSize::Humble, Keep);
	Fortress.CollectAnnexHexes(ESolidStrongholdSize::Humble, Annex);
	TestEqual(
		TEXT("the Keep is the medium room"),
		Keep.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium));
	TestEqual(
		TEXT("the Annex is the small room"),
		Annex.Num(),
		SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small));
	TestTrue(
		TEXT("every Keep hex sits inside the Keep"),
		VerticesInside(Keep, Fortress.KeepInnerMinCm(), FVector2D(Fortress.RoomWidthCm(Fortress.KeepColumns), Fortress.RoomDepthCm())));
	TestTrue(
		TEXT("every Annex hex sits inside the Annex"),
		VerticesInside(Annex, Fortress.AnnexInnerMinCm(), FVector2D(Fortress.RoomWidthCm(Fortress.AnnexColumns), Fortress.RoomDepthCm())));

	TArray<SolidStrongholdGrid::FHex> Ground;
	Fortress.CollectGroundHexes(ESolidStrongholdSize::Humble, Ground);
	TestEqual(
		TEXT("the ground floor is the Humble kit"),
		Ground.Num(),
		SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Humble)));
	TestEqual(TEXT("the ground floor is the Keep then the Annex"), Ground.Num(), Keep.Num() + Annex.Num());
	bool bGroundOrder = Ground.Num() == Keep.Num() + Annex.Num();
	for (int32 Index = 0; bGroundOrder && Index < Keep.Num(); ++Index)
	{
		bGroundOrder = Ground[Index].Center.Equals(Keep[Index].Center, 0.01f);
	}
	for (int32 Index = 0; bGroundOrder && Index < Annex.Num(); ++Index)
	{
		bGroundOrder = Ground[Keep.Num() + Index].Center.Equals(Annex[Index].Center, 0.01f);
	}
	TestTrue(TEXT("the ground floor lists the Keep and then the Annex"), bGroundOrder);

	TArray<SolidStrongholdGrid::FHex> Empty;
	Fortress.CollectKeepHexes(ESolidStrongholdSize::Average, Empty);
	TestEqual(TEXT("Average has no Keep hexes"), Empty.Num(), 0);
	Fortress.CollectAnnexHexes(ESolidStrongholdSize::Meagre, Empty);
	TestEqual(TEXT("Meagre has no Annex hexes"), Empty.Num(), 0);
	return true;
}

#endif
