#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidMonsterActor.h"
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

	TestFalse(TEXT("a closed visit has no monsters"), SolidMonster::WorldHasMonsters(true));
	TestTrue(TEXT("the open world can hold monsters"), SolidMonster::WorldHasMonsters(false));
	TestEqual(TEXT("standing height"), SolidMonster::StandingHeightCm, 220.f);
	TestTrue(TEXT("a 110 cm mesh doubles"), FMath::IsNearlyEqual(SolidMonster::UniformScale(110.f), 2.f));
	TestTrue(TEXT("a 220 cm mesh stays"), FMath::IsNearlyEqual(SolidMonster::UniformScale(220.f), 1.f));
	TestTrue(TEXT("a zero height stays at one"), FMath::IsNearlyEqual(SolidMonster::UniformScale(0.f), 1.f));
	TestTrue(
		TEXT("a centered mesh drops its bottom to the feet"),
		FMath::IsNearlyEqual(SolidMonster::MeshFootOffsetCm(0.f, 100.f, 1.f), 100.f));

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

	TArray<SolidMonster::FChoice> Choices;
	TestEqual(
		TEXT("three reptiles"),
		SolidMonster::CollectChoices(Points, Trees, Forage, Iglin, Bridges, Choices),
		SolidMonster::Count);
	if (Choices.Num() != 3)
	{
		return false;
	}

	const int32 Expected[] = { 1, 6, 8 };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestEqual(TEXT("nearest grassland in the band"), Choices[Index].PointIndex, Expected[Index]);
		TestTrue(
			TEXT("the place is that point"),
			Choices[Index].XY.Equals(FVector2D(Points[Expected[Index]].X, Points[Expected[Index]].Y)));
	}
	for (int32 Index = 1; Index < Choices.Num(); ++Index)
	{
		TestTrue(
			TEXT("later places stay 4 m apart"),
			FVector2D::Distance(Choices[Index].XY, Choices[Index - 1].XY) >= SolidMonster::SeparationCm);
	}

	const FVector2D Outer(0.f, 0.f);
	for (const SolidMonster::FChoice& Choice : Choices)
	{
		const float Yaw = FMath::DegreesToRadians(Choice.YawDeg);
		const FVector2D Facing(FMath::Cos(Yaw), FMath::Sin(Yaw));
		const FVector2D ToOuter = (Outer - Choice.XY).GetSafeNormal();
		TestTrue(TEXT("yaw looks at the Iglin ramp"), FVector2D::DotProduct(Facing, ToOuter) > 0.99f);
		TestTrue(TEXT("a skipped point is not chosen"), Choice.PointIndex != 0 && Choice.PointIndex != 2
			&& Choice.PointIndex != 3 && Choice.PointIndex != 4 && Choice.PointIndex != 5
			&& Choice.PointIndex != 7 && Choice.PointIndex != 9);
	}

	TArray<SolidMonster::FChoice> Repaired;
	Bridges[0].bBroken = false;
	TestEqual(
		TEXT("a repair keeps the same places"),
		SolidMonster::CollectChoices(Points, Trees, Forage, Iglin, Bridges, Repaired),
		Choices.Num());
	for (int32 Index = 0; Index < Choices.Num() && Index < Repaired.Num(); ++Index)
	{
		TestEqual(TEXT("the repaired bridge uses the same point"), Repaired[Index].PointIndex, Choices[Index].PointIndex);
	}

	TArray<SolidMonster::FChoice> None;
	TArray<SolidBridge::FBridge> NoBridge;
	TestEqual(
		TEXT("no Iglin bridge places none"),
		SolidMonster::CollectChoices(Points, Trees, Forage, Iglin, NoBridge, None),
		0);
	TestEqual(TEXT("the list is empty"), None.Num(), 0);

	TArray<SolidBridge::FBridge> Other;
	Other.Add(IglinBridge(true));
	Other[0].bIglinRelion = false;
	TestEqual(
		TEXT("another crossing places none"),
		SolidMonster::CollectChoices(Points, Trees, Forage, Iglin, Other, None),
		0);

	TArray<FSolidTerrainPoint> OnePoint;
	OnePoint.Add(PointAt(900.f, ESolidBiome::Grassland));
	TArray<int32> NoSkip;
	TArray<SolidMonster::FChoice> Few;
	TestEqual(
		TEXT("one eligible point places one"),
		SolidMonster::CollectChoices(OnePoint, NoSkip, NoSkip, Iglin, Bridges, Few),
		1);

	TArray<int32> Ready;
	Points[1].Fog = 1.f;
	Points[6].Fog = 0.5f;
	Points[8].Fog = 0.f;
	const TArray<int32> Seeded = { 2 };
	SolidMonster::CollectReadyToSeed(Points, Choices, Seeded, Ready);
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
		TEXT("the capsule radius is 60 cm"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), SolidMonster::CapsuleRadiusCm));
	TestTrue(
		TEXT("the capsule is half the standing height"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), SolidMonster::CapsuleHalfHeightCm));
	TestTrue(
		TEXT("the capsule bottom is the actor origin"),
		FMath::IsNearlyEqual(Capsule->GetRelativeLocation().Z, SolidMonster::CapsuleHalfHeightCm));
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

#endif // WITH_DEV_AUTOMATION_TESTS
