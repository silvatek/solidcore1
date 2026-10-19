#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGameState.h"
#include "SolidStronghold.h"
#include "SolidStrongholdGrid.h"
#include "SolidStrongholdTransition.h"
#include "SolidStrongholdType.h"
#include "SolidTowerStronghold.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool SameHexes(
		const TArray<SolidStrongholdGrid::FHex>& A,
		const TArray<SolidStrongholdGrid::FHex>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].Col != B[Index].Col
				|| A[Index].Row != B[Index].Row
				|| !A[Index].Center.Equals(B[Index].Center, 0.01f))
			{
				return false;
			}
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdTypeOfTest,
	"SolidCore1.Stronghold->TypeOf",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdTypeOfTest::RunTest(const FString& Parameters)
{
	const FSolidStronghold& Dungeon = SolidStronghold::TypeOf(ESolidStrongholdType::Dungeon);
	const FSolidStronghold& Fortress = SolidStronghold::TypeOf(ESolidStrongholdType::Fortress);
	const FSolidStronghold& Tower = SolidStronghold::TypeOf(ESolidStrongholdType::Tower);

	TestTrue(
		TEXT("TypeOf returns the dungeon subclass"),
		&Dungeon == &SolidStronghold::FSolidDungeon::Instance());
	TestTrue(
		TEXT("TypeOf returns the fortress subclass"),
		&Fortress == &SolidStronghold::FSolidFortress::Instance());
	TestTrue(
		TEXT("TypeOf returns the tower subclass"),
		&Tower == &SolidStronghold::FSolidTower::Instance());

	TestTrue(
		TEXT("the dungeon instance stays"),
		&SolidStronghold::TypeOf(ESolidStrongholdType::Dungeon) == &Dungeon);
	TestTrue(
		TEXT("the fortress instance stays"),
		&SolidStronghold::TypeOf(ESolidStrongholdType::Fortress) == &Fortress);
	TestTrue(
		TEXT("the tower instance stays"),
		&SolidStronghold::TypeOf(ESolidStrongholdType::Tower) == &Tower);

	TestTrue(TEXT("dungeon and fortress differ"), &Dungeon != &Fortress);
	TestTrue(TEXT("dungeon and tower differ"), &Dungeon != &Tower);
	TestTrue(TEXT("fortress and tower differ"), &Fortress != &Tower);

	const TUniquePtr<FSolidStronghold> DungeonOwned = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre);
	const TUniquePtr<FSolidStronghold> DungeonCopy = DungeonOwned->Clone();
	TestEqual(
		TEXT("a copied dungeon keeps the cliff mouth"),
		DungeonCopy->UsesCliffMouth(),
		DungeonOwned->UsesCliffMouth());
	TestEqual(
		TEXT("a copied dungeon keeps its size"),
		static_cast<uint8>(DungeonCopy->Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));

	const TUniquePtr<FSolidStronghold> TowerOwned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average);
	const TUniquePtr<FSolidStronghold> TowerCopy = TowerOwned->Clone();
	TestEqual(
		TEXT("a copied tower keeps the tower trip"),
		static_cast<uint8>(TowerCopy->Trip()),
		static_cast<uint8>(ESolidStrongholdTrip::Tower));
	TestEqual(
		TEXT("a copied tower keeps its size"),
		static_cast<uint8>(TowerCopy->Size),
		static_cast<uint8>(ESolidStrongholdSize::Average));
	TestTrue(TEXT("a copy is its own stronghold"), DungeonCopy.Get() != DungeonOwned.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdTypeSupportedTest,
	"SolidCore1.Stronghold->TypeSupported",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdTypeSupportedTest::RunTest(const FString& Parameters)
{
	const FSolidStronghold& Dungeon = SolidStronghold::TypeOf(ESolidStrongholdType::Dungeon);
	const FSolidStronghold& Fortress = SolidStronghold::TypeOf(ESolidStrongholdType::Fortress);
	const FSolidStronghold& Tower = SolidStronghold::TypeOf(ESolidStrongholdType::Tower);

	ESolidBiome CaveShelf = ESolidBiome::Grassland;
	const FSolidStronghold MeagreCave(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre);
	if (SolidStronghold::BiomeFor(MeagreCave, CaveShelf) && CaveShelf == ESolidBiome::Mountain)
	{
		TestTrue(TEXT("the dungeon uses the cliff mouth"), Dungeon.UsesCliffMouth());
	}
	TestFalse(TEXT("the fortress does not use the cliff mouth"), Fortress.UsesCliffMouth());
	TestFalse(TEXT("the tower does not use the cliff mouth"), Tower.UsesCliffMouth());
	TestEqual(
		TEXT("the dungeon trip is the cave"),
		static_cast<uint8>(Dungeon.Trip()),
		static_cast<uint8>(ESolidStrongholdTrip::Cave));
	TestEqual(
		TEXT("the fortress has no trip"),
		static_cast<uint8>(Fortress.Trip()),
		static_cast<uint8>(ESolidStrongholdTrip::None));
	TestEqual(
		TEXT("the tower trip is the tower"),
		static_cast<uint8>(Tower.Trip()),
		static_cast<uint8>(ESolidStrongholdTrip::Tower));

	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		const FSolidStronghold Cave(ESolidStrongholdType::Dungeon, Size);
		const FSolidStronghold Hold(ESolidStrongholdType::Fortress, Size);
		const FSolidStronghold Spire(ESolidStrongholdType::Tower, Size);
		const bool bMeagre = Size == ESolidStrongholdSize::Meagre;
		const bool bTowerSize = Size >= ESolidStrongholdSize::Average && Size <= ESolidStrongholdSize::Imperious;

		TestEqual(
			*FString::Printf(TEXT("%s dungeon supported"), SolidStronghold::SizeName(Size)),
			Dungeon.Supported(Size),
			bMeagre);
		TestFalse(
			*FString::Printf(TEXT("%s fortress is not supported"), SolidStronghold::SizeName(Size)),
			Fortress.Supported(Size));
		TestEqual(
			*FString::Printf(TEXT("%s tower supported"), SolidStronghold::SizeName(Size)),
			Tower.Supported(Size),
			bTowerSize);

		TestEqual(
			*FString::Printf(TEXT("%s dungeon mouth"), SolidStronghold::SizeName(Size)),
			SolidStronghold::UsesCaveMouth(Cave),
			Dungeon.UsesCliffMouth());
		TestEqual(
			*FString::Printf(TEXT("%s fortress mouth"), SolidStronghold::SizeName(Size)),
			SolidStronghold::UsesCaveMouth(Hold),
			Fortress.UsesCliffMouth());
		TestEqual(
			*FString::Printf(TEXT("%s tower mouth"), SolidStronghold::SizeName(Size)),
			SolidStronghold::UsesCaveMouth(Spire),
			Tower.UsesCliffMouth());

		TArray<SolidStrongholdGrid::FHex> DungeonHexes;
		TArray<SolidStrongholdGrid::FHex> FortressHexes;
		TArray<SolidStrongholdGrid::FHex> TowerHexes;
		Dungeon.CollectGroundHexes(Size, DungeonHexes);
		Fortress.CollectGroundHexes(Size, FortressHexes);
		Tower.CollectGroundHexes(Size, TowerHexes);

		TArray<SolidStrongholdGrid::FHex> CaveFloor;
		if (bMeagre)
		{
			SolidStrongholdGrid::CollectMeagreHexes(CaveFloor);
		}
		TestTrue(
			*FString::Printf(TEXT("%s dungeon ground floor"), SolidStronghold::SizeName(Size)),
			SameHexes(DungeonHexes, CaveFloor));
		TestEqual(
			*FString::Printf(TEXT("%s fortress has no ground floor"), SolidStronghold::SizeName(Size)),
			FortressHexes.Num(),
			0);

		TArray<SolidStrongholdGrid::FHex> Kept;
		SolidTowerStronghold::FPlan Plan;
		if (SolidTowerStronghold::PlanFor(Spire, Plan))
		{
			SolidTowerStronghold::CollectKeptHexes(Plan, 0, Kept);
		}
		TestTrue(
			*FString::Printf(TEXT("%s tower ground floor"), SolidStronghold::SizeName(Size)),
			SameHexes(TowerHexes, Kept));
	}

	TArray<SolidStrongholdGrid::FHex> MeagreFloor;
	Dungeon.CollectGroundHexes(ESolidStrongholdSize::Meagre, MeagreFloor);
	TestEqual(TEXT("the cave ground floor is fourteen hexes"), MeagreFloor.Num(), 14);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdTypeChoiceTest,
	"SolidCore1.Stronghold->TypeChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdTypeChoiceTest::RunTest(const FString& Parameters)
{
	FGameState Locked;
	TArray<ESolidStrongholdType> Listed;
	TestEqual(
		TEXT("a locked size lists nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(Locked, ESolidStrongholdSize::Imperious, Listed)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(TEXT("a locked Imperious site is empty"), Listed.Num(), 0);
	TestTrue(
		TEXT("Imperious tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Imperious));

	FGameState State;
	State.MaxStrongholdSize = ESolidStrongholdSize::Imperious;

	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		const bool bLarger = static_cast<int32>(Size) > static_cast<int32>(State.ActiveStronghold->Size);
		const bool bUnlocked = SolidEvents::IsStrongholdSizeUnlocked(State, Size);
		SolidStrongholdTransition::Consider(State, Size, Listed);
		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			const bool bSupported = SolidStronghold::TypeOf(Type).Supported(Size);
			const bool bListed = Listed.Contains(Type);
			const FString Label = FString::Printf(
				TEXT("%s %s"),
				SolidStronghold::SizeName(Size),
				SolidStronghold::TypeName(Type));
			if (bLarger && bUnlocked && bSupported)
			{
				TestTrue(*FString::Printf(TEXT("%s is listed"), *Label), bListed);
			}
			else
			{
				TestFalse(*FString::Printf(TEXT("%s is not listed"), *Label), bListed);
			}
		}
	}
	return true;
}

#endif
