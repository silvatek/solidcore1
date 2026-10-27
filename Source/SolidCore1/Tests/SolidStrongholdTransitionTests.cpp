#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGameState.h"
#include "Company/SolidCompany.h"
#include "SolidHUD.h"
#include "SolidInventory.h"
#include "SolidStronghold.h"
#include "Strongholds/Furnishings/Actors/SolidBed.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "Strongholds/Tower/SolidTower.h"
#include "SolidWorldChart.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	void Unlock(USolidGameState& State, const ESolidStrongholdSize Size)
	{
		if (static_cast<int32>(Size) > static_cast<int32>(State.MaxStrongholdSize))
		{
			State.MaxStrongholdSize = Size;
		}
	}

	bool SameId(const TCHAR* A, const TCHAR* B)
	{
		if (A == B)
		{
			return true;
		}
		if (!A || !B)
		{
			return false;
		}
		return FCString::Strcmp(A, B) == 0;
	}

	bool SameItem(const FSolidItem& A, const FSolidItem& B)
	{
		return A.Count == B.Count && A.Power() == B.Power() && SameId(A.Id(), B.Id());
	}

	bool SameChest(const SolidStorage::FContainer& A, const SolidStorage::FContainer& B)
	{
		for (int32 Index = 0; Index < SolidStorage::Count; ++Index)
		{
			if (!SameItem(A.Slots[Index], B.Slots[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool SameLoadout(const SolidInventory::FLoadout& A, const SolidInventory::FLoadout& B)
	{
		for (int32 Index = 0; Index < SolidInventory::WornCount; ++Index)
		{
			if (!SameItem(A.Worn[Index], B.Worn[Index]))
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
		{
			if (!SameItem(A.Pack[Index], B.Pack[Index]))
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < SolidInventory::CraftCount; ++Index)
		{
			if (!SameItem(A.Craft[Index], B.Craft[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool SamePieces(
		const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& A,
		const TArray<SolidStrongholdFurniture::FPlacedFurnishing>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].ItemIndex != B[Index].ItemIndex
				|| A[Index].Level != B[Index].Level
				|| A[Index].X != B[Index].X
				|| A[Index].Y != B[Index].Y
				|| !FMath::IsNearlyEqual(A[Index].RotationDeg, B[Index].RotationDeg))
			{
				return false;
			}
		}
		return true;
	}

	int32 OfferedCount(const USolidGameState& State, const FSolidStronghold* Active)
	{
		int32 Count = 0;
		if (!Active)
		{
			return 0;
		}
		const int32 ActiveIndex = static_cast<int32>(Active->Size);
		for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
		{
		if (!SolidEvents::IsStrongholdSizeUnlocked(State, Size))
		{
			continue;
		}
		if (State.bStrongholdSiteRestrictions && static_cast<int32>(Size) <= ActiveIndex)
		{
			continue;
		}
			for (const ESolidStrongholdType Type : SolidStronghold::Types)
			{
				if (SolidStronghold::TypeOf(Type).Supported(Size))
				{
					++Count;
				}
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdSiteOfferTest,
	"SolidCore1.Stronghold.SiteOffer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdSiteOfferTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("the starting stronghold is built"),
		SolidStrongholdTransition::IsBuilt(FSolidStronghold(
			ESolidStrongholdType::Dungeon,
			ESolidStrongholdSize::Meagre)));
	TestFalse(
		TEXT("a Meagre fortress is not a pair"),
		SolidStrongholdTransition::IsFinalPair(FSolidStronghold{ ESolidStrongholdType::Fortress, ESolidStrongholdSize::Meagre }));
	TestFalse(
		TEXT("a Meagre tower is not a pair"),
		SolidStrongholdTransition::IsFinalPair(FSolidStronghold{ ESolidStrongholdType::Tower, ESolidStrongholdSize::Meagre }));
	TestFalse(
		TEXT("a Humble tower is not a pair"),
		SolidStrongholdTransition::IsFinalPair(FSolidStronghold{ ESolidStrongholdType::Tower, ESolidStrongholdSize::Humble }));
	TestTrue(
		TEXT("a Humble dungeon is a pair"),
		SolidStrongholdTransition::IsFinalPair(FSolidStronghold{ ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Humble }));
	TestTrue(
		TEXT("a Humble fortress is a pair"),
		SolidStrongholdTransition::IsFinalPair(FSolidStronghold{ ESolidStrongholdType::Fortress, ESolidStrongholdSize::Humble }));
	TestFalse(
		TEXT("a Humble dungeon is not built"),
		SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Humble }));
	TestTrue(
		TEXT("a Humble fortress is built"),
		SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Fortress, ESolidStrongholdSize::Humble }));

	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		const bool bBuiltTower = Size == ESolidStrongholdSize::Average
			|| Size == ESolidStrongholdSize::Impressive
			|| Size == ESolidStrongholdSize::Grand
			|| Size == ESolidStrongholdSize::Stately
			|| Size == ESolidStrongholdSize::Imperious;
		TestEqual(
			*FString::Printf(TEXT("%s tower is built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Tower, Size }),
			bBuiltTower);
		if (static_cast<int32>(Size) < static_cast<int32>(ESolidStrongholdSize::Average))
		{
			continue;
		}
		TestFalse(
			*FString::Printf(TEXT("%s dungeon is not built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Dungeon, Size }));
		TestFalse(
			*FString::Printf(TEXT("%s fortress is not built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Fortress, Size }));
	}

	USolidGameState& Open = *NewObject<USolidGameState>();
	USolidCompany* OpenCompany = NewObject<USolidCompany>();
	OpenCompany->InitializeStartingStronghold();
	TestFalse(TEXT("site restrictions start off"), Open.bStrongholdSiteRestrictions);
	TArray<FSolidStrongholdChoice> OpenChoices;
	TestEqual(
		TEXT("an open site lists a choice"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(Open, OpenCompany->GetStronghold(), OpenChoices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
	TestTrue(
		TEXT("Meagre Dungeon is a row"),
		SolidStrongholdTransition::Offers(OpenChoices, ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the open list is that one row"), OpenChoices.Num(), 1);
	TestFalse(TEXT("site restrictions stay off on the same object"), Open.bStrongholdSiteRestrictions);
	const FIntPoint MeagreSite(12, 12);
	TestTrue(
		TEXT("Meagre Dungeon can be chosen"),
		SolidStrongholdTransition::Move(*OpenCompany, Open, MeagreSite, ESolidStrongholdSize::Meagre, ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the company is on that site"), OpenCompany->GetStronghold()->Cell, MeagreSite);

	USolidGameState& State = *NewObject<USolidGameState>();
	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	State.bStrongholdSiteRestrictions = true;
	TestFalse(TEXT("strongholds start closed"), State.bStrongholdsEnabled);
	TestFalse(
		TEXT("a closed stronghold refuses enter"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), Company->GetStronghold()->Cell));
	State.bStrongholdsEnabled = true;
	TestTrue(
		TEXT("the open stronghold still accepts enter"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), Company->GetStronghold()->Cell));

	TArray<FSolidStrongholdChoice> Choices;
	TestEqual(
		TEXT("a locked size does nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Choices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(TEXT("a locked size lists nothing"), Choices.Num(), 0);

	Unlock(State, ESolidStrongholdSize::Meagre);
	TestEqual(
		TEXT("Meagre is not a site choice"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Choices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));

	Unlock(State, ESolidStrongholdSize::Humble);
	TestEqual(
		TEXT("Humble asks for the fortress"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Choices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
	TestEqual(TEXT("Humble lists the fortress"), Choices.Num(), 1);
	TestTrue(
		TEXT("Humble fortress is a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Humble, ESolidStrongholdType::Fortress));

	Unlock(State, ESolidStrongholdSize::Imperious);
	TestEqual(
		TEXT("an unlocked larger size asks"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Choices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
	TestEqual(TEXT("the list is Humble fortress and Average through Imperious towers"), Choices.Num(), 6);
	TestEqual(
		TEXT("the first row is Humble"),
		static_cast<uint8>(Choices[0].Size),
		static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestEqual(
		TEXT("the first row is a fortress"),
		static_cast<uint8>(Choices[0].Type),
		static_cast<uint8>(ESolidStrongholdType::Fortress));
	TestEqual(
		TEXT("the second row is Average"),
		static_cast<uint8>(Choices[1].Size),
		static_cast<uint8>(ESolidStrongholdSize::Average));
	TestEqual(
		TEXT("the third row is Impressive"),
		static_cast<uint8>(Choices[2].Size),
		static_cast<uint8>(ESolidStrongholdSize::Impressive));
	TestEqual(
		TEXT("the fourth row is Grand"),
		static_cast<uint8>(Choices[3].Size),
		static_cast<uint8>(ESolidStrongholdSize::Grand));
	TestEqual(
		TEXT("the fifth row is Stately"),
		static_cast<uint8>(Choices[4].Size),
		static_cast<uint8>(ESolidStrongholdSize::Stately));
	TestEqual(
		TEXT("the sixth row is Imperious"),
		static_cast<uint8>(Choices[5].Size),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestTrue(
		TEXT("Average tower is a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Average, ESolidStrongholdType::Tower));

	Company->SetStronghold(SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		FIntPoint::ZeroValue,
		SolidStronghold::WorldXY(FIntPoint::ZeroValue)));
	TestEqual(
		TEXT("a larger size still asks"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, Company->GetStronghold(), Choices)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
	TestFalse(
		TEXT("the same size is not a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Average, ESolidStrongholdType::Tower));
	TestFalse(
		TEXT("a smaller size is not a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Humble, ESolidStrongholdType::Dungeon));
	TestFalse(
		TEXT("a smaller fortress is not a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Humble, ESolidStrongholdType::Fortress));
	TestTrue(
		TEXT("Impressive tower is a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Impressive, ESolidStrongholdType::Tower));
	TestTrue(
		TEXT("Grand tower is a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Grand, ESolidStrongholdType::Tower));
	TestTrue(
		TEXT("Stately tower is a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Stately, ESolidStrongholdType::Tower));
	TestTrue(
		TEXT("Imperious tower is still a row"),
		SolidStrongholdTransition::Offers(Choices, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));

	USolidWorldChart* Map = NewObject<USolidWorldChart>();
	TestNotNull(TEXT("world map"), Map);
	TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault());
	if (!Map)
	{
		return false;
	}

	int32 Sites = 0;
	for (int32 Row = 0; Row < USolidWorldChart::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldChart::MapWidth; ++Col)
		{
			ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
			if (!Map->StrongholdSiteAtCell(Col, Row, SiteSize))
			{
				continue;
			}
			++Sites;
			USolidGameState& Fresh = *NewObject<USolidGameState>();
			USolidCompany* FreshCompany = NewObject<USolidCompany>();
			FreshCompany->InitializeStartingStronghold();
			Fresh.bStrongholdSiteRestrictions = true;
			TArray<FSolidStrongholdChoice> Offered;
			TestEqual(
				*FString::Printf(TEXT("site %d,%d does nothing until unlocked"), Row + 1, Col + 1),
				static_cast<uint8>(SolidStrongholdTransition::Consider(Fresh, FreshCompany->GetStronghold(), Offered)),
				static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
			Unlock(Fresh, SiteSize);
			const SolidStrongholdTransition::EOffer Offer = SolidStrongholdTransition::Consider(
				Fresh,
				FreshCompany->GetStronghold(),
				Offered);
			const int32 Expected = OfferedCount(Fresh, FreshCompany->GetStronghold());
			if (Expected == 0)
			{
				TestEqual(
					*FString::Printf(TEXT("site %d,%d has nothing built above the company"), Row + 1, Col + 1),
					static_cast<uint8>(Offer),
					static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
			}
			else
			{
				TestEqual(
					*FString::Printf(TEXT("site %d,%d asks"), Row + 1, Col + 1),
					static_cast<uint8>(Offer),
					static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
				TestEqual(
					*FString::Printf(TEXT("site %d,%d lists built sizes and types"), Row + 1, Col + 1),
					Offered.Num(),
					Expected);
			}
		}
	}
	TestTrue(TEXT("the shipped map has a site"), Sites >= 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdMoveTest,
	"SolidCore1.Stronghold.Move",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdMoveTest::RunTest(const FString& Parameters)
{
	const FIntPoint AverageCell(10, 12);
	const FIntPoint ImperiousCell(35, 15);
	const FIntPoint MapTowerCell(30, 16);

	USolidCompany* Company = NewObject<USolidCompany>();
	Company->InitializeStartingStronghold();
	USolidGameState& State = *NewObject<USolidGameState>();
	State.bStrongholdSiteRestrictions = true;
	State.bStrongholdsEnabled = false;
	SolidStrongholdFurniture::FPlacedFurnishing* ChestPiece = Company->GetStronghold()->StorageAt(0);
	TestNotNull(TEXT("the cave has a chest"), ChestPiece);
	if (!ChestPiece)
	{
		return false;
	}
	SolidStorage::FContainer Edited = ChestPiece->Container();
	SolidStorage::WriteSlot(Edited, SolidStorage::Index(1, 2), SolidItemType::IdStick, 4);
	ChestPiece->SetContainer(Edited);
	SolidInventory::WritePack(State.Inventory, 0, SolidItemType::IdStick, 2);
	const SolidStorage::FContainer Chest = ChestPiece->Container();
	const SolidInventory::FLoadout Pack = State.Inventory;
	const TArray<SolidStrongholdFurniture::FPlacedFurnishing> Started = Company->GetStronghold()->Furnishings;

	TestFalse(
		TEXT("a locked size does not move"),
		SolidStrongholdTransition::Move(*Company, State, AverageCell, ESolidStrongholdSize::Average, ESolidStrongholdType::Tower));
	Unlock(State, ESolidStrongholdSize::Imperious);
	TestFalse(
		TEXT("a type that was not offered does not move"),
		SolidStrongholdTransition::Move(*Company, State, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("a refused type leaves the stronghold"), static_cast<uint8>(Company->GetStronghold()->Type), static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("a refused type leaves the size"), static_cast<uint8>(Company->GetStronghold()->Size), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("a refused type leaves the cell"), Company->GetStronghold()->Cell, SolidStronghold::StartingCell());
	TestEqual(TEXT("a refused type leaves no history"), State.PreviousStrongholds.Num(), 0);
	TestTrue(TEXT("a refused type leaves the pieces"), SamePieces(Company->GetStronghold()->Furnishings, Started));
	const SolidStrongholdFurniture::FPlacedFurnishing* KeptChest = Company->GetStronghold()->StorageAt(0);
	TestTrue(TEXT("a refused type leaves the chest"), KeptChest && SameChest(KeptChest->Container(), Chest));
	TestTrue(TEXT("a refused type leaves the pack"), SameLoadout(State.Inventory, Pack));

	TestFalse(
		TEXT("a Humble tower is not offered"),
		SolidStrongholdTransition::Move(*Company, State, AverageCell, ESolidStrongholdSize::Humble, ESolidStrongholdType::Tower));
	TestEqual(TEXT("a refused size leaves the stronghold"), static_cast<uint8>(Company->GetStronghold()->Size), static_cast<uint8>(ESolidStrongholdSize::Meagre));

	FSolidStronghold* Cave = Company->GetStronghold();
	TestTrue(
		TEXT("the cave's pieces move onto the tower"),
		SolidStrongholdTransition::Move(*Company, State, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));
	TestTrue(
		TEXT("the stronghold that was active is the history entry"),
		State.PreviousStrongholds[0].Get() == Cave);
	TestEqual(TEXT("the company is in a tower"), static_cast<uint8>(Company->GetStronghold()->Type), static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(TEXT("the stronghold size is Imperious"), static_cast<uint8>(Company->GetStronghold()->Size), static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(TEXT("the company is on the site"), Company->GetStronghold()->Cell, ImperiousCell);
	TestEqual(TEXT("the new tower is named from its size and type"), Company->GetStronghold()->Name, FString(TEXT("Imperious Tower")));
	TestEqual(TEXT("the cave keeps its name"), State.PreviousStrongholds[0]->Name, FString(SolidStronghold::StartingStrongholdName));
	TestEqual(TEXT("one previous stronghold is recorded"), State.PreviousStrongholds.Num(), 1);
	TestEqual(
		TEXT("the previous stronghold is a Dungeon"),
		static_cast<uint8>(State.PreviousStrongholds[0]->Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the previous stronghold keeps its cell"), State.PreviousStrongholds[0]->Cell, SolidStronghold::StartingCell());
	TestFalse(
		TEXT("the move leaves strongholds shut"),
		State.bStrongholdsEnabled);
	TestFalse(
		TEXT("the previous door is not the passage"),
		State.PreviousStrongholds[0]->AcceptsEnter(State, Company->GetStronghold(), SolidStronghold::StartingCell()));
	TestFalse(
		TEXT("a shut stronghold refuses the new door"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), ImperiousCell));
	State.bStrongholdsEnabled = true;
	TestFalse(
		TEXT("the previous door stays shut"),
		State.PreviousStrongholds[0]->AcceptsEnter(State, Company->GetStronghold(), SolidStronghold::StartingCell()));
	TestTrue(
		TEXT("the active stronghold accepts enter"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), ImperiousCell));
	TestFalse(
		TEXT("another tower door is not a passage"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), AverageCell));
	TestFalse(
		TEXT("a map tower door is not a passage"),
		Company->GetStronghold()->AcceptsEnter(State, Company->GetStronghold(), MapTowerCell));
	const SolidStrongholdFurniture::FPlacedFurnishing* MovedChest = Company->GetStronghold()->StorageAt(0);
	TestTrue(TEXT("the chest moved with the company"), MovedChest && SameChest(MovedChest->Container(), Chest));
	TestTrue(TEXT("the pack stayed with the captain"), SameLoadout(State.Inventory, Pack));
	TestEqual(TEXT("the stronghold left behind is empty"), State.PreviousStrongholds[0]->Furnishings.Num(), 0);
	TestEqual(TEXT("the starting pieces moved"), Company->GetStronghold()->Furnishings.Num(), Started.Num());

	const SolidStronghold::FSolidTower* ImperiousTower = static_cast<const SolidStronghold::FSolidTower*>(Company->GetStronghold());
	TArray<SolidStrongholdGrid::FHex> ImperiousHexes;
	ImperiousTower->CollectFloorHexes(0, ImperiousHexes);
	int32 Beds = 0;
	for (const SolidStrongholdFurniture::FPlacedFurnishing& Piece : Company->GetStronghold()->Furnishings)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Piece.ItemIndex);
		TestTrue(TEXT("piece in the new stronghold"), Item != nullptr);
		TestEqual(TEXT("pieces sit on the ground floor"), Piece.Level, 0);
		TestTrue(TEXT("new cell is a ground hex tile"), SolidStrongholdFurniture::FindCell(ImperiousHexes, Piece.X, Piece.Y) != nullptr);
		if (Item && Item->ActorClass == ASolidBed::StaticClass())
		{
			++Beds;
			const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(ImperiousHexes, Piece.X, Piece.Y);
			const SolidStrongholdGrid::FHex* Span = Piece.SpanCell(ImperiousHexes);
			TestTrue(TEXT("the bed spans neighbors"), Anchor && Span && SolidStrongholdTransition::AreNeighbors(*Anchor, *Span));
		}
	}
	TestEqual(TEXT("the bed moved"), Beds, 1);

	TestFalse(
		TEXT("the same size does not move again"),
		SolidStrongholdTransition::Move(*Company, State, AverageCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));
	TestEqual(TEXT("a refused move leaves the cell"), Company->GetStronghold()->Cell, ImperiousCell);
	TestEqual(TEXT("a refused move leaves one previous stronghold"), State.PreviousStrongholds.Num(), 1);

	TestFalse(
		TEXT("outside, the chest stays shut"),
		SolidStrongholdTransition::CompanyChestOpen(State, Company->GetStronghold(), false));
	TestTrue(
		TEXT("the chest opens in the company tower"),
		SolidStrongholdTransition::CompanyChestOpen(State, Company->GetStronghold(), true));

	TArray<SolidStrongholdFurniture::FPlacedFurnishing> Stuck = Started;
	TestFalse(TEXT("pieces do not sit on an empty floor"), SolidStrongholdTransition::PlaceOnFloor(Stuck, TArray<SolidStrongholdGrid::FHex>()));
	TestTrue(TEXT("a failed placement leaves the pieces"), SamePieces(Stuck, Started));

	USolidCompany* FortressCompany = NewObject<USolidCompany>();
	FortressCompany->InitializeStartingStronghold();
	USolidGameState& FortressState = *NewObject<USolidGameState>();
	FortressState.bStrongholdSiteRestrictions = true;
	Unlock(FortressState, ESolidStrongholdSize::Humble);
	const FIntPoint HumbleCell(12, 20);
	const TArray<SolidStrongholdFurniture::FPlacedFurnishing> CavePieces = FortressCompany->GetStronghold()->Furnishings;
	TestTrue(
		TEXT("Humble fortress can be chosen"),
		SolidStrongholdTransition::Move(
			*FortressCompany,
			FortressState,
			HumbleCell,
			ESolidStrongholdSize::Humble,
			ESolidStrongholdType::Fortress));
	TestEqual(
		TEXT("the company is in a fortress"),
		static_cast<uint8>(FortressCompany->GetStronghold()->Type),
		static_cast<uint8>(ESolidStrongholdType::Fortress));
	TestEqual(
		TEXT("the stronghold size is Humble"),
		static_cast<uint8>(FortressCompany->GetStronghold()->Size),
		static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestEqual(TEXT("the company is on the fortress site"), FortressCompany->GetStronghold()->Cell, HumbleCell);
	TestEqual(TEXT("the cave is the fortress history"), FortressState.PreviousStrongholds.Num(), 1);
	TestEqual(TEXT("the starting pieces moved onto the fortress"), FortressCompany->GetStronghold()->Furnishings.Num(), CavePieces.Num());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidStrongholdChoiceTest,
	"SolidCore1.Stronghold.Choice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidStrongholdChoiceTest::RunTest(const FString& Parameters)
{
	ASolidHUD* HUD = NewObject<ASolidHUD>();
	TestNotNull(TEXT("hud"), HUD);
	if (!HUD)
	{
		return false;
	}

	TArray<FSolidStrongholdChoice> None;
	HUD->OpenStrongholdChoice(FIntPoint(1, 2), None);
	TestFalse(TEXT("an empty list does not ask"), HUD->IsStrongholdChoiceOpen());

	HUD->HandleMenuKey();
	TestTrue(TEXT("the menu is open"), HUD->IsMainMenuOpen());
	TArray<FSolidStrongholdChoice> Choices;
	FSolidStrongholdChoice HumbleDungeon;
	HumbleDungeon.Size = ESolidStrongholdSize::Humble;
	HumbleDungeon.Type = ESolidStrongholdType::Dungeon;
	FSolidStrongholdChoice ImperiousTower;
	ImperiousTower.Size = ESolidStrongholdSize::Imperious;
	ImperiousTower.Type = ESolidStrongholdType::Tower;
	Choices.Add(HumbleDungeon);
	Choices.Add(ImperiousTower);
	const FIntPoint Cell(35, 15);
	HUD->OpenStrongholdChoice(Cell, Choices);
	TestTrue(TEXT("the choice asks"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("the choice closes the menu"), HUD->IsMainMenuOpen());
	TestEqual(TEXT("the first row is selected"), HUD->GetStrongholdChoiceIndex(), 0);
	TestEqual(TEXT("both rows are listed"), HUD->GetStrongholdChoiceCount(), 2);
	TestEqual(TEXT("the choice keeps the site column"), HUD->GetStrongholdChoiceCell().X, Cell.X);
	TestEqual(TEXT("the choice keeps the site row"), HUD->GetStrongholdChoiceCell().Y, Cell.Y);
	TestEqual(
		TEXT("the first row is Humble"),
		static_cast<uint8>(HUD->GetStrongholdChoiceSize()),
		static_cast<uint8>(ESolidStrongholdSize::Humble));
	TestEqual(
		TEXT("the first row is a dungeon"),
		static_cast<uint8>(HUD->GetStrongholdChoiceType()),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));

	HUD->MoveStrongholdChoice(1);
	TestEqual(TEXT("down selects the tower"), HUD->GetStrongholdChoiceIndex(), 1);
	TestEqual(
		TEXT("the selected row is Imperious"),
		static_cast<uint8>(HUD->GetStrongholdChoiceSize()),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(
		TEXT("the selected row is a tower"),
		static_cast<uint8>(HUD->GetStrongholdChoiceType()),
		static_cast<uint8>(ESolidStrongholdType::Tower));
	HUD->MoveStrongholdChoice(1);
	TestEqual(TEXT("down wraps"), HUD->GetStrongholdChoiceIndex(), 0);
	HUD->SetStrongholdChoiceIndex(1);
	TestEqual(TEXT("a number key selects the tower"), HUD->GetStrongholdChoiceIndex(), 1);
	HUD->SetStrongholdChoiceIndex(9);
	TestEqual(TEXT("a missing row does not change the selection"), HUD->GetStrongholdChoiceIndex(), 1);

	HUD->HandleMenuKey();
	TestFalse(TEXT("F10 leaves the site"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("leaving the choice does not open the menu"), HUD->IsMainMenuOpen());

	HUD->OpenStrongholdChoice(Cell, Choices);
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc leaves the site"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("Esc does not open the menu"), HUD->IsMainMenuOpen());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
