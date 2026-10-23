#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidMonsterGroup.h"
#include "Monster/SolidEncounter.h"
#include "Monster/SolidEncounters.h"
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
	TestEqual(TEXT("the definitions list has one encounter"), SolidEncounter::DefinitionCount, 1);
	if (SolidEncounter::DefinitionCount == 1)
	{
		TestTrue(
			TEXT("that encounter is Iglin Bridge"),
			FCString::Strcmp(SolidEncounter::Definitions[0].Name, TEXT("Iglin Bridge")) == 0);
		TestEqual(TEXT("Iglin Bridge has one group"), SolidEncounter::Definitions[0].GroupCount, 1);
		TestEqual(
			TEXT("Iglin Bridge places three raptors"),
			SolidEncounter::Definitions[0].Groups[0].Count,
			3);
		TestTrue(
			TEXT("the near limit stays 8 m"),
			FMath::IsNearlyEqual(SolidEncounter::Definitions[0].NearLimitCm, 800.f));
		TestTrue(
			TEXT("the far limit stays 40 m"),
			FMath::IsNearlyEqual(SolidEncounter::Definitions[0].FarLimitCm, 4000.f));
		TestTrue(
			TEXT("the separation stays 4 m"),
			FMath::IsNearlyEqual(SolidEncounter::Definitions[0].SeparationCm, 400.f));
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

	const SolidEncounter::FDefinition IglinDef = SolidEncounter::Definitions[0];
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
		TestTrue(
			TEXT("hide has Strength"),
			Member->Armour.Strength > 0.f && FMath::IsFinite(Member->Armour.Strength));
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

	FSolidEncounter Encounter(SolidEncounter::Definitions[0]);
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

	FSolidEncounter Beside(SolidEncounter::Definitions[0]);
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

	FSolidEncounter Diagonal(SolidEncounter::Definitions[0]);
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
	FSolidEncounter Iglin(SolidEncounter::Definitions[0]);
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

	SolidEncounter::FDefinition Bare{ TEXT("Bare") };
	Bare = Bare.WithGroup(SolidEncounter::EMonsterKind::Raptor, ESolidMonsterSize::Small, 1);
	const FSolidEncounter NoPlan(Bare);
	TestEqual(TEXT("one group"), NoPlan.Groups.Num(), 1);
	if (NoPlan.Groups.Num() == 1)
	{
		TestEqual(TEXT("a group with no plan name has none"), NoPlan.Groups[0].Available.Num(), 0);
		TestNull(TEXT("that group has no active plan"), NoPlan.Groups[0].GetActivePlan());
	}

	SolidEncounter::FDefinition Missing{ TEXT("Missing") };
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
	FString Grid;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		Grid += FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
		Grid += TEXT("\n");
	}

	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* Empty = NewObject<USolidWorldChart>();
	if (!TestNotNull(TEXT("chart"), Chart)
		|| !TestNotNull(TEXT("map"), Empty)
		|| !TestTrue(TEXT("grass imports"), Chart->ImportFromString(Grid))
		|| !TestTrue(TEXT("the unmarked chart loads"), Empty->LoadFromChart(Chart)))
	{
		return false;
	}
	TestEqual(TEXT("a definition with no mark places nobody"), Empty->GetEncounterMarks().Num(), 0);

	auto StandAt = [](const FSolidWorldEncounterMark& Mark) -> int32
	{
		const SolidEncounter::FDefinition* Def = SolidEncounter::FindByName(Mark.Name);
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

	TestTrue(TEXT("a second mark places"), Chart->PlaceEncounter(FIntPoint(22, 9), TEXT("iglin bridge"), Error));
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
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidMonsterMarkGroundTest,
	"SolidCore1.Monster.MarkGround",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidMonsterMarkGroundTest::RunTest(const FString& Parameters)
{
	FString Grid;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		Grid += FString::ChrN(USolidWorldChart::MapWidth, USolidWorldChart::GrassMarker);
		Grid += TEXT("\n");
	}

	const FIntPoint Cell(10, 4);
	USolidWorldChart* Chart = NewObject<USolidWorldChart>();
	USolidWorldChart* WorldMap = NewObject<USolidWorldChart>();
	FString Error;
	if (!TestNotNull(TEXT("chart"), Chart)
		|| !TestNotNull(TEXT("map"), WorldMap)
		|| !TestTrue(TEXT("grass imports"), Chart->ImportFromString(Grid))
		|| !TestTrue(TEXT("the mark places"), Chart->PlaceEncounter(Cell, TEXT("Iglin Bridge"), Error))
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

	const SolidEncounter::FDefinition* Def = SolidEncounter::FindByName(TEXT("Iglin Bridge"));
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

#endif // WITH_DEV_AUTOMATION_TESTS
