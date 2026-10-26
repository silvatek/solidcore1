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
	 * The row stores how many floor cells it covers, and how many slots it holds.
	 * bEnabled on the session's copy is the unlock. The catalogue holds the new-game value.
	 */
	class FItem
	{
	public:
		FItem() = default;
		FItem(
			const TCHAR* InName,
			UClass* InActorClass,
			int32 InHexes,
			int32 InLimit,
			bool bInEnabled,
			int32 InStorageSlots = 0);

		const TCHAR* Name = TEXT("");
		UClass* ActorClass = nullptr;
		/** How many slots the piece holds. Zero holds nothing. The chest has six. */
		int32 StorageSlots = 0;
		/** How many of this kind one stronghold holds, on every floor, including a piece that is held. */
		int32 Limit = 1;
		/** A new game starts from the recipe. The session copy is the flag that changes. */
		bool bEnabled = false;

		/** Cells this kind covers on the floor. */
		int32 HexCount() const;
		/** True when this kind holds items. */
		bool IsStorage() const;

	private:
		/** Floor cells. One for a fireplace, a chest, and a small table. Two for a bed. */
		int32 Hexes = 1;
	};

	/** The catalogue. The session copies this for a new game. */
	const TArray<FItem>& FurnitureCatalogue();

	/** A fresh copy of FurnitureCatalogue for a new game. */
	TArray<FItem> StartingCatalog();

	/** A catalogue row. This is not the session copy, so its bEnabled is the new-game value. */
	const FItem* ItemAt(int32 Index);

	/** Index of the recipe row whose actor class is Class. */
	int32 IndexOfActor(const UClass* ActorClass);

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
		if (Item->HexCount() < 2)
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
