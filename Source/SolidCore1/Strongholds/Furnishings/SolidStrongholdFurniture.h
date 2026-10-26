#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdGrid.h"

class FSolidStronghold;
class UClass;

/**
 * Stronghold furnishings. Sizes are in hex cells of the floor grid.
 * A fireplace, a chest, and a small table take one cell. A bed takes two neighboring cells.
 */
namespace SolidStrongholdFurniture
{
	enum class EKind : uint8
	{
		Fireplace,
		Bed,
		Chest,
		SmallTable,
	};

	/** Cells the piece covers. The actor for that kind answers. */
	int32 HexCount(EKind Kind);

	/** The actor spawned for a catalog kind. */
	UClass* ActorClass(EKind Kind);

	/** One kind of furnishing. The actor is built from Kind. A storage piece holds items. */
	struct FItem
	{
		const TCHAR* Name = TEXT("");
		EKind Kind = EKind::Fireplace;
		/** True when the piece holds items. The chest does. The fireplace, the bed, and the small table do not. */
		bool bStorage = false;
		/** How many of this kind one stronghold holds, on every floor, including a piece that is held. */
		int32 Limit = 1;
	};

	/** Every furnishing the game knows. Strongholds point back into this list. */
	inline const FItem Catalog[] = {
		{ TEXT("Fireplace"), EKind::Fireplace, false, 1 },
		{ TEXT("Bed"), EKind::Bed, false, 3 },
		{ TEXT("Chest"), EKind::Chest, true, 1 },
		{ TEXT("Small Table"), EKind::SmallTable, false, 10 },
	};

	inline constexpr int32 CatalogCount = UE_ARRAY_COUNT(Catalog);

	inline const FItem* ItemAt(const int32 Index)
	{
		return Index >= 0 && Index < CatalogCount ? &Catalog[Index] : nullptr;
	}

	inline bool IsStorage(const FItem* Item)
	{
		return Item && Item->bStorage;
	}

	inline int32 IndexOfKind(const EKind Kind)
	{
		for (int32 Index = 0; Index < CatalogCount; ++Index)
		{
			if (Catalog[Index].Kind == Kind)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** A piece placed in a stronghold. ItemIndex refers to Catalog. */
	struct FUsed
	{
		int32 ItemIndex = 0;
		/** 0 is the ground floor. Meagre strongholds use only this level. */
		int32 Level = 0;
		/** Hex column. */
		int32 X = 0;
		/** Hex row. */
		int32 Y = 0;
		float RotationDeg = 0.f;
	};

	inline const SolidStrongholdGrid::FHex* FindCell(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const int32 X,
		const int32 Y)
	{
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			if (Hex.Col == X && Hex.Row == Y)
			{
				return &Hex;
			}
		}
		return nullptr;
	}

	/** Second cell for a multi-hex piece, in the direction it is rotated. Same cell when it occupies one. */
	inline const SolidStrongholdGrid::FHex* SpanCell(
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		const FUsed& Used)
	{
		const FItem* Item = ItemAt(Used.ItemIndex);
		const SolidStrongholdGrid::FHex* Anchor = FindCell(Hexes, Used.X, Used.Y);
		if (!Item || !Anchor)
		{
			return nullptr;
		}
		if (HexCount(Item->Kind) < 2)
		{
			return Anchor;
		}

		const FVector Forward = FRotator(0.f, Used.RotationDeg, 0.f).Vector();
		const FVector2D Target = Anchor->Center
			+ FVector2D(Forward.X, Forward.Y) * SolidStrongholdGrid::FlatToFlatCm;
		const SolidStrongholdGrid::FHex* Best = nullptr;
		float BestDist = SolidStrongholdGrid::FlatToFlatCm * 0.45f;
		for (const SolidStrongholdGrid::FHex& Hex : Hexes)
		{
			const float Dist = FVector2D::Distance(Hex.Center, Target);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				Best = &Hex;
			}
		}
		return Best;
	}

	/**
	 * Furnishings in a stronghold. The starting Meagre Dungeon is the cave.
	 * Every entry is level 0 when the stronghold is Meagre.
	 */
	void UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FUsed>& OutUsed);

	/** The pieces a new game carries. The Meagre stronghold is where they start. */
	TArray<FUsed> StartingFurnishings();
}
