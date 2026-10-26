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
	/**
	 * One kind of furnishing. The actor class is the kind.
	 * A storage piece holds items.
	 * bEnabled on the session's copy is the unlock. The recipe holds the new-game value.
	 */
	struct FItem
	{
		const TCHAR* Name = TEXT("");
		UClass* ActorClass = nullptr;
		/** True when the piece holds items. The chest does. The fireplace, the bed, and the small table do not. */
		bool bStorage = false;
		/** How many of this kind one stronghold holds, on every floor, including a piece that is held. */
		int32 Limit = 1;
		/** A new game starts from the recipe. The session copy is the flag that changes. */
		bool bEnabled = false;
	};

	/** Cells the piece covers. The actor for that row answers. */
	int32 HexCount(const FItem& Item);

	/** The new-game catalog. Each call returns a fresh copy of the recipe. */
	TArray<FItem> StartingCatalog();

	/** The recipe row. This is not the session copy, so its bEnabled is the new-game value. */
	const FItem* ItemAt(int32 Index);

	/** Index of the recipe row whose actor class is Class. */
	int32 IndexOfActor(const UClass* ActorClass);

	inline bool IsStorage(const FItem* Item)
	{
		return Item && Item->bStorage;
	}

	/** A piece placed in a stronghold. ItemIndex is the catalog row. */
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
		if (HexCount(*Item) < 2)
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
