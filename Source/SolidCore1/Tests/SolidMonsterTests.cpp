#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidMonsterGroup.h"
#include "Monster/SolidMonsterActor.h"
#include "Monster/SolidRaptor.h"
#include "Game/SolidMonsterMeasure.h"
#include "Terrain/SolidBridge.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidMonsterTestPrivate
{
	FSolidTerrainPoint PointAt(const float X, const ESolidBiome Biome, const float Fog = 1.f)
	{
		FSolidTerrainPoint Point;
		Point.X = X;
		Point.Y = 0.f;
		Point.Biome = Biome;
		Point.Fog = Fog;
		return Point;
	}

	SolidBridge::FBridge IglinBridge(const bool bBroken)
	{
		SolidBridge::FBridge Bridge;
		Bridge.bIglinRelion = true;
		Bridge.bBroken = bBroken;
		Bridge.WidthCm = 400.f;
		Bridge.Deck.Add(FVector(5000.f, 0.f, 100.f));
		Bridge.Deck.Add(FVector(6000.f, 0.f, 100.f));
		Bridge.RampFrom.Add(FVector(5000.f, 0.f, 100.f));
		Bridge.RampFrom.Add(FVector(0.f, 0.f, 0.f));
		Bridge.RampTo.Add(FVector(6000.f, 0.f, 100.f));
		Bridge.RampTo.Add(FVector(30000.f, 0.f, 0.f));
		return Bridge;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterChoiceTest,
	"SolidCore1.Monster.Choice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterChoiceTest::RunTest(const FString& Parameters)
{
	using namespace SolidMonsterTestPrivate;

	TestFalse(TEXT("a closed visit has no monsters"), FSolidMonsterGroup::WorldHasMonsters(true));
	TestTrue(TEXT("the open world can hold monsters"), FSolidMonsterGroup::WorldHasMonsters(false));
	TestTrue(
		TEXT("a mesh of half the standing height doubles"),
		FMath::IsNearlyEqual(FSolidMonster::UniformScale(FSolidMonster::StandingHeightCm * 0.5f), 2.f));
	TestTrue(
		TEXT("a mesh of the standing height stays"),
		FMath::IsNearlyEqual(FSolidMonster::UniformScale(FSolidMonster::StandingHeightCm), 1.f));
	TestTrue(TEXT("a zero height stays at one"), FMath::IsNearlyEqual(FSolidMonster::UniformScale(0.f), 1.f));
	TestTrue(
		TEXT("a centered mesh drops its bottom to the feet"),
		FMath::IsNearlyEqual(FSolidMonster::MeshFootOffsetCm(0.f, 100.f, 1.f), 100.f));
	TestTrue(
		TEXT("a raptor draws six times the span scale"),
		FMath::IsNearlyEqual(FSolidRaptor().DrawnBodyFactor(), FSolidRaptor::SmallDrawnBodyFactor));

	TArray<float> BodyZ;
	for (int32 Sample = 0; Sample < 20; ++Sample)
	{
		BodyZ.Add(Sample * 10.f);
	}
	const FSolidMonsterStandingSpan Short = FSolidMonster::StandingSpan(BodyZ);
	TestTrue(TEXT("a short body keeps its full height"), FMath::IsNearlyEqual(Short.Height, 190.f));
	TestTrue(TEXT("a short body keeps its feet"), FMath::IsNearlyEqual(Short.BottomZ, 0.f));

	TArray<float> OutlierZ;
	for (int32 Sample = 0; Sample < 100; ++Sample)
	{
		OutlierZ.Add(Sample * 1.8f);
	}
	OutlierZ.Add(50000.f);
	const FSolidMonsterStandingSpan Trimmed = FSolidMonster::StandingSpan(OutlierZ);
	TestTrue(TEXT("one far sample does not set the height"), Trimmed.Height < 500.f);
	TestTrue(TEXT("the body height remains"), Trimmed.Height > 100.f);

	TArray<float> Scattered;
	for (int32 Sample = 0; Sample < 80; ++Sample)
	{
		Scattered.Add(Sample * 2.f);
	}
	for (int32 Sample = 0; Sample < 20; ++Sample)
	{
		Scattered.Add(8000.f);
	}
	const FSolidMonsterStandingSpan Dense = FSolidMonster::StandingSpan(Scattered);
	TestTrue(TEXT("a far cloud does not set the height"), Dense.Height < 400.f);
	TestTrue(TEXT("the dense band is the body"), Dense.Height > 100.f);

	TArray<float> EmptyZ;
	TestTrue(TEXT("no samples have no height"), FSolidMonster::StandingSpan(EmptyZ).Height == 0.f);

	TArray<FSolidTerrainPoint> Points;
	Points.Add(PointAt(500.f, ESolidBiome::Grassland));
	Points.Add(PointAt(900.f, ESolidBiome::Grassland));
	Points.Add(PointAt(1000.f, ESolidBiome::Road));
	Points.Add(PointAt(1100.f, ESolidBiome::River));
	Points.Add(PointAt(1400.f, ESolidBiome::Grassland));
	Points.Add(PointAt(1600.f, ESolidBiome::Grassland));
	Points.Add(PointAt(1800.f, ESolidBiome::Grassland));
	Points.Add(PointAt(2000.f, ESolidBiome::Grassland));
	Points.Add(PointAt(2300.f, ESolidBiome::Grassland));
	Points.Add(PointAt(4500.f, ESolidBiome::Grassland));
	Points.Add(PointAt(3000.f, ESolidBiome::Grassland));

	const TArray<int32> Trees = { 4 };
	const TArray<int32> Forage = { 5 };
	const FVector2D Iglin(-20000.f, 0.f);
	TArray<SolidBridge::FBridge> Bridges;
	Bridges.Add(IglinBridge(true));

	FSolidMonsterGroup Group;
	TestEqual(
		TEXT("three reptiles"),
		Group.PlaceBesideIglinRamp(Points, Trees, Forage, Iglin, Bridges),
		FSolidMonsterGroup::BesideIglinCount);
	if (Group.Num() != 3)
	{
		return false;
	}

	const int32 Expected[] = { 1, 6, 8 };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FSolidMonster* Member = Group.Get(Index);
		TestNotNull(TEXT("member"), Member);
		if (!Member)
		{
			return false;
		}
		TestEqual(TEXT("nearest grassland in the band"), Member->PointIndex, Expected[Index]);
		TestTrue(
			TEXT("the place is that point"),
			Member->XY.Equals(FVector2D(Points[Expected[Index]].X, Points[Expected[Index]].Y)));
		TestTrue(
			TEXT("each member is a raptor"),
			FCString::Strcmp(Member->Kind(), TEXT("Raptor")) == 0);
	}
	for (int32 Index = 1; Index < Group.Num(); ++Index)
	{
		const FSolidMonster* Prev = Group.Get(Index - 1);
		const FSolidMonster* Cur = Group.Get(Index);
		TestTrue(
			TEXT("later places stay 4 m apart"),
			Prev && Cur
				&& FVector2D::Distance(Cur->XY, Prev->XY) >= FSolidMonsterGroup::SeparationCm);
	}

	const FVector2D Outer(0.f, 0.f);
	for (int32 Index = 0; Index < Group.Num(); ++Index)
	{
		const FSolidMonster* Member = Group.Get(Index);
		if (!Member)
		{
			continue;
		}
		const float Yaw = FMath::DegreesToRadians(Member->YawDeg);
		const FVector2D Facing(FMath::Cos(Yaw), FMath::Sin(Yaw));
		const FVector2D ToOuter = (Outer - Member->XY).GetSafeNormal();
		TestTrue(TEXT("yaw looks at the Iglin ramp"), FVector2D::DotProduct(Facing, ToOuter) > 0.99f);
		TestTrue(TEXT("a skipped point is not chosen"), Member->PointIndex != 0 && Member->PointIndex != 2
			&& Member->PointIndex != 3 && Member->PointIndex != 4 && Member->PointIndex != 5
			&& Member->PointIndex != 7 && Member->PointIndex != 9);
	}

	FSolidMonsterGroup Repaired;
	Bridges[0].bBroken = false;
	TestEqual(
		TEXT("a repair keeps the same places"),
		Repaired.PlaceBesideIglinRamp(Points, Trees, Forage, Iglin, Bridges),
		Group.Num());
	for (int32 Index = 0; Index < Group.Num() && Index < Repaired.Num(); ++Index)
	{
		const FSolidMonster* Was = Group.Get(Index);
		const FSolidMonster* Now = Repaired.Get(Index);
		TestTrue(
			TEXT("the repaired bridge uses the same point"),
			Was && Now && Now->PointIndex == Was->PointIndex);
	}

	FSolidMonsterGroup None;
	TArray<SolidBridge::FBridge> NoBridge;
	TestEqual(
		TEXT("no Iglin bridge places none"),
		None.PlaceBesideIglinRamp(Points, Trees, Forage, Iglin, NoBridge),
		0);
	TestEqual(TEXT("the list is empty"), None.Num(), 0);

	TArray<SolidBridge::FBridge> Other;
	Other.Add(IglinBridge(true));
	Other[0].bIglinRelion = false;
	TestEqual(
		TEXT("another crossing places none"),
		None.PlaceBesideIglinRamp(Points, Trees, Forage, Iglin, Other),
		0);

	TArray<FSolidTerrainPoint> OnePoint;
	OnePoint.Add(PointAt(900.f, ESolidBiome::Grassland));
	TArray<int32> NoSkip;
	FSolidMonsterGroup Few;
	TestEqual(
		TEXT("one eligible point places one"),
		Few.PlaceBesideIglinRamp(OnePoint, NoSkip, NoSkip, Iglin, Bridges),
		1);

	TArray<int32> Ready;
	Points[1].Fog = 1.f;
	Points[6].Fog = 0.5f;
	Points[8].Fog = 0.f;
	const TArray<int32> Seeded = { 2 };
	Group.CollectReadyToSeed(Points, Seeded, Ready);
	TestEqual(TEXT("one clear point is ready"), Ready.Num(), 1);
	TestFalse(TEXT("the fogged point waits"), Ready.Contains(0));
	TestFalse(TEXT("the seeded point is not seeded again"), Ready.Contains(2));
	TestTrue(TEXT("the clear point seeds"), Ready.Contains(1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterBodyTest,
	"SolidCore1.Monster.Body",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterBodyTest::RunTest(const FString& Parameters)
{
	ASolidMonster* Monster = NewObject<ASolidMonster>();
	TestNotNull(TEXT("monster"), Monster);
	if (!Monster)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Monster->GetCapsule();
	TestNotNull(TEXT("capsule"), Capsule);
	if (!Capsule)
	{
		return false;
	}
	TestTrue(
		TEXT("the capsule radius matches the body"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), FSolidMonster::CapsuleRadiusCm));
	TestTrue(
		TEXT("the capsule radius fits under the half height"),
		FSolidMonster::CapsuleRadiusCm <= FSolidMonster::CapsuleHalfHeightCm);
	TestTrue(
		TEXT("the capsule is half the standing height"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), FSolidMonster::CapsuleHalfHeightCm));
	TestTrue(
		TEXT("the capsule bottom is the actor origin"),
		FMath::IsNearlyEqual(Capsule->GetRelativeLocation().Z, FSolidMonster::CapsuleHalfHeightCm));
	TestTrue(TEXT("the capsule blocks a pawn"), Capsule->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block);
	TestTrue(TEXT("the capsule ignores a click"), Capsule->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Ignore);
	TestFalse(TEXT("a monster takes no damage"), Monster->CanBeDamaged());

	const USkeletalMeshComponent* Body = Monster->GetBody();
	TestNotNull(TEXT("body"), Body);
	if (Body)
	{
		TestTrue(TEXT("the mesh does not block"), Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterMeasureTest,
	"SolidCore1.Monster.Measure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterMeasureTest::RunTest(const FString& Parameters)
{
	ASolidMonsterMeasure* Measure = NewObject<ASolidMonsterMeasure>();
	TestNotNull(TEXT("measure"), Measure);
	if (!Measure)
	{
		return false;
	}
	Measure->BuildVisuals();

	const UStaticMeshComponent* Red = Measure->GetRedPost();
	const UStaticMeshComponent* Blue = Measure->GetBluePost();
	TestNotNull(TEXT("red post"), Red);
	TestNotNull(TEXT("blue post"), Blue);
	if (!Red || !Blue)
	{
		return false;
	}

	TestTrue(
		TEXT("the red post is 100 cm"),
		FMath::IsNearlyEqual(Red->GetRelativeScale3D().Z * 100.f, SolidMonsterMeasure::RedPostCm));
	TestTrue(
		TEXT("the red post stands on the ground"),
		FMath::IsNearlyEqual(Red->GetRelativeLocation().Z, SolidMonsterMeasure::RedPostCm * 0.5f));
	TestTrue(
		TEXT("the red post stands to the right"),
		FMath::IsNearlyEqual(Red->GetRelativeLocation().Y, SolidMonsterMeasure::RedPostSideCm));
	TestTrue(TEXT("the red post does not block"), Red->GetCollisionEnabled() == ECollisionEnabled::NoCollision);

	TestTrue(
		TEXT("the blue post is 200 cm"),
		FMath::IsNearlyEqual(Blue->GetRelativeScale3D().Z * 100.f, SolidMonsterMeasure::BluePostCm));
	TestTrue(
		TEXT("the blue post stands on the ground"),
		FMath::IsNearlyEqual(Blue->GetRelativeLocation().Z, SolidMonsterMeasure::BluePostCm * 0.5f));
	TestTrue(
		TEXT("the blue post stands further right"),
		FMath::IsNearlyEqual(Blue->GetRelativeLocation().Y, SolidMonsterMeasure::BluePostSideCm));

	const UStaticMeshComponent* First = Measure->GetGridLine(0);
	const UStaticMeshComponent* Next = Measure->GetGridLine(1);
	TestNotNull(TEXT("first grid line"), First);
	TestNotNull(TEXT("next grid line"), Next);
	if (!First || !Next)
	{
		return false;
	}
	TestTrue(
		TEXT("the grid starts 4 m from the feet"),
		FMath::IsNearlyEqual(First->GetRelativeLocation().Y, -SolidMonsterMeasure::GridHalfCm));
	TestTrue(
		TEXT("the grid steps 100 cm"),
		FMath::IsNearlyEqual(
			Next->GetRelativeLocation().Y - First->GetRelativeLocation().Y,
			SolidMonsterMeasure::GridStepCm));
	TestTrue(
		TEXT("a grid line does not block"),
		First->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	TestNull(TEXT("a missing grid line"), Measure->GetGridLine(SolidMonsterMeasure::GridLineCount));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
