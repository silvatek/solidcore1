#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGameState.h"
#include "Company/SolidCompany.h"
#include "SolidStronghold.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "Strongholds/Dungeon/SolidDungeon.h"
#include "Strongholds/Fortress/SolidFortress.h"
#include "Strongholds/Fortress/Actors/SolidFortressExterior.h"
#include "Strongholds/Fortress/Actors/SolidFortressInterior.h"
#include "Strongholds/Tower/SolidTower.h"
#include "Strongholds/Dungeon/SolidDungeonExterior.h"
#include "Strongholds/Dungeon/SolidDungeonInterior.h"
#include "Strongholds/Dungeon/SolidDungeonLevel.h"
#include "Strongholds/Tower/Actors/SolidTowerExterior.h"
#include "Strongholds/Tower/Actors/SolidTowerInterior.h"

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

	const FIntPoint OriginCell = FIntPoint::ZeroValue;
	const TUniquePtr<FSolidStronghold> DungeonOwned = SolidStronghold::Make(
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdSize::Meagre,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	const TUniquePtr<FSolidStronghold> DungeonCopy = DungeonOwned->Clone();
	TestEqual(
		TEXT("a copied dungeon keeps its size"),
		static_cast<uint8>(DungeonCopy->Size),
		static_cast<uint8>(ESolidStrongholdSize::Meagre));

	const TUniquePtr<FSolidStronghold> TowerOwned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	const TUniquePtr<FSolidStronghold> TowerCopy = TowerOwned->Clone();
	TestEqual(
		TEXT("a copied tower keeps the tower trip"),
		static_cast<uint8>(TowerCopy->Trip()),
		static_cast<uint8>(ESolidStrongholdTrip::Tower));
	TestEqual(
		TEXT("a copied tower keeps its size"),
		static_cast<uint8>(TowerCopy->Size),
		static_cast<uint8>(ESolidStrongholdSize::Average));

	const SolidStronghold::FSolidTower* AverageTower = static_cast<const SolidStronghold::FSolidTower*>(TowerOwned.Get());
	const SolidStronghold::FSolidTower* AverageCopy = static_cast<const SolidStronghold::FSolidTower*>(TowerCopy.Get());
	SolidStronghold::FSolidTowerBand AverageBand;
	TestTrue(TEXT("Average has a floor band"), SolidStronghold::FindTowerBand(ESolidStrongholdSize::Average, AverageBand));
	TestEqual(TEXT("a made tower keeps its floor count"), AverageTower->Plan().FloorCount, SolidStronghold::BandFloorCount(AverageBand));
	TestEqual(TEXT("a made tower keeps its ground floor"), AverageTower->Plan().SourceFloor, SolidStronghold::BandSourceFloor(AverageBand));
	TestTrue(
		TEXT("the tower height is its plan height"),
		FMath::IsNearlyEqual(AverageTower->HeightCm(), SolidStronghold::HeightCm(AverageTower->Plan()), 0.01f));
	TestEqual(TEXT("a copied tower keeps its plan"), AverageCopy->Plan().FloorCount, AverageTower->Plan().FloorCount);
	TArray<SolidStrongholdGrid::FHex> TowerGround;
	TArray<SolidStrongholdGrid::FHex> PlanGround;
	AverageTower->CollectFloorHexes(0, TowerGround);
	AverageCopy->CollectFloorHexes(0, PlanGround);
	TestTrue(TEXT("a copied tower keeps its ground floor"), SameHexes(TowerGround, PlanGround));
	TestTrue(TEXT("a copy is its own stronghold"), DungeonCopy.Get() != DungeonOwned.Get());

	TestTrue(
		TEXT("a meagre dungeon names the dungeon exterior"),
		DungeonOwned->ExteriorClass() == ASolidDungeonExterior::StaticClass());
	TestTrue(
		TEXT("a copied dungeon names the dungeon exterior"),
		DungeonCopy->ExteriorClass() == ASolidDungeonExterior::StaticClass());
	TestTrue(
		TEXT("a meagre dungeon names the dungeon interior"),
		DungeonOwned->InteriorClass() == ASolidDungeonInterior::StaticClass());
	TestTrue(
		TEXT("a copied dungeon names the dungeon interior"),
		DungeonCopy->InteriorClass() == ASolidDungeonInterior::StaticClass());
	TestNull(
		TEXT("a humble dungeon has no exterior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Dungeon,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->ExteriorClass());
	TestNull(
		TEXT("a humble dungeon has no interior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Dungeon,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->InteriorClass());
	TestTrue(
		TEXT("a humble fortress names the fortress exterior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Fortress,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->ExteriorClass() == ASolidFortressExterior::StaticClass());
	TestTrue(
		TEXT("a humble fortress names the fortress interior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Fortress,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->InteriorClass() == ASolidFortressInterior::StaticClass());
	AddExpectedError(TEXT("Meagre is not a fortress size"), EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(
		TEXT("a meagre fortress has no exterior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Fortress,
			ESolidStrongholdSize::Meagre,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->ExteriorClass());
	TestTrue(
		TEXT("an Imperious fortress names the fortress exterior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Fortress,
			ESolidStrongholdSize::Imperious,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->ExteriorClass() == ASolidFortressExterior::StaticClass());
	TestTrue(
		TEXT("an Imperious fortress names the fortress interior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Fortress,
			ESolidStrongholdSize::Imperious,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->InteriorClass() == ASolidFortressInterior::StaticClass());
	TestTrue(
		TEXT("an average tower names the tower exterior"),
		TowerOwned->ExteriorClass() == ASolidTowerExterior::StaticClass());
	TestTrue(
		TEXT("a copied average tower names the tower exterior"),
		TowerCopy->ExteriorClass() == ASolidTowerExterior::StaticClass());
	TestTrue(
		TEXT("an average tower names the tower interior"),
		TowerOwned->InteriorClass() == ASolidTowerInterior::StaticClass());
	TestTrue(
		TEXT("a copied average tower names the tower interior"),
		TowerCopy->InteriorClass() == ASolidTowerInterior::StaticClass());
	const TUniquePtr<FSolidStronghold> ImperiousOwned = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Imperious,
		OriginCell,
		SolidStronghold::WorldXY(OriginCell));
	TestTrue(
		TEXT("an Imperious tower names the tower exterior"),
		ImperiousOwned->ExteriorClass() == ASolidTowerExterior::StaticClass());
	TestTrue(
		TEXT("an Imperious tower names the tower interior"),
		ImperiousOwned->InteriorClass() == ASolidTowerInterior::StaticClass());
	TestNull(
		TEXT("a humble tower has no exterior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Tower,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->ExteriorClass());
	TestNull(
		TEXT("a humble tower has no interior"),
		SolidStronghold::Make(
			ESolidStrongholdType::Tower,
			ESolidStrongholdSize::Humble,
			OriginCell,
			SolidStronghold::WorldXY(OriginCell))->InteriorClass());
	ASolidDungeonInterior* Cave = NewObject<ASolidDungeonInterior>();
	ASolidTowerInterior* Room = NewObject<ASolidTowerInterior>();
	TestNotNull(TEXT("dungeon interior"), Cave);
	TestNotNull(TEXT("tower interior"), Room);
	if (Cave && Room)
	{
		DungeonOwned->ConfigureInterior(Cave);
		ImperiousOwned->ConfigureInterior(Room);
		TestNotNull(TEXT("the dungeon fill builds the mouth"), Cave->GetMouthMesh());
		TestEqual(TEXT("the dungeon opening is the mouth"), Cave->GetPassageMesh(), Cave->GetMouthMesh());
		TestEqual(TEXT("the dungeon fill keeps the cell"), Cave->GetStrongholdCell(), OriginCell);
		TestEqual(TEXT("the tower fill keeps the cell"), Room->GetStrongholdCell(), OriginCell);
		TestNotNull(TEXT("the tower fill builds the door"), Room->GetDoorMesh());
		TestEqual(TEXT("the tower opening is the door"), Room->GetPassageMesh(), Room->GetDoorMesh());
	}
	TestTrue(
		TEXT("a copy keeps the world position"),
		DungeonCopy->Location.Equals(DungeonOwned->Location, 0.01f));
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
		SolidStronghold::FSolidTower Spire;
		Spire.Size = Size;
		Spire.BindPlan();
		const bool bMeagre = Size == ESolidStrongholdSize::Meagre;
		const bool bTowerSize = Size == ESolidStrongholdSize::Average
			|| Size == ESolidStrongholdSize::Impressive
			|| Size == ESolidStrongholdSize::Grand
			|| Size == ESolidStrongholdSize::Stately
			|| Size == ESolidStrongholdSize::Imperious;

		TestEqual(
			*FString::Printf(TEXT("%s dungeon supported"), SolidStronghold::SizeName(Size)),
			Dungeon.Supported(Size),
			bMeagre);
		TestEqual(
			*FString::Printf(TEXT("%s fortress supported"), SolidStronghold::SizeName(Size)),
			Fortress.Supported(Size),
			Size != ESolidStrongholdSize::Meagre);
		TestEqual(
			*FString::Printf(TEXT("%s tower supported"), SolidStronghold::SizeName(Size)),
			Tower.Supported(Size),
			bTowerSize);

		TArray<SolidStrongholdGrid::FHex> DungeonHexes;
		TArray<SolidStrongholdGrid::FHex> FortressHexes;
		TArray<SolidStrongholdGrid::FHex> TowerHexes;
		Dungeon.CollectGroundHexes(Size, DungeonHexes);
		Fortress.CollectGroundHexes(Size, FortressHexes);
		Tower.CollectGroundHexes(Size, TowerHexes);

		TArray<SolidStrongholdGrid::FHex> CaveFloor;
		if (bMeagre)
		{
			SolidDungeonLevel::CollectMeagreHexes(CaveFloor);
		}
		TestTrue(
			*FString::Printf(TEXT("%s dungeon ground floor"), SolidStronghold::SizeName(Size)),
			SameHexes(DungeonHexes, CaveFloor));
		const int32 SmallHexes = SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small);
		const int32 MediumHexes = SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium);
		const int32 LargeHexes = SolidStronghold::RoomHexes(ESolidStrongholdRoom::Large);
		int32 FortressFloor = SmallHexes;
		switch (Size)
		{
		case ESolidStrongholdSize::Meagre:
			FortressFloor = 0;
			break;
		case ESolidStrongholdSize::Humble:
			FortressFloor = SmallHexes;
			break;
		case ESolidStrongholdSize::Average:
			FortressFloor = 2 * SmallHexes;
			break;
		case ESolidStrongholdSize::Impressive:
			FortressFloor = MediumHexes + 2 * SmallHexes;
			break;
		case ESolidStrongholdSize::Grand:
			FortressFloor = LargeHexes + MediumHexes + 2 * SmallHexes;
			break;
		case ESolidStrongholdSize::Stately:
			FortressFloor = LargeHexes + 2 * MediumHexes + 2 * SmallHexes;
			break;
		case ESolidStrongholdSize::Imperious:
			FortressFloor = LargeHexes + 2 * MediumHexes + 3 * SmallHexes;
			break;
		default:
			break;
		}
		TestEqual(
			*FString::Printf(TEXT("%s fortress ground floor"), SolidStronghold::SizeName(Size)),
			FortressHexes.Num(),
			FortressFloor);

		TArray<SolidStrongholdGrid::FHex> Room;
		if (Spire.Supported(Size))
		{
			Spire.CollectFloorHexes(0, Room);
		}
		TestTrue(
			*FString::Printf(TEXT("%s tower ground floor"), SolidStronghold::SizeName(Size)),
			SameHexes(TowerHexes, Room));
		if (Size == ESolidStrongholdSize::Imperious)
		{
			TestEqual(TEXT("Imperious ground floor has 20 hex tiles"), TowerHexes.Num(), 20);
		}
		if (Size == ESolidStrongholdSize::Average)
		{
			TestEqual(TEXT("Average ground floor has 12 hex tiles"), TowerHexes.Num(), 12);
		}
		if (Size == ESolidStrongholdSize::Impressive)
		{
			TestEqual(TEXT("Impressive ground floor has 12 hex tiles"), TowerHexes.Num(), 12);
		}
		if (Size == ESolidStrongholdSize::Grand)
		{
			TestEqual(TEXT("Grand ground floor has 20 hex tiles"), TowerHexes.Num(), 20);
		}
		if (Size == ESolidStrongholdSize::Stately)
		{
			TestEqual(TEXT("Stately ground floor has 20 hex tiles"), TowerHexes.Num(), 20);
		}
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
	USolidGameState& Locked = *NewObject<USolidGameState>();
	USolidCompany* LockedCompany = NewObject<USolidCompany>();
	LockedCompany->InitializeStartingStronghold();
	Locked.bStrongholdSiteRestrictions = true;
	TArray<FSolidStrongholdChoice> Listed;
	TestEqual(
		TEXT("a locked size lists nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(Locked, LockedCompany->GetStronghold(), Listed)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(TEXT("a locked company lists nothing"), Listed.Num(), 0);
	TestTrue(
		TEXT("Imperious tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Imperious));
	TestTrue(
		TEXT("Average tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Average));
	TestTrue(
		TEXT("Impressive tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Impressive));
	TestTrue(
		TEXT("Grand tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Grand));
	TestTrue(
		TEXT("Stately tower can be built"),
		SolidStronghold::TypeOf(ESolidStrongholdType::Tower).Supported(ESolidStrongholdSize::Stately));

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	State.bStrongholdSiteRestrictions = true;
	State.MaxStrongholdSize = ESolidStrongholdSize::Imperious;
	SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Listed);

	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		const bool bLarger = static_cast<int32>(Size) > static_cast<int32>(Company->GetStronghold()->Size);
		const bool bUnlocked = SolidEvents::IsStrongholdSizeUnlocked(State, Size);
		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			const bool bSupported = SolidStronghold::TypeOf(Type).Supported(Size);
			const bool bListed = SolidStrongholdTransition::Offers(Listed, Size, Type);
			const FString Label = SolidStronghold::Label(Size, Type);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdRoomsTest,
	"SolidCore1.Stronghold.Rooms",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdRoomsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("a large room is 20 hexes"), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Large), 20);
	TestEqual(TEXT("a medium room is 12 hexes"), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Medium), 12);
	TestEqual(TEXT("a small room is 6 hexes"), SolidStronghold::RoomHexes(ESolidStrongholdRoom::Small), 6);

	const int32 Large[] = { 0, 0, 0, 0, 1, 2, 3 };
	const int32 Medium[] = { 1, 1, 1, 3, 4, 4, 4 };
	const int32 Small[] = { 0, 1, 2, 2, 2, 2, 3 };
	TestEqual(TEXT("every size has rooms"), SolidStronghold::SizeCount, static_cast<int32>(UE_ARRAY_COUNT(Large)));

	for (int32 Index = 0; Index < SolidStronghold::SizeCount; ++Index)
	{
		const ESolidStrongholdSize Size = SolidStronghold::Sizes[Index];
		const SolidStronghold::FRooms Rooms = SolidStronghold::RoomsFor(Size);
		const FString Name = SolidStronghold::SizeName(Size);
		TestEqual(*FString::Printf(TEXT("%s rooms are that size"), *Name), static_cast<uint8>(Rooms.Size), static_cast<uint8>(Size));
		TestEqual(*FString::Printf(TEXT("%s large rooms"), *Name), Rooms.Large, Large[Index]);
		TestEqual(*FString::Printf(TEXT("%s medium rooms"), *Name), Rooms.Medium, Medium[Index]);
		TestEqual(*FString::Printf(TEXT("%s small rooms"), *Name), Rooms.Small, Small[Index]);
	}

	TestEqual(TEXT("Meagre rooms are 12 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Meagre)), 12);
	TestEqual(TEXT("Humble rooms are 18 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Humble)), 18);
	TestEqual(TEXT("Average rooms are 24 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Average)), 24);
	TestEqual(TEXT("Impressive rooms are 48 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Impressive)), 48);
	TestEqual(TEXT("Grand rooms are 80 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Grand)), 80);
	TestEqual(TEXT("Stately rooms are 100 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Stately)), 100);
	TestEqual(TEXT("Imperious rooms are 126 hexes"), SolidStronghold::Hexes(SolidStronghold::RoomsFor(ESolidStrongholdSize::Imperious)), 126);
	return true;
}

#endif
