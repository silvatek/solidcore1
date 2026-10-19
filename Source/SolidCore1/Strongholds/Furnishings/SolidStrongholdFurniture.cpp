#include "SolidStrongholdFurniture.h"
#include "Strongholds/SolidStronghold.h"

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
