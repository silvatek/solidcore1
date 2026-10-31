#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidMonsterGroup.h"
#include "Events/SolidEncounter.h"
#include "Events/SolidEncounters.h"
#include "Events/SolidEvents.h"
#include "SolidGameState.h"
#include "Monster/SolidRaptorActor.h"
#include "Monster/SolidRaptor.h"
#include "Inventory/SolidItem.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidWorldChart.h"

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterChoiceTest,
	"SolidCore1.Monster.Choice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterChoiceTest::RunTest(const FString& Parameters)
{
	using namespace SolidMonsterTestPrivate;

	TestFalse(TEXT("a closed visit has no encounters"), SolidEncounter::WorldHasEncounters(true));
	TestTrue(TEXT("the open world can hold encounters"), SolidEncounter::WorldHasEncounters(false));
	TestEqual(TEXT("the catalog has the rows that have shipped"), SolidEncounter::DefinitionCount, 2);
	TestTrue(
		TEXT("Iglin Bridge stays first"),
		SolidEncounter::DefinitionCount > 0
			&& FCString::Strcmp(SolidEncounter::StaticEncounters[0].Name, TEXT("Iglin Bridge")) == 0);
	if (SolidEncounter::DefinitionCount > 0)
	{
		const SolidEncounter::FStaticEncounter& Iglin = SolidEncounter::StaticEncounters[0];
		TestEqual(TEXT("Iglin Bridge has one group"), Iglin.GroupCount, 1);
		TestEqual(TEXT("Iglin Bridge places three raptors"), Iglin.Groups[0].Count, 3);
		TestTrue(TEXT("Iglin Bridge raptors are Small"), Iglin.Groups[0].Size == ESolidMonsterSize::Small);
		TestTrue(
			TEXT("Iglin Bridge is Attack"),
			FCString::Strcmp(Iglin.Groups[0].Plan, TEXT("Attack")) == 0);
		TestTrue(TEXT("the near limit stays 8 m"), FMath::IsNearlyEqual(Iglin.NearLimitCm, 800.f));
		TestTrue(TEXT("the far limit stays 40 m"), FMath::IsNearlyEqual(Iglin.FarLimitCm, 4000.f));
		TestTrue(TEXT("the separation stays 4 m"), FMath::IsNearlyEqual(Iglin.SeparationCm, 400.f));
		TestTrue(TEXT("Iglin Bridge waits on no event"), !Iglin.EventId || !Iglin.EventId[0]);
	}
	const SolidEncounter::FStaticEncounter* Dracorex = SolidEncounter::FindByName(TEXT("Dracorex"));
	const SolidEncounter::FStaticEncounter* Folded = SolidEncounter::FindByName(TEXT("dracorex"));
	if (TestNotNull(TEXT("Dracorex is in the catalog"), Dracorex))
	{
		TestTrue(TEXT("dracorex is that same row"), Folded == Dracorex);
		TestEqual(TEXT("Dracorex has one group"), Dracorex->GroupCount, 1);
		TestEqual(TEXT("Dracorex places one raptor"), Dracorex->Groups[0].Count, 1);
		TestTrue(
			TEXT("that raptor is a raptor"),
			Dracorex->Groups[0].Kind == SolidEncounter::EMonsterKind::Raptor);
		TestTrue(TEXT("that raptor is Immense"), Dracorex->Groups[0].Size == ESolidMonsterSize::Immense);
		TestTrue(
			TEXT("Dracorex is Attack"),
			FCString::Strcmp(Dracorex->Groups[0].Plan, TEXT("Attack")) == 0);
		TestTrue(TEXT("Dracorex near limit stays 8 m"), FMath::IsNearlyEqual(Dracorex->NearLimitCm, 800.f));
		TestTrue(TEXT("Dracorex far limit stays 40 m"), FMath::IsNearlyEqual(Dracorex->FarLimitCm, 4000.f));
		TestTrue(TEXT("Dracorex separation stays 4 m"), FMath::IsNearlyEqual(Dracorex->SeparationCm, 400.f));
		TestTrue(TEXT("Dracorex waits on no event"), !Dracorex->EventId || !Dracorex->EventId[0]);
	}
	TestTrue(
		TEXT("a mesh of half the standing height doubles"),
		FMath::IsNearlyEqual(FSolidMonster::UniformScale(FSolidMonster::StandingHeightCm * 0.5f), 2.f));
	TestTrue(
		TEXT("a mesh of the standing height stays"),
		FMath::IsNearlyEqual(FSolidMonster::UniformScale(FSolidMonster::StandingHeightCm), 1.f));
	TestTrue(TEXT("a zero height stays at one"), FMath::IsNearlyEqual(FSolidMonster::UniformScale(0.f), 1.f));
	TestEqual(TEXT("six monster sizes"), SolidMonster::SizeCount, 6);
	TestTrue(
		TEXT("the size list starts at Tiny"),
		SolidMonster::Sizes[0] == ESolidMonsterSize::Tiny);
	TestTrue(
		TEXT("the size list ends at Immense"),
		SolidMonster::Sizes[SolidMonster::SizeCount - 1] == ESolidMonsterSize::Immense);
	TestTrue(
		TEXT("Small is named Small"),
		FCString::Strcmp(SolidMonster::SizeName(ESolidMonsterSize::Small), TEXT("Small")) == 0);
	TestTrue(
		TEXT("a Small raptor draws six times the span scale"),
		FMath::IsNearlyEqual(FSolidRaptor().DrawnBodyFactor(), FSolidRaptor::SmallDrawnBodyFactor));
	FSolidRaptor ImmenseBody;
	ImmenseBody.Size = ESolidMonsterSize::Immense;
	TestTrue(
		TEXT("an Immense raptor draws the Immense factor"),
		FMath::IsNearlyEqual(ImmenseBody.DrawnBodyFactor(), FSolidRaptor::ImmenseDrawnBodyFactor));
	TestTrue(
		TEXT("the Immense factor is eight times Small"),
		FMath::IsNearlyEqual(
			FSolidRaptor::ImmenseDrawnBodyFactor,
			FSolidRaptor::SmallDrawnBodyFactor * 8.f));
	FSolidRaptor MediumBody;
	MediumBody.Size = ESolidMonsterSize::Medium;
	TestTrue(
		TEXT("a size between Small and Immense draws as Small"),
		FMath::IsNearlyEqual(MediumBody.DrawnBodyFactor(), FSolidRaptor::SmallDrawnBodyFactor));
	FSolidRaptor SmallArmed;
	SmallArmed.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
	TestEqual(TEXT("Small hide stays 20"), SmallArmed.Armour.Strength, FSolidRaptor::HideStrength);
	FSolidRaptor ImmenseArmed;
	ImmenseArmed.Size = ESolidMonsterSize::Immense;
	ImmenseArmed.Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
	TestEqual(TEXT("Immense hide is 500"), ImmenseArmed.Armour.Strength, FSolidRaptor::ImmenseHideStrength);

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
	const FVector2D Focus = FVector2D::ZeroVector;

	const SolidEncounter::FStaticEncounter IglinDef = SolidEncounter::StaticEncounters[0];
	FSolidEncounter Encounter(IglinDef);
	TestEqual(
		TEXT("three reptiles"),
		Encounter.Place(Points, Trees, Forage, Focus),
		IglinDef.Groups[0].Count);
	TestTrue(
		TEXT("the encounter focus is the mark"),
		Encounter.Location.Equals(Focus));
	if (Encounter.Groups.Num() != 1 || Encounter.Groups[0].Num() != 3)
	{
		return false;
	}
	FSolidMonsterGroup& Group = Encounter.Groups[0];

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
		TestTrue(
			TEXT("the Iglin-bridge raptors are Small"),
			Member->Size == ESolidMonsterSize::Small);
		TestEqual(
			TEXT("the raptor wears claws"),
			FString(Member->Weapon.Name),
			FString(TEXT("Claws")));
		TestEqual(
			TEXT("the raptor wears hide"),
			FString(Member->Armour.Name),
			FString(TEXT("Hide")));
		TestEqual(
			TEXT("claws use the Small starting roll"),
			Member->Weapon.Power,
			SolidMonster::PowerFor(Member->Size, FSolidItem::StartingDraw));
		TestTrue(
			TEXT("claws have Speed"),
			Member->Weapon.Speed > 0.f && FMath::IsFinite(Member->Weapon.Speed));
		TestEqual(
			TEXT("Small hide stays 20"),
			Member->Armour.Strength,
			FSolidRaptor::HideStrength);
	}
	const int32 SharedPower = Group.Get(0)->Weapon.Power;
	Group.Get(1)->Weapon.Power = SharedPower + 1;
	TestEqual(TEXT("one raptor keeps its own weapon"), Group.Get(0)->Weapon.Power, SharedPower);
	TestEqual(TEXT("the third raptor keeps its own weapon"), Group.Get(2)->Weapon.Power, SharedPower);
	for (int32 Index = 1; Index < Group.Num(); ++Index)
	{
		const FSolidMonster* Prev = Group.Get(Index - 1);
		const FSolidMonster* Cur = Group.Get(Index);
		TestTrue(
			TEXT("later places stay 4 m apart"),
			Prev && Cur
				&& FVector2D::Distance(Cur->XY, Prev->XY) >= IglinDef.SeparationCm);
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
		TestTrue(TEXT("yaw looks at the mark"), FVector2D::DotProduct(Facing, ToOuter) > 0.99f);
		TestTrue(TEXT("a skipped point is not chosen"), Member->PointIndex != 0 && Member->PointIndex != 2
			&& Member->PointIndex != 3 && Member->PointIndex != 4 && Member->PointIndex != 5
			&& Member->PointIndex != 7 && Member->PointIndex != 9);
	}

	TArray<FSolidTerrainPoint> OnePoint;
	OnePoint.Add(PointAt(900.f, ESolidBiome::Grassland));
	TArray<int32> NoSkip;
	FSolidEncounter Few(IglinDef);
	TestEqual(
		TEXT("one eligible point places one"),
		Few.Place(OnePoint, NoSkip, NoSkip, Focus),
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
	ASolidRaptor* Raptor = NewObject<ASolidRaptor>();
	TestNotNull(TEXT("raptor"), Raptor);
	if (!Raptor)
	{
		return false;
	}

	const UCapsuleComponent* Capsule = Raptor->GetCapsule();
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
	TestFalse(TEXT("a raptor takes no damage"), Raptor->CanBeDamaged());

	const USkeletalMeshComponent* Body = Raptor->GetBody();
	TestNotNull(TEXT("body"), Body);
	if (Body)
	{
		TestTrue(TEXT("the mesh does not block"), Body->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
	}

	FSolidRaptor Immense;
	Immense.Size = ESolidMonsterSize::Immense;
	Raptor->SetRaptor(Immense);
	const float Scale = FSolidRaptor::ImmenseDrawnBodyFactor / FSolidRaptor::SmallDrawnBodyFactor;
	TestTrue(
		TEXT("an Immense capsule keeps the Small radius, times eight"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), FSolidMonster::CapsuleRadiusCm * Scale));
	TestTrue(
		TEXT("an Immense capsule keeps the Small half height, times eight"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), FSolidMonster::CapsuleHalfHeightCm * Scale));
	TestTrue(
		TEXT("the Immense capsule bottom is the actor origin"),
		FMath::IsNearlyEqual(Capsule->GetRelativeLocation().Z, FSolidMonster::CapsuleHalfHeightCm * Scale));

	FSolidRaptor Small;
	Raptor->SetRaptor(Small);
	TestTrue(
		TEXT("a Small raptor keeps the 27 cm radius"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleRadius(), FSolidMonster::CapsuleRadiusCm));
	TestTrue(
		TEXT("a Small raptor keeps the 50 cm half height"),
		FMath::IsNearlyEqual(Capsule->GetUnscaledCapsuleHalfHeight(), FSolidMonster::CapsuleHalfHeightCm));
	TestTrue(
		TEXT("the Small capsule bottom is the actor origin"),
		FMath::IsNearlyEqual(Capsule->GetRelativeLocation().Z, FSolidMonster::CapsuleHalfHeightCm));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterSpawnTest,
	"SolidCore1.Monster.Spawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterSpawnTest::RunTest(const FString& Parameters)
{
	using namespace SolidMonsterTestPrivate;

	TArray<FSolidTerrainPoint> Points;
	Points.Add(PointAt(900.f, ESolidBiome::Grassland, 0.f));
	TArray<int32> NoSkip;

	FSolidEncounter Encounter(SolidEncounter::StaticEncounters[0]);
	TestEqual(
		TEXT("the encounter places one raptor"),
		Encounter.Place(Points, NoSkip, NoSkip, FVector2D::ZeroVector),
		1);
	if (Encounter.Groups.Num() != 1)
	{
		return false;
	}

	const FSolidMonster* Member = Encounter.Groups[0].Get(0);
	TestNotNull(TEXT("the encounter member"), Member);
	if (!Member)
	{
		return false;
	}
	TestTrue(TEXT("the member is a raptor"), FCString::Strcmp(Member->Kind(), TEXT("Raptor")) == 0);
	TestNull(
		TEXT("that member needs a world to stand"),
		Member->Spawn(nullptr, FVector(Member->XY.X, Member->XY.Y, 0.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterMountainTest,
	"SolidCore1.Monster.Mountain",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterMountainTest::RunTest(const FString& Parameters)
{
	using namespace SolidMonsterTestPrivate;

	TArray<FSolidTerrainPoint> Points;
	Points.Add(PointAt(900.f, ESolidBiome::Grassland));
	Points.Add(PointAt(1100.f, ESolidBiome::Mountain));
	Points.Add(PointAt(1500.f, ESolidBiome::Grassland));
	Points.Add(PointAt(2000.f, ESolidBiome::Grassland));
	Points.Add(PointAt(2600.f, ESolidBiome::Grassland));

	TArray<int32> NoSkip;
	const float Spacing = 200.f;

	FSolidEncounter Beside(SolidEncounter::StaticEncounters[0]);
	TestEqual(
		TEXT("three reptiles stand off the cliff"),
		Beside.Place(Points, NoSkip, NoSkip, FVector2D::ZeroVector, Spacing),
		3);
	if (Beside.Groups.Num() != 1 || Beside.Groups[0].Num() != 3)
	{
		return false;
	}
	const int32 OffCliff[] = { 2, 3, 4 };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const FSolidMonster* Member = Beside.Groups[0].Get(Index);
		TestTrue(
			TEXT("the cliff corner is not used"),
			Member && Member->PointIndex == OffCliff[Index]);
	}

	TArray<FSolidTerrainPoint> DiagonalPoints;
	DiagonalPoints.Add(PointAt(900.f, ESolidBiome::Grassland));
	FSolidTerrainPoint Corner = PointAt(1100.f, ESolidBiome::Mountain);
	Corner.Y = 200.f;
	DiagonalPoints.Add(Corner);
	DiagonalPoints.Add(PointAt(1800.f, ESolidBiome::Grassland));
	DiagonalPoints.Add(PointAt(2300.f, ESolidBiome::Grassland));
	DiagonalPoints.Add(PointAt(3000.f, ESolidBiome::Grassland));

	FSolidEncounter Diagonal(SolidEncounter::StaticEncounters[0]);
	TestEqual(
		TEXT("three reptiles stand off a diagonal cliff"),
		Diagonal.Place(DiagonalPoints, NoSkip, NoSkip, FVector2D::ZeroVector, Spacing),
		3);
	const FSolidMonster* First = Diagonal.Groups.Num() == 1 ? Diagonal.Groups[0].Get(0) : nullptr;
	TestTrue(TEXT("a diagonal mountain corner is not used"), First && First->PointIndex != 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterBattlePlanTest,
	"SolidCore1.Monster.BattlePlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterBattlePlanTest::RunTest(const FString& Parameters)
{
	FSolidEncounter Iglin(SolidEncounter::StaticEncounters[0]);
	TestEqual(TEXT("Iglin Bridge has one group"), Iglin.Groups.Num(), 1);
	if (Iglin.Groups.Num() != 1)
	{
		return false;
	}
	const FSolidMonsterGroup& Group = Iglin.Groups[0];
	TestEqual(TEXT("the group has one available plan"), Group.Available.Num(), 1);
	TestEqual(TEXT("that plan is active"), Group.ActivePlan, 0);
	const FSolidBattlePlan* Active = Group.GetActivePlan();
	TestNotNull(TEXT("the active plan"), Active);
	FSolidBattlePlan Attack;
	if (Active)
	{
		TestTrue(TEXT("the active plan is Attack"), Active->Is(TEXT("Attack")));
		TestEqual(TEXT("Attack names itself"), Active->Name, FString(TEXT("Attack")));
		TestTrue(TEXT("Attack uses Mob"), Active->Formation == ESolidBattleFormation::Mob);
		TestTrue(TEXT("Attack uses standard spacing"), Active->Spacing == ESolidBattleSpacing::Standard);
		TestTrue(TEXT("Attack stores attack"), Active->AttackOrRetreat == ESolidAggression::Attack);
		TestTrue(TEXT("Attack attacks"), Active->Attacks());
		TestTrue(TEXT("Attack stores Random"), Active->TargetStrategy == ESolidTargetStrategy::Random);
		TestTrue(TEXT("Attack picks at random"), Active->PicksAtRandom());
		Attack = *Active;
	}

	TArray<FSolidTerrainPoint> Points;
	const TArray<int32> Skip;
	TestEqual(
		TEXT("no points place no reptiles"),
		Iglin.Place(Points, Skip, Skip, FVector2D::ZeroVector),
		0);
	const FSolidBattlePlan* Kept = Iglin.Groups[0].GetActivePlan();
	TestNotNull(TEXT("the group keeps a plan"), Kept);
	if (Kept)
	{
		TestTrue(TEXT("Attack stays the active plan"), Kept->SameAs(Attack));
	}

	SolidEncounter::FStaticEncounter Bare{ TEXT("Bare") };
	Bare = Bare.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 1);
	const FSolidEncounter NoPlan(Bare);
	TestEqual(TEXT("one group"), NoPlan.Groups.Num(), 1);
	if (NoPlan.Groups.Num() == 1)
	{
		TestEqual(TEXT("a group with no plan name has none"), NoPlan.Groups[0].Available.Num(), 0);
		TestNull(TEXT("that group has no active plan"), NoPlan.Groups[0].GetActivePlan());
	}

	SolidEncounter::FStaticEncounter Missing{ TEXT("Missing") };
	Missing = Missing.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 1, TEXT("Defend"));
	const FSolidEncounter NotInArray(Missing);
	TestEqual(TEXT("one group for a missing name"), NotInArray.Groups.Num(), 1);
	if (NotInArray.Groups.Num() == 1)
	{
		TestEqual(TEXT("a name absent from the monster plans has none"), NotInArray.Groups[0].Available.Num(), 0);
		TestNull(TEXT("that group has no active plan either"), NotInArray.Groups[0].GetActivePlan());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterDetectionTest,
	"SolidCore1.Monster.Detection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterDetectionTest::RunTest(const FString& Parameters)
{
	FSolidRaptor Raptor;
	Raptor.XY = FVector2D::ZeroVector;
	const float Range = FSolidMonster::StartDetectionRangeCm;
	TestEqual(TEXT("detection starts at 5 m"), Raptor.DetectionRange, Range);
	TestTrue(TEXT("standing on the monster is inside"), Raptor.Detects(FVector2D::ZeroVector));
	TestTrue(TEXT("the edge is inside"), Raptor.Detects(FVector2D(Range, 0.f)));
	TestFalse(TEXT("past the edge is outside"), Raptor.Detects(FVector2D(Range + 1.f, 0.f)));

	Raptor.DetectionRange = Range * 0.5f;
	TestFalse(TEXT("a shorter range leaves the old edge outside"), Raptor.Detects(FVector2D(Range, 0.f)));
	TestTrue(TEXT("the new edge is inside"), Raptor.Detects(FVector2D(Raptor.DetectionRange, 0.f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterMarkTest,
	"SolidCore1.Monster.Mark",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterMarkTest::RunTest(const FString& Parameters)
{
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* Empty = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("chart"), Chart) || !TestNotNull(TEXT("map"), Empty))
	{
		return false;
	}
	Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
	if (!TestTrue(TEXT("the unmarked chart loads"), Empty->LoadFromChart(Chart)))
	{
		return false;
	}
	TestEqual(TEXT("a definition with no mark places nobody"), Empty->GetEncounterMarks().Num(), 0);
	int32 DracorexOnEmpty = 0;
	for (const FSolidWorldEncounterMark& Mark : Empty->GetEncounterMarks())
	{
		if (Mark.Name.Equals(TEXT("Dracorex"), ESearchCase::IgnoreCase))
		{
			++DracorexOnEmpty;
		}
	}
	TestEqual(TEXT("a chart with no Dracorex mark places none of that encounter"), DracorexOnEmpty, 0);

	auto StandAt = [](const FSolidWorldEncounterMark& Mark) -> int32
	{
		const SolidEncounter::FStaticEncounter* Def = SolidEncounter::FindByName(Mark.Name);
		if (!Def)
		{
			return 0;
		}
		TArray<FSolidTerrainPoint> Points;
		const float Offsets[] = { 900.f, 1400.f, 1800.f, 2300.f };
		for (const float Offset : Offsets)
		{
			FSolidTerrainPoint Point;
			Point.X = Mark.PositionCm.X + Offset;
			Point.Y = Mark.PositionCm.Y;
			Point.Biome = ESolidBiome::Grassland;
			Points.Add(Point);
		}
		FSolidEncounter Encounter(*Def);
		const TArray<int32> None;
		const int32 Count = Encounter.Place(Points, None, None, Mark.PositionCm);
		return Encounter.Location.Equals(Mark.PositionCm) ? Count : 0;
	};

	FString Error;
	TestTrue(TEXT("the mark places"), Chart->PlaceEncounter(FIntPoint(10, 4), TEXT("Iglin Bridge"), Error));
	USolidWorldChart* Marked = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("the marked chart loads"), Marked && Marked->LoadFromChart(Chart)))
	{
		return false;
	}
	TestEqual(TEXT("one mark"), Marked->GetEncounterMarks().Num(), 1);
	if (Marked->GetEncounterMarks().Num() == 1)
	{
		TestEqual(TEXT("three members stand at the mark"), StandAt(Marked->GetEncounterMarks()[0]), 3);
	}

	FSolidWorldChartMark Second;
	Second.Kind = ESolidWorldChartMarkKind::Encounter;
	Second.Name = TEXT("Iglin Bridge");
	FVector2D SecondCentre = FVector2D::ZeroVector;
	TestTrue(TEXT("the second cell has a centre"), USolidWorldChart::CellCentre(FIntPoint(22, 9), SecondCentre));
	Second.PositionCm = SecondCentre;
	Chart->Marks.Add(Second);
	USolidWorldChart* Two = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("both marks load"), Two && Two->LoadFromChart(Chart)))
	{
		return false;
	}
	TestEqual(TEXT("two marks with one name"), Two->GetEncounterMarks().Num(), 2);
	int32 Total = 0;
	for (const FSolidWorldEncounterMark& Mark : Two->GetEncounterMarks())
	{
		Total += StandAt(Mark);
	}
	TestEqual(TEXT("two marks place two encounters"), Total, 6);

	USolidWorldChart* Rex = NewObject<USolidWorldChart>();
	FString RexError;
	const FIntPoint RexCell(30, 8);
	const FIntPoint RexCellTwo(40, 12);
	if (Rex)
	{
		Rex->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
	}
	if (!TestNotNull(TEXT("a Dracorex chart"), Rex)
		|| !TestFalse(TEXT("an unknown encounter is rejected"), Rex->PlaceEncounter(RexCell, TEXT("Nope"), RexError))
		|| !TestTrue(TEXT("Dracorex places"), Rex->PlaceEncounter(RexCell, TEXT("dracorex"), RexError)))
	{
		return false;
	}
	const int32 RexMark = Rex->MarkAtCell(RexCell);
	if (!TestTrue(TEXT("Dracorex is on the cell"), RexMark != INDEX_NONE))
	{
		return false;
	}
	TestEqual(TEXT("the stored name is Dracorex"), Rex->Marks[RexMark].Name, FString(TEXT("Dracorex")));

	USolidWorldChart* RexLoaded = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("the Dracorex chart loads"), RexLoaded && RexLoaded->LoadFromChart(Rex)))
	{
		return false;
	}
	TestEqual(TEXT("one Dracorex mark"), RexLoaded->GetEncounterMarks().Num(), 1);
	if (RexLoaded->GetEncounterMarks().Num() == 1)
	{
		const FSolidWorldEncounterMark& Mark = RexLoaded->GetEncounterMarks()[0];
		TestEqual(TEXT("one Immense raptor stands at the mark"), StandAt(Mark), 1);
		const SolidEncounter::FStaticEncounter* Def = SolidEncounter::FindByName(Mark.Name);
		if (TestNotNull(TEXT("the mark names Dracorex"), Def))
		{
			TArray<FSolidTerrainPoint> OnePoint;
			FSolidTerrainPoint Point;
			Point.X = Mark.PositionCm.X + 900.f;
			Point.Y = Mark.PositionCm.Y;
			Point.Biome = ESolidBiome::Grassland;
			OnePoint.Add(Point);
			FSolidEncounter Encounter(*Def);
			const TArray<int32> None;
			TestEqual(
				TEXT("one point places the one member"),
				Encounter.Place(OnePoint, None, None, Mark.PositionCm),
				1);
			const FSolidMonster* Member = Encounter.Groups.Num() == 1 ? Encounter.Groups[0].Get(0) : nullptr;
			TestTrue(TEXT("that member is Immense"), Member && Member->Size == ESolidMonsterSize::Immense);
			TestEqual(
				TEXT("that hide is Immense"),
				Member ? Member->Armour.Strength : 0.f,
				FSolidRaptor::ImmenseHideStrength);
		}
	}

	FSolidWorldChartMark SecondRex;
	SecondRex.Kind = ESolidWorldChartMarkKind::Encounter;
	SecondRex.Name = TEXT("Dracorex");
	FVector2D SecondRexCentre = FVector2D::ZeroVector;
	TestTrue(TEXT("the second Dracorex cell has a centre"), USolidWorldChart::CellCentre(RexCellTwo, SecondRexCentre));
	SecondRex.PositionCm = SecondRexCentre;
	Rex->Marks.Add(SecondRex);
	USolidWorldChart* TwoRex = NewObject<USolidWorldChart>();
	if (!TestTrue(TEXT("both Dracorex marks load"), TwoRex && TwoRex->LoadFromChart(Rex)))
	{
		return false;
	}
	TestEqual(TEXT("two marks named Dracorex"), TwoRex->GetEncounterMarks().Num(), 2);
	int32 RexTotal = 0;
	for (const FSolidWorldEncounterMark& Mark : TwoRex->GetEncounterMarks())
	{
		RexTotal += StandAt(Mark);
	}
	TestEqual(TEXT("two marks are two of that encounter"), RexTotal, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterMarkGroundTest,
	"SolidCore1.Monster.MarkGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterMarkGroundTest::RunTest(const FString& Parameters)
{
	const FIntPoint Cell(10, 4);
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	FString Error;
	if (!TestNotNull(TEXT("chart"), Chart) || !TestNotNull(TEXT("map"), WorldMap))
	{
		return false;
	}
	Chart->Cells.Init(ESolidBiome::Grassland, USolidWorldChart::MapWidth * USolidWorldChart::MapHeight);
	if (!TestTrue(TEXT("the mark places"), Chart->PlaceEncounter(Cell, TEXT("Iglin Bridge"), Error))
		|| !TestTrue(TEXT("the chart loads"), WorldMap->LoadFromChart(Chart))
		|| !TestEqual(TEXT("one mark"), WorldMap->GetEncounterMarks().Num(), 1))
	{
		return false;
	}

	USolidTerrainMap* Terrain = NewObject<USolidTerrainMap>();
	Terrain->Build(1337, 0.00012f, 3000.f, 0.f, 513, 513, 200.f, true, WorldMap);
	if (!TestTrue(TEXT("the terrain is built"), Terrain->IsBuilt()))
	{
		return false;
	}

	const FVector2D Stored = WorldMap->GetEncounterMarks()[0].PositionCm;
	const FVector2D Ground = USolidWorldChart::TerrainPosition(Stored, Terrain->GetWorldMinXY());
	FVector2D CellGround = FVector2D::ZeroVector;
	TestTrue(TEXT("the cell has a ground centre"),
		WorldMap->CellToWorldXY(Cell, Terrain->GetWorldMinXY(), Terrain->GetWorldMaxXY(), CellGround));
	TestTrue(TEXT("play uses the centred terrain"), Ground.Equals(CellGround, 1.f));
	TestFalse(TEXT("the stored chart position is not that ground point"), Stored.Equals(Ground, 1.f));

	const SolidEncounter::FStaticEncounter* Def = SolidEncounter::FindByName(TEXT("Iglin Bridge"));
	if (!TestNotNull(TEXT("Iglin Bridge"), Def))
	{
		return false;
	}
	const TArray<int32> None;
	FSolidEncounter OnChart(*Def);
	TestEqual(TEXT("the stored position places nobody on the terrain"),
		OnChart.Place(Terrain->GetPoints(), None, None, Stored, Terrain->PointSpacing), 0);

	FSolidEncounter OnGround(*Def);
	TestEqual(TEXT("the ground position places three"),
		OnGround.Place(Terrain->GetPoints(), None, None, Ground, Terrain->PointSpacing), 3);
	if (OnGround.Groups.Num() == 1)
	{
		for (int32 Index = 0; Index < OnGround.Groups[0].Num(); ++Index)
		{
			const FSolidMonster* Member = OnGround.Groups[0].Get(Index);
			if (!TestNotNull(TEXT("a member"), Member))
			{
				continue;
			}
			const float Distance = FVector2D::Distance(Member->XY, Ground);
			TestTrue(TEXT("the member stands in the band around the mark"),
				Distance >= Def->NearLimitCm && Distance <= Def->FarLimitCm);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterEventTest,
	"SolidCore1.Monster.Event",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterEventTest::RunTest(const FString& Parameters)
{
	USolidGameState* State = NewObject<USolidGameState>();
	if (!TestNotNull(TEXT("state"), State))
	{
		return false;
	}

	const SolidEncounter::FStaticEncounter* Iglin = SolidEncounter::FindByName(TEXT("Iglin Bridge"));
	if (!TestNotNull(TEXT("Iglin Bridge"), Iglin))
	{
		return false;
	}
	TestTrue(TEXT("Iglin Bridge waits on no event"), !Iglin->EventId || !Iglin->EventId[0]);
	TestTrue(TEXT("Iglin Bridge is placed before any event"), Iglin->HasFired(nullptr));
	TestTrue(TEXT("Iglin Bridge is placed on a new game"), Iglin->HasFired(State));

	TArray<FSolidTerrainPoint> Points;
	const float Offsets[] = { 900.f, 1400.f, 1800.f, 2300.f };
	for (const float Offset : Offsets)
	{
		FSolidTerrainPoint Point;
		Point.X = Offset;
		Point.Y = 0.f;
		Point.Biome = ESolidBiome::Grassland;
		Points.Add(Point);
	}
	const TArray<int32> None;
	const FVector2D Focus = FVector2D::ZeroVector;

	TArray<FSolidEncounter> Startup;
	TestEqual(
		TEXT("Iglin Bridge places three at startup"),
		SolidEncounter::PlaceMark(Startup, *Iglin, false, false, State, Points, None, None, Focus, 200.f),
		3);
	TestEqual(TEXT("Iglin Bridge is one encounter"), Startup.Num(), 1);
	TestEqual(
		TEXT("a second pass leaves Iglin Bridge where it stands"),
		SolidEncounter::PlaceMark(Startup, *Iglin, false, false, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("Iglin Bridge is not added twice"), Startup.Num(), 1);

	const SolidEncounter::FStaticEncounter* Dracorex = SolidEncounter::FindByName(TEXT("Dracorex"));
	if (!TestNotNull(TEXT("Dracorex"), Dracorex))
	{
		return false;
	}
	TestTrue(TEXT("Dracorex waits on no event"), !Dracorex->EventId || !Dracorex->EventId[0]);
	TestTrue(TEXT("Dracorex is placed on a new game"), Dracorex->HasFired(State));
	TArray<FSolidEncounter> RexStartup;
	TestEqual(
		TEXT("Dracorex places one raptor at startup"),
		SolidEncounter::PlaceMark(RexStartup, *Dracorex, false, false, State, Points, None, None, Focus, 200.f),
		1);
	TestEqual(TEXT("Dracorex is one encounter"), RexStartup.Num(), 1);
	if (RexStartup.Num() == 1 && RexStartup[0].Groups.Num() == 1)
	{
		const FSolidMonster* Member = RexStartup[0].Groups[0].Get(0);
		TestTrue(TEXT("the startup raptor is Immense"), Member && Member->Size == ESolidMonsterSize::Immense);
	}

	const SolidEncounter::FStaticEncounter Twice = SolidEncounter::FStaticEncounter{ TEXT("Twice") }
		.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 3, TEXT("Attack"))
		.Require(SolidEvents::IdEnterIglin)
		.Require(SolidEvents::IdEnterRelion);
	TestTrue(
		TEXT("a second event id replaces the first"),
		Twice.EventId && FCString::Strcmp(Twice.EventId, SolidEvents::IdEnterRelion) == 0);
	TestFalse(TEXT("the later event id starts unfired"), Twice.HasFired(State));

	const SolidEncounter::FStaticEncounter Waiting = SolidEncounter::FStaticEncounter{ TEXT("Waiting") }
		.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 3, TEXT("Attack"))
		.Require(SolidEvents::IdEnterIglin);
	const SolidEncounter::FStaticEncounter Unknown = SolidEncounter::FStaticEncounter{ TEXT("Unknown") }
		.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 1, TEXT("Attack"))
		.Require(TEXT("missing"));

	TestFalse(TEXT("an unfired event holds the row"), Waiting.HasFired(State));
	TestFalse(TEXT("a missing state holds a waiting row"), Waiting.HasFired(nullptr));
	TestFalse(TEXT("an unknown event id is unfired"), Unknown.HasFired(State));

	TArray<FSolidEncounter> Held;
	TestEqual(
		TEXT("an unfired event places nobody"),
		SolidEncounter::PlaceMark(Held, Waiting, false, false, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("the waiting mark stays out"), Held.Num(), 0);
	TestEqual(
		TEXT("an unknown event id places nobody"),
		SolidEncounter::PlaceMark(Held, Unknown, false, false, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("an unknown event stays out"), Held.Num(), 0);

	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	const int32 IglinIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterIglin);
	if (!TestTrue(TEXT("Enter Relion is an event"), RelionIndex != INDEX_NONE)
		|| !TestTrue(TEXT("Enter Iglin is an event"), IglinIndex != INDEX_NONE))
	{
		return false;
	}
	State->bFired[RelionIndex] = true;
	TestTrue(TEXT("the later event id is the one that fires"), Twice.HasFired(State));
	TestFalse(TEXT("another event does not release the wait"), Waiting.HasFired(State));
	TestEqual(
		TEXT("another event still places nobody"),
		SolidEncounter::PlaceMark(Held, Waiting, false, false, State, Points, None, None, Focus, 200.f),
		0);

	State->bFired[IglinIndex] = true;
	TestTrue(TEXT("the event firing releases the wait"), Waiting.HasFired(State));
	TestEqual(
		TEXT("the fired event places the members"),
		SolidEncounter::PlaceMark(Held, Waiting, false, false, State, Points, None, None, Focus, 200.f),
		3);
	TestEqual(TEXT("the mark is one encounter"), Held.Num(), 1);
	TestEqual(
		TEXT("the placed mark is not added again"),
		SolidEncounter::PlaceMark(Held, Waiting, false, false, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("one mark stays one encounter"), Held.Num(), 1);

	TArray<FSolidTerrainPoint> SecondPoints;
	for (const float Offset : Offsets)
	{
		FSolidTerrainPoint Point;
		Point.X = 20000.f + Offset;
		Point.Y = 0.f;
		Point.Biome = ESolidBiome::Grassland;
		SecondPoints.Add(Point);
	}
	const FVector2D SecondFocus(20000.f, 0.f);
	TestEqual(
		TEXT("a second mark places its own members"),
		SolidEncounter::PlaceMark(
			Held, Waiting, false, false, State, SecondPoints, None, None, SecondFocus, 200.f),
		3);
	TestEqual(TEXT("two marks of one name are two encounters"), Held.Num(), 2);

	TArray<FSolidEncounter> Closed;
	TestEqual(
		TEXT("a closed visit places none"),
		SolidEncounter::PlaceMark(Closed, Waiting, false, true, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("a closed visit adds no encounter"), Closed.Num(), 0);
	TestEqual(
		TEXT("a disabled name places none"),
		SolidEncounter::PlaceMark(Closed, Waiting, true, false, State, Points, None, None, Focus, 200.f),
		0);
	TestEqual(TEXT("a disabled name adds no encounter"), Closed.Num(), 0);

	if (const SolidEncounter::FStaticEncounter* Ants = SolidEncounter::FindByName(TEXT("Giant ants")))
	{
		TestTrue(TEXT("Giant ants waits on no event"), !Ants->EventId || !Ants->EventId[0]);
		int32 Expected = 0;
		for (int32 Group = 0; Group < Ants->GroupCount; ++Group)
		{
			Expected += Ants->Groups[Group].Count;
		}
		TArray<FSolidEncounter> AntEncounters;
		TestEqual(
			TEXT("Giant ants stands at startup"),
			SolidEncounter::PlaceMark(
				AntEncounters, *Ants, false, false, State, Points, None, None, Focus, 200.f),
			Expected);
		TestEqual(TEXT("Giant ants is one encounter"), AntEncounters.Num(), 1);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
