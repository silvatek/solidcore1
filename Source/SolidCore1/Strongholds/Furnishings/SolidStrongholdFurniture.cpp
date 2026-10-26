#include "SolidStrongholdFurniture.h"
#include "Actors/SolidBed.h"
#include "Actors/SolidChest.h"
#include "Actors/SolidFireplace.h"
#include "Actors/SolidSmallTable.h"
#include "Strongholds/SolidStronghold.h"

int32 SolidStrongholdFurniture::HexCount(const EKind Kind)
{
	const ASolidFurnishing* Piece = Cast<ASolidFurnishing>(ActorClass(Kind)->GetDefaultObject());
	return Piece ? Piece->HexCount() : 1;
}

UClass* SolidStrongholdFurniture::ActorClass(const EKind Kind)
{
	switch (Kind)
	{
	case EKind::Fireplace:
		return ASolidFireplace::StaticClass();
	case EKind::Bed:
		return ASolidBed::StaticClass();
	case EKind::Chest:
		return ASolidChest::StaticClass();
	case EKind::SmallTable:
		return ASolidSmallTable::StaticClass();
	default:
		checkNoEntry();
		return ASolidFireplace::StaticClass();
	}
}

void SolidStrongholdFurniture::UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FUsed>& OutUsed)
{
	OutUsed.Reset();
	if (Stronghold.Type != ESolidStrongholdType::Dungeon
		|| Stronghold.Size != ESolidStrongholdSize::Meagre)
	{
		return;
	}

	OutUsed.Add({ IndexOfKind(EKind::Fireplace), 0, 2, 0, 0.f });
	OutUsed.Add({ IndexOfKind(EKind::Bed), 0, 0, -1, 0.f });
	OutUsed.Add({ IndexOfKind(EKind::Chest), 0, 2, 1, 0.f });
}

TArray<SolidStrongholdFurniture::FUsed> SolidStrongholdFurniture::StartingFurnishings()
{
	TArray<FUsed> Used;
	UsedFurnishings(
		FSolidStronghold(ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre),
		Used);
	return Used;
}
