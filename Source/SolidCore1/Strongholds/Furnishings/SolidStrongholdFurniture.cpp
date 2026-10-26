#include "SolidStrongholdFurniture.h"
#include "Actors/SolidBed.h"
#include "Actors/SolidChest.h"
#include "Actors/SolidFireplace.h"
#include "Actors/SolidSmallTable.h"
#include "Strongholds/SolidStronghold.h"

namespace
{
	SolidStrongholdFurniture::FItem MakeItem(
		const TCHAR* Name,
		UClass* ActorClass,
		const bool bStorage,
		const int32 Limit,
		const bool bEnabled)
	{
		SolidStrongholdFurniture::FItem Item;
		Item.Name = Name;
		Item.ActorClass = ActorClass;
		Item.bStorage = bStorage;
		Item.Limit = Limit;
		Item.bEnabled = bEnabled;
		return Item;
	}

	SolidStrongholdFurniture::FItem Fireplace()
	{
		return MakeItem(TEXT("Fireplace"), ASolidFireplace::StaticClass(), false, 1, true);
	}

	SolidStrongholdFurniture::FItem Bed()
	{
		return MakeItem(TEXT("Bed"), ASolidBed::StaticClass(), false, 3, true);
	}

	SolidStrongholdFurniture::FItem Chest()
	{
		return MakeItem(TEXT("Chest"), ASolidChest::StaticClass(), true, 1, true);
	}

	SolidStrongholdFurniture::FItem SmallTable()
	{
		return MakeItem(TEXT("Small Table"), ASolidSmallTable::StaticClass(), false, 10, true);
	}

	const TArray<SolidStrongholdFurniture::FItem>& Recipe()
	{
		static const TArray<SolidStrongholdFurniture::FItem> Catalog = {
			Fireplace(),
			Bed(),
			Chest(),
			SmallTable(),
		};
		return Catalog;
	}
}

int32 SolidStrongholdFurniture::HexCount(const FItem& Item)
{
	const ASolidFurnishing* Piece = Item.ActorClass
		? Cast<ASolidFurnishing>(Item.ActorClass->GetDefaultObject())
		: nullptr;
	return Piece ? Piece->HexCount() : 1;
}

TArray<SolidStrongholdFurniture::FItem> SolidStrongholdFurniture::StartingCatalog()
{
	return Recipe();
}

const SolidStrongholdFurniture::FItem* SolidStrongholdFurniture::ItemAt(const int32 Index)
{
	const TArray<FItem>& Catalog = Recipe();
	return Catalog.IsValidIndex(Index) ? &Catalog[Index] : nullptr;
}

int32 SolidStrongholdFurniture::IndexOfActor(const UClass* ActorClass)
{
	const TArray<FItem>& Catalog = Recipe();
	for (int32 Index = 0; Index < Catalog.Num(); ++Index)
	{
		if (Catalog[Index].ActorClass == ActorClass)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void SolidStrongholdFurniture::UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FUsed>& OutUsed)
{
	OutUsed.Reset();
	if (Stronghold.Type != ESolidStrongholdType::Dungeon
		|| Stronghold.Size != ESolidStrongholdSize::Meagre)
	{
		return;
	}

	OutUsed.Add({ IndexOfActor(ASolidFireplace::StaticClass()), 0, 2, 0, 0.f });
	OutUsed.Add({ IndexOfActor(ASolidBed::StaticClass()), 0, 0, -1, 0.f });
	OutUsed.Add({ IndexOfActor(ASolidChest::StaticClass()), 0, 2, 1, 0.f });
}

TArray<SolidStrongholdFurniture::FUsed> SolidStrongholdFurniture::StartingFurnishings()
{
	TArray<FUsed> Used;
	UsedFurnishings(
		FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre),
		Used);
	return Used;
}
