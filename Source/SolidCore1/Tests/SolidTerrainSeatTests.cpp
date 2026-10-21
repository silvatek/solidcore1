#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Strongholds/Dungeon/SolidDungeonExterior.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainSeat.h"
#include "SolidTerrainTestHelpers.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidTerrainSeatTest,
	"SolidCore1.Terrain.Seat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidTerrainSeatTest::RunTest(const FString& Parameters)
{
	const FVector Origin(0.f, 0.f, 0.f);
	SolidTerrainSeat::FFootprint Box;
	Box.HalfXCm = 100.f;
	Box.HalfYCm = 40.f;

	TestTrue(TEXT("the center of a box is inside"),
		SolidTerrainSeat::Contains(Origin, 0.f, Box, 0.f, 0.f));
	TestTrue(TEXT("a box edge is inside"),
		SolidTerrainSeat::Contains(Origin, 0.f, Box, 100.f, 0.f));
	TestFalse(TEXT("past the box on X is outside"),
		SolidTerrainSeat::Contains(Origin, 0.f, Box, 101.f, 0.f));
	TestFalse(TEXT("past the box on Y is outside"),
		SolidTerrainSeat::Contains(Origin, 0.f, Box, 0.f, 41.f));

	TestTrue(TEXT("yaw 90 keeps a point along local X inside"),
		SolidTerrainSeat::Contains(Origin, 90.f, Box, 0.f, 80.f));
	TestFalse(TEXT("yaw 90 puts local Y on world X"),
		SolidTerrainSeat::Contains(Origin, 90.f, Box, 80.f, 0.f));

	SolidTerrainSeat::FFootprint Shifted;
	Shifted.LocalCenter = FVector2D(80.f, 0.f);
	Shifted.HalfXCm = 10.f;
	Shifted.HalfYCm = 10.f;
	TestTrue(TEXT("a shifted footprint contains its center"),
		SolidTerrainSeat::Contains(Origin, 0.f, Shifted, 80.f, 0.f));
	TestFalse(TEXT("a shifted footprint leaves the actor origin"),
		SolidTerrainSeat::Contains(Origin, 0.f, Shifted, 0.f, 0.f));

	SolidTerrainSeat::FFootprint Round;
	Round.bRound = true;
	Round.HalfXCm = 50.f;
	Round.HalfYCm = 500.f;
	TestTrue(TEXT("a round footprint contains a point inside the radius"),
		SolidTerrainSeat::Contains(Origin, 0.f, Round, 30.f, 30.f));
	TestTrue(TEXT("a round footprint contains the rim"),
		SolidTerrainSeat::Contains(Origin, 0.f, Round, 50.f, 0.f));
	TestFalse(TEXT("a round footprint ignores the other half-extent"),
		SolidTerrainSeat::Contains(Origin, 0.f, Round, 0.f, 100.f));

	USolidTerrainMap* Map = SolidTerrainTestHelpers::MakeSmallMap();
	TestNotNull(TEXT("terrain"), Map);
	if (!Map)
	{
		return false;
	}

	SolidTerrainSeat::FFootprint Point;
	const float AtOrigin = Map->SampleHeight(0.f, 0.f);
	TestTrue(TEXT("a point footprint is the sample under the actor"),
		FMath::IsNearlyEqual(SolidTerrainSeat::LowestCm(Map, Origin, 0.f, Point), AtOrigin, 0.01f));

	const FVector ShiftedActor(120.f, -80.f, 0.f);
	TestTrue(TEXT("a point footprint follows the actor"),
		FMath::IsNearlyEqual(
			SolidTerrainSeat::LowestCm(Map, ShiftedActor, 40.f, Point),
			Map->SampleHeight(ShiftedActor.X, ShiftedActor.Y),
			0.01f));

	SolidTerrainSeat::FFootprint OffsetPoint;
	OffsetPoint.LocalCenter = FVector2D(300.f, 0.f);
	TestTrue(TEXT("a local center is sampled in the actor frame"),
		FMath::IsNearlyEqual(
			SolidTerrainSeat::LowestCm(Map, Origin, 0.f, OffsetPoint),
			Map->SampleHeight(300.f, 0.f),
			0.01f));
	TestTrue(TEXT("yaw turns the local center"),
		FMath::IsNearlyEqual(
			SolidTerrainSeat::LowestCm(Map, Origin, 90.f, OffsetPoint),
			Map->SampleHeight(0.f, 300.f),
			0.01f));

	SolidTerrainSeat::FFootprint Wide;
	Wide.HalfXCm = 450.f;
	Wide.HalfYCm = 300.f;
	const FVector WideActor(200.f, -400.f, 0.f);
	const float WideLowest = SolidTerrainSeat::LowestCm(Map, WideActor, 0.f, Wide);
	const float WideCenter = Map->SampleHeight(WideActor.X, WideActor.Y);
	TestTrue(TEXT("a box is at least as low as its center"), WideLowest <= WideCenter + 0.01f);

	float Known = WideCenter;
	const FVector2D Corners[4] = {
		FVector2D(-Wide.HalfXCm, -Wide.HalfYCm),
		FVector2D(Wide.HalfXCm, -Wide.HalfYCm),
		FVector2D(Wide.HalfXCm, Wide.HalfYCm),
		FVector2D(-Wide.HalfXCm, Wide.HalfYCm),
	};
	for (const FVector2D& Corner : Corners)
	{
		Known = FMath::Min(Known, Map->SampleHeight(WideActor.X + Corner.X, WideActor.Y + Corner.Y));
	}
	const float Step = SolidTerrainSeat::SampleStepCm(Map->PointSpacing);
	for (int32 Edge = 0; Edge < 4; ++Edge)
	{
		const FVector2D Start = Corners[Edge];
		const FVector2D End = Corners[(Edge + 1) % 4];
		const float Length = FVector2D::Distance(Start, End);
		const int32 Pieces = FMath::Max(1, FMath::CeilToInt(Length / Step));
		for (int32 Piece = 0; Piece <= Pieces; ++Piece)
		{
			const float Along = static_cast<float>(Piece) / static_cast<float>(Pieces);
			const FVector2D Sample = FMath::Lerp(Start, End, Along);
			Known = FMath::Min(Known, Map->SampleHeight(WideActor.X + Sample.X, WideActor.Y + Sample.Y));
		}
	}
	for (int32 IndexY = 0; IndexY < Map->GridHeight; ++IndexY)
	{
		for (int32 IndexX = 0; IndexX < Map->GridWidth; ++IndexX)
		{
			const FSolidTerrainPoint& TerrainPoint = Map->GetPoint(IndexX, IndexY);
			if (SolidTerrainSeat::Contains(WideActor, 0.f, Wide, TerrainPoint.X, TerrainPoint.Y))
			{
				Known = FMath::Min(Known, TerrainPoint.Height);
			}
		}
	}
	TestTrue(TEXT("a level box uses the lowest sample under it"),
		FMath::IsNearlyEqual(WideLowest, Known, 0.05f));

	const float Bias = 3.f;
	const FVector Seated(0.f, 0.f, AtOrigin + Bias + 4.f);
	const float Drop = SolidTerrainSeat::DropCm(Map, Bias, Seated, 0.f, Point);
	TestTrue(TEXT("the bottom is 10 cm under the surface"),
		FMath::IsNearlyEqual(Seated.Z - Drop, AtOrigin + Bias - SolidTerrainSeat::BurialCm, 0.01f));

	const FVector AlreadyUnder(0.f, 0.f, AtOrigin + Bias - SolidTerrainSeat::BurialCm - 20.f);
	TestTrue(TEXT("an origin already below the burial is not lifted"),
		FMath::IsNearlyZero(SolidTerrainSeat::DropCm(Map, Bias, AlreadyUnder, 0.f, Point)));
	TestTrue(TEXT("missing terrain does not bury"),
		FMath::IsNearlyZero(SolidTerrainSeat::DropCm(nullptr, Bias, Seated, 0.f, Point)));

	USolidTerrainMap* Bare = NewObject<USolidTerrainMap>();
	TestTrue(TEXT("an unbuilt map does not bury"),
		Bare && FMath::IsNearlyZero(SolidTerrainSeat::DropCm(Bare, Bias, Seated, 0.f, Point)));

	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(GetTransientPackage());
	TestNotNull(TEXT("part"), Part);
	if (Part)
	{
		Part->SetRelativeLocation(FVector(12.f, -4.f, 160.f));
		Part->SetRelativeScale3D(FVector(2.f, 3.f, 3.2f));
		SolidTerrainSeat::BuryCentered(Part, 0.f, 320.f, 0.f, 100.f);
		TestTrue(TEXT("a zero drop leaves the part"),
			Part->GetRelativeLocation().Equals(FVector(12.f, -4.f, 160.f), 0.01f)
				&& Part->GetRelativeScale3D().Equals(FVector(2.f, 3.f, 3.2f), 0.01f));

		SolidTerrainSeat::BuryCentered(Part, 0.f, 320.f, 40.f, 100.f);
		const FVector BuriedLocation = Part->GetRelativeLocation();
		const FVector BuriedScale = Part->GetRelativeScale3D();
		const float Top = BuriedLocation.Z + BuriedScale.Z * 50.f;
		const float Bottom = BuriedLocation.Z - BuriedScale.Z * 50.f;
		TestTrue(TEXT("burial keeps the part's plan"),
			FMath::IsNearlyEqual(BuriedLocation.X, 12.f, 0.01f)
				&& FMath::IsNearlyEqual(BuriedLocation.Y, -4.f, 0.01f)
				&& FMath::IsNearlyEqual(BuriedScale.X, 2.f, 0.01f)
				&& FMath::IsNearlyEqual(BuriedScale.Y, 3.f, 0.01f));
		TestTrue(TEXT("the top stays"), FMath::IsNearlyEqual(Top, 320.f, 0.01f));
		TestTrue(TEXT("the bottom drops by the burial"), FMath::IsNearlyEqual(Bottom, -40.f, 0.01f));
	}

	TestTrue(TEXT("the dungeon ground edge uses the same burial"),
		FMath::IsNearlyEqual(ASolidDungeonExterior::GroundBurialCm, SolidTerrainSeat::BurialCm, 0.01f));

	return true;
}

#endif
