#include "SolidStrongholdFurniture.h"
#include "Actors/SolidBed.h"
#include "Actors/SolidChest.h"
#include "Actors/SolidFireplace.h"
#include "Actors/SolidSmallTable.h"
#include "Inventory/SolidStorage.h"
#include "Strongholds/SolidStronghold.h"

namespace
{
	SolidStrongholdFurniture::FItem MakeItem(
		const TCHAR* Name,
		UClass* ActorClass,
		const int32 Limit,
		const bool bEnabled)
	{
		SolidStrongholdFurniture::FItem Item;
		Item.Name = Name;
		Item.ActorClass = ActorClass;
		Item.StorageSlots = 0;
		Item.Limit = Limit;
		Item.bEnabled = bEnabled;
		return Item;
	}

	SolidStrongholdFurniture::FItem MakeStorage(
		const TCHAR* Name,
		UClass* ActorClass,
		const int32 Limit,
		const bool bEnabled,
		const int32 StorageSlots)
	{
		SolidStrongholdFurniture::FItem Item = MakeItem(Name, ActorClass, Limit, bEnabled);
		Item.StorageSlots = StorageSlots;
		return Item;
	}
}

const TArray<SolidStrongholdFurniture::FItem>& SolidStrongholdFurniture::FurnitureCatalogue()
{
	static const TArray<FItem> Catalogue = {
		MakeItem(TEXT("Fireplace"), ASolidFireplace::StaticClass(), 1, true),
		MakeItem(TEXT("Bed"), ASolidBed::StaticClass(), 3, true),
		MakeStorage(TEXT("Chest"), ASolidChest::StaticClass(), 1, true, SolidStorage::Count),
		MakeItem(TEXT("Small Table"), ASolidSmallTable::StaticClass(), 10, true),
	};
	return Catalogue;
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
	return FurnitureCatalogue();
}

const SolidStrongholdFurniture::FItem* SolidStrongholdFurniture::ItemAt(const int32 Index)
{
	const TArray<FItem>& Catalogue = FurnitureCatalogue();
	return Catalogue.IsValidIndex(Index) ? &Catalogue[Index] : nullptr;
}

int32 SolidStrongholdFurniture::IndexOfActor(const UClass* ActorClass)
{
	const TArray<FItem>& Catalogue = FurnitureCatalogue();
	for (int32 Index = 0; Index < Catalogue.Num(); ++Index)
	{
		if (Catalogue[Index].ActorClass == ActorClass)
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
