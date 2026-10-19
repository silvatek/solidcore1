#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidGameState.h"
#include "SolidHUD.h"
#include "SolidInventory.h"
#include "SolidStronghold.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdTransition.h"
#include "SolidTowerStronghold.h"
#include "SolidWorldMap.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	void Unlock(FGameState& State, const ESolidStrongholdSize Size)
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

	bool SameChest(const SolidStorage::FContainer& A, const SolidStorage::FContainer& B)
	{
		for (int32 Index = 0; Index < SolidStorage::Count; ++Index)
		{
			if (A.Counts[Index] != B.Counts[Index] || !SameId(A.Slots[Index], B.Slots[Index]))
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
			if (A.WornStack[Index] != B.WornStack[Index] || !SameId(A.Worn[Index], B.Worn[Index]))
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < SolidInventory::PackCount; ++Index)
		{
			if (A.PackStack[Index] != B.PackStack[Index] || !SameId(A.Pack[Index], B.Pack[Index]))
			{
				return false;
			}
		}
		for (int32 Index = 0; Index < SolidInventory::CraftCount; ++Index)
		{
			if (A.CraftStack[Index] != B.CraftStack[Index] || !SameId(A.Craft[Index], B.Craft[Index]))
			{
				return false;
			}
		}
		return true;
	}

	bool SamePieces(
		const TArray<SolidStrongholdFurniture::FUsed>& A,
		const TArray<SolidStrongholdFurniture::FUsed>& B)
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

	int32 BuiltTypeCount(const ESolidStrongholdSize Size)
	{
		int32 Count = 0;
		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			if (SolidStrongholdTransition::IsBuilt(FSolidStronghold{ Type, Size }))
			{
				++Count;
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
	TestFalse(
		TEXT("a Humble fortress is not built"),
		SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Fortress, ESolidStrongholdSize::Humble }));

	for (const ESolidStrongholdSize Size : SolidStronghold::Sizes)
	{
		if (static_cast<int32>(Size) < static_cast<int32>(ESolidStrongholdSize::Average))
		{
			continue;
		}
		TestTrue(
			*FString::Printf(TEXT("%s tower is built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Tower, Size }));
		TestFalse(
			*FString::Printf(TEXT("%s dungeon is not built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Dungeon, Size }));
		TestFalse(
			*FString::Printf(TEXT("%s fortress is not built"), SolidStronghold::SizeName(Size)),
			SolidStrongholdTransition::IsBuilt(FSolidStronghold{ ESolidStrongholdType::Fortress, Size }));
	}

	FGameState State;
	State.bCaveEnabled = true;
	TestTrue(TEXT("the open stronghold still accepts enter"), SolidStrongholdTransition::CompanyCaveAcceptsEnter(State));

	TArray<ESolidStrongholdType> Types;
	TestEqual(
		TEXT("a locked size does nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Imperious, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(TEXT("a locked size lists nothing"), Types.Num(), 0);

	Unlock(State, ESolidStrongholdSize::Meagre);
	TestEqual(
		TEXT("Meagre is not a site choice"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Meagre, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));

	Unlock(State, ESolidStrongholdSize::Humble);
	TestEqual(
		TEXT("a size with no built type does nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Humble, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(TEXT("Humble lists nothing"), Types.Num(), 0);

	Unlock(State, ESolidStrongholdSize::Imperious);
	TestEqual(
		TEXT("an unlocked larger size asks"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Imperious, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));
	TestEqual(TEXT("Imperious lists the built type"), Types.Num(), 1);
	TestEqual(
		TEXT("Imperious offers a tower"),
		static_cast<uint8>(Types[0]),
		static_cast<uint8>(ESolidStrongholdType::Tower));

	State.ActiveStronghold = SolidStronghold::Make(
		ESolidStrongholdType::Tower,
		ESolidStrongholdSize::Average,
		FIntPoint::ZeroValue,
		SolidStronghold::WorldXY(FIntPoint::ZeroValue));
	Unlock(State, ESolidStrongholdSize::Average);
	TestEqual(
		TEXT("the same size does nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Average, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(
		TEXT("a smaller size does nothing"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Humble, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
	TestEqual(
		TEXT("a larger size still asks"),
		static_cast<uint8>(SolidStrongholdTransition::Consider(State, ESolidStrongholdSize::Imperious, Types)),
		static_cast<uint8>(SolidStrongholdTransition::EOffer::Choose));

	USolidWorldMap* Map = NewObject<USolidWorldMap>();
	TestNotNull(TEXT("world map"), Map);
	TestTrue(TEXT("shipped map loads"), Map && Map->LoadDefault());
	if (!Map)
	{
		return false;
	}

	int32 Sites = 0;
	for (int32 Row = 0; Row < USolidWorldMap::MapHeight; ++Row)
	{
		for (int32 Col = 0; Col < USolidWorldMap::MapWidth; ++Col)
		{
			ESolidStrongholdSize SiteSize = ESolidStrongholdSize::Meagre;
			if (!Map->StrongholdSiteAtCell(Col, Row, SiteSize))
			{
				continue;
			}
			++Sites;
			FGameState Fresh;
			TArray<ESolidStrongholdType> Offered;
			TestEqual(
				*FString::Printf(TEXT("site %d,%d does nothing until unlocked"), Row + 1, Col + 1),
				static_cast<uint8>(SolidStrongholdTransition::Consider(Fresh, SiteSize, Offered)),
				static_cast<uint8>(SolidStrongholdTransition::EOffer::None));
			Unlock(Fresh, SiteSize);
			const SolidStrongholdTransition::EOffer Offer = SolidStrongholdTransition::Consider(Fresh, SiteSize, Offered);
			const int32 Built = BuiltTypeCount(SiteSize);
			if (Built == 0)
			{
				TestEqual(
					*FString::Printf(TEXT("site %d,%d has no built type"), Row + 1, Col + 1),
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
					*FString::Printf(TEXT("site %d,%d lists built types"), Row + 1, Col + 1),
					Offered.Num(),
					Built);
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

	FGameState State;
	State.bCaveEnabled = true;
	SolidStorage::WriteSlot(State.CaveChest, SolidStorage::Index(0, 0), SolidItemType::IdRustySword, 4);
	SolidInventory::WritePack(State.Inventory, 0, SolidItemType::IdStick, 2);
	const SolidStorage::FContainer Chest = State.CaveChest;
	const SolidInventory::FLoadout Pack = State.Inventory;
	const TArray<SolidStrongholdFurniture::FUsed> Started = State.Furnishings;

	TestFalse(
		TEXT("a locked size does not commit"),
		SolidStrongholdTransition::Commit(State, AverageCell, ESolidStrongholdSize::Average, ESolidStrongholdType::Tower));
	Unlock(State, ESolidStrongholdSize::Imperious);
	TestFalse(
		TEXT("a type that was not offered does not commit"),
		SolidStrongholdTransition::Commit(State, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("a refused type leaves the stronghold"), static_cast<uint8>(State.ActiveStronghold->Type), static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("a refused type leaves the size"), static_cast<uint8>(State.ActiveStronghold->Size), static_cast<uint8>(ESolidStrongholdSize::Meagre));
	TestEqual(TEXT("a refused type leaves the cell"), State.ActiveStronghold->Cell, SolidStronghold::StartingCell());
	TestEqual(TEXT("a refused type leaves no history"), State.PreviousStrongholds.Num(), 0);
	TestTrue(TEXT("a refused type leaves the pieces"), SamePieces(State.Furnishings, Started));
	TestTrue(TEXT("a refused type leaves the chest"), SameChest(State.CaveChest, Chest));
	TestTrue(TEXT("a refused type leaves the pack"), SameLoadout(State.Inventory, Pack));

	State.Furnishings.Reset();
	TestTrue(
		TEXT("an empty furnishing list still moves the starting pieces"),
		SolidStrongholdTransition::Commit(State, AverageCell, ESolidStrongholdSize::Average, ESolidStrongholdType::Tower));
	TestEqual(TEXT("the company is in a tower"), static_cast<uint8>(State.ActiveStronghold->Type), static_cast<uint8>(ESolidStrongholdType::Tower));
	TestEqual(TEXT("the stronghold size is average"), static_cast<uint8>(State.ActiveStronghold->Size), static_cast<uint8>(ESolidStrongholdSize::Average));
	TestEqual(TEXT("the company is on the site"), State.ActiveStronghold->Cell, AverageCell);
	TestEqual(TEXT("one previous stronghold is recorded"), State.PreviousStrongholds.Num(), 1);
	TestEqual(
		TEXT("the previous stronghold is a Dungeon"),
		static_cast<uint8>(State.PreviousStrongholds[0]->Type),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));
	TestEqual(TEXT("the previous stronghold keeps its cell"), State.PreviousStrongholds[0]->Cell, SolidStronghold::StartingCell());
	TestFalse(TEXT("the stronghold no longer accepts enter"), SolidStrongholdTransition::CompanyCaveAcceptsEnter(State));
	TestTrue(TEXT("the chest moved with the company"), SameChest(State.CaveChest, Chest));
	TestTrue(TEXT("the pack stayed with the captain"), SameLoadout(State.Inventory, Pack));
	TestEqual(TEXT("the starting pieces moved"), State.Furnishings.Num(), Started.Num());
	TestFalse(TEXT("a map tower door is not a passage"), SolidStrongholdTransition::TowerDoorAcceptsEnter(State, MapTowerCell));

	SolidTowerStronghold::FPlan AveragePlan;
	TestTrue(TEXT("Average has a plan"), SolidTowerStronghold::PlanFor(*State.ActiveStronghold, AveragePlan));
	TArray<SolidStrongholdGrid::FHex> AverageHexes;
	SolidTowerStronghold::CollectKeptHexes(AveragePlan, 0, AverageHexes);
	for (const SolidStrongholdFurniture::FUsed& Piece : State.Furnishings)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Piece.ItemIndex);
		TestTrue(TEXT("moved piece"), Item != nullptr);
		TestEqual(TEXT("pieces sit on the ground floor"), Piece.Level, 0);
		const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(AverageHexes, Piece.X, Piece.Y);
		TestTrue(TEXT("piece cell is kept"), Anchor != nullptr);
		if (Item && SolidStrongholdFurniture::HexCount(Item->Kind) >= 2)
		{
			const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(AverageHexes, Piece);
			TestTrue(TEXT("the bed spans a second cell"), Span != nullptr);
			TestTrue(TEXT("the bed's cells are neighbors"), Anchor && Span && SolidStrongholdTransition::AreNeighbors(*Anchor, *Span));
		}
	}

	TestTrue(
		TEXT("the next size commits"),
		SolidStrongholdTransition::Commit(State, ImperiousCell, ESolidStrongholdSize::Imperious, ESolidStrongholdType::Tower));
	TestEqual(TEXT("the company is Imperious"), static_cast<uint8>(State.ActiveStronghold->Size), static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(TEXT("the company is on the later site"), State.ActiveStronghold->Cell, ImperiousCell);
	TestEqual(TEXT("two previous strongholds are recorded"), State.PreviousStrongholds.Num(), 2);
	TestEqual(TEXT("the later previous stronghold is the Average tower"), State.PreviousStrongholds[1]->Cell, AverageCell);
	TestFalse(TEXT("a previous tower door is not a passage"), SolidStrongholdTransition::TowerDoorAcceptsEnter(State, AverageCell));
	TestFalse(TEXT("another tower door is not a passage"), SolidStrongholdTransition::TowerDoorAcceptsEnter(State, MapTowerCell));
	TestTrue(TEXT("the active door is not a previous door"), SolidStrongholdTransition::TowerDoorAcceptsEnter(State, ImperiousCell));
	TestTrue(TEXT("the chest still holds the same slots"), SameChest(State.CaveChest, Chest));
	TestTrue(TEXT("the pack is still the captain's"), SameLoadout(State.Inventory, Pack));

	SolidTowerStronghold::FPlan ImperiousPlan;
	TestTrue(TEXT("Imperious has a plan"), SolidTowerStronghold::PlanFor(*State.ActiveStronghold, ImperiousPlan));
	TArray<SolidStrongholdGrid::FHex> ImperiousHexes;
	SolidTowerStronghold::CollectKeptHexes(ImperiousPlan, 0, ImperiousHexes);
	int32 Beds = 0;
	for (const SolidStrongholdFurniture::FUsed& Piece : State.Furnishings)
	{
		const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Piece.ItemIndex);
		TestTrue(TEXT("piece in the new stronghold"), Item != nullptr);
		TestTrue(TEXT("new cell is kept"), SolidStrongholdFurniture::FindCell(ImperiousHexes, Piece.X, Piece.Y) != nullptr);
		if (Item && Item->Kind == SolidStrongholdFurniture::EKind::Bed)
		{
			++Beds;
			const SolidStrongholdGrid::FHex* Anchor = SolidStrongholdFurniture::FindCell(ImperiousHexes, Piece.X, Piece.Y);
			const SolidStrongholdGrid::FHex* Span = SolidStrongholdFurniture::SpanCell(ImperiousHexes, Piece);
			TestTrue(TEXT("the bed still spans neighbors"), Anchor && Span && SolidStrongholdTransition::AreNeighbors(*Anchor, *Span));
		}
	}
	TestEqual(TEXT("the bed moved"), Beds, 1);

	TestFalse(
		TEXT("inside the tower the chest is not the stronghold"),
		SolidStrongholdTransition::CompanyChestOpen(State, true, false));
	TestTrue(
		TEXT("the chest opens in the company tower"),
		SolidStrongholdTransition::CompanyChestOpen(State, false, true));

	TArray<SolidStrongholdFurniture::FUsed> Stuck = Started;
	TestFalse(TEXT("pieces do not sit on an empty floor"), SolidStrongholdTransition::PlaceOnFloor(Stuck, TArray<SolidStrongholdGrid::FHex>()));
	TestTrue(TEXT("a failed placement leaves the pieces"), SamePieces(Stuck, Started));
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

	TArray<ESolidStrongholdType> None;
	HUD->OpenStrongholdChoice(FIntPoint(1, 2), ESolidStrongholdSize::Humble, None);
	TestFalse(TEXT("an empty list does not ask"), HUD->IsStrongholdChoiceOpen());

	HUD->HandleMenuKey();
	TestTrue(TEXT("the menu is open"), HUD->IsMainMenuOpen());
	TArray<ESolidStrongholdType> Types;
	Types.Add(ESolidStrongholdType::Dungeon);
	Types.Add(ESolidStrongholdType::Tower);
	const FIntPoint Cell(35, 15);
	HUD->OpenStrongholdChoice(Cell, ESolidStrongholdSize::Imperious, Types);
	TestTrue(TEXT("the choice asks"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("the choice closes the menu"), HUD->IsMainMenuOpen());
	TestEqual(TEXT("the first type is selected"), HUD->GetStrongholdChoiceIndex(), 0);
	TestEqual(TEXT("both types are listed"), HUD->GetStrongholdChoiceCount(), 2);
	TestEqual(TEXT("the choice keeps the site column"), HUD->GetStrongholdChoiceCell().X, Cell.X);
	TestEqual(TEXT("the choice keeps the site row"), HUD->GetStrongholdChoiceCell().Y, Cell.Y);
	TestEqual(
		TEXT("the choice keeps the site size"),
		static_cast<uint8>(HUD->GetStrongholdChoiceSize()),
		static_cast<uint8>(ESolidStrongholdSize::Imperious));
	TestEqual(
		TEXT("the first row is a dungeon"),
		static_cast<uint8>(HUD->GetStrongholdChoiceType()),
		static_cast<uint8>(ESolidStrongholdType::Dungeon));

	HUD->MoveStrongholdChoice(1);
	TestEqual(TEXT("down selects the tower"), HUD->GetStrongholdChoiceIndex(), 1);
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
	TestFalse(TEXT("F10 leaves the stone"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("leaving the choice does not open the menu"), HUD->IsMainMenuOpen());

	HUD->OpenStrongholdChoice(Cell, ESolidStrongholdSize::Imperious, Types);
	HUD->CloseMenuOverlay();
	TestFalse(TEXT("Esc leaves the stone"), HUD->IsStrongholdChoiceOpen());
	TestFalse(TEXT("Esc does not open the menu"), HUD->IsMainMenuOpen());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
