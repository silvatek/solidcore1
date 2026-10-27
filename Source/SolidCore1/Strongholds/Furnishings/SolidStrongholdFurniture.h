#pragma once

#include "CoreMinimal.h"
#include "Inventory/SolidStorage.h"
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

	/**
	 * One piece placed on a stronghold floor.
	 * ItemIndex is the catalogue row. The cell and the facing belong to this floor.
	 * Slots are the items this piece holds.
	 */
	class FPlacedFurnishing
	{
	public:
		int32 ItemIndex = 0;
		/** 0 is the ground floor. Meagre strongholds use only this level. */
		int32 Level = 0;
		/** Hex column. */
		int32 X = 0;
		/** Hex row. */
		int32 Y = 0;
		float RotationDeg = 0.f;
		/**
		 * Items in this piece. The length is the kind's StorageSlots.
		 * An empty item is an empty cell. A fireplace and a bed have none.
		 */
		SolidStorage::FSlots Slots;

		/** Second cell in the direction this piece is rotated. The anchor when it occupies one. */
		const SolidStrongholdGrid::FHex* SpanCell(const TArray<SolidStrongholdGrid::FHex>& Hexes) const;

		/** True when the anchor or the second cell is this column and row. */
		bool Covers(const TArray<SolidStrongholdGrid::FHex>& Hexes, int32 Col, int32 Row) const;

		/** Empty slots for this kind. A fireplace and a bed get none. A chest gets Count. */
		void GiveSlots();

		/** A copy of these slots as the chest grid. Cells past Slots stay empty. */
		SolidStorage::FContainer Container() const;

		/**
		 * Write the chest grid onto Slots.
		 * A kind that holds nothing keeps an empty list.
		 */
		void SetContainer(const SolidStorage::FContainer& Container);
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

	/**
	 * Furnishings in a stronghold. The starting Meagre Dungeon is the cave.
	 * Every entry is level 0 when the stronghold is Meagre.
	 */
	void UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FPlacedFurnishing>& OutUsed);

	/** The pieces a new game carries. The Meagre stronghold is where they start. */
	TArray<FPlacedFurnishing> StartingFurnishings();

	/**
	 * Spawn Used on Hexes and attach each piece to Owner.
	 * ToLocal maps a hex-space midpoint and facing into the owner's local space.
	 * A second call leaves pieces that already match.
	 */
	int32 PlaceFurnishings(
		AActor* Owner,
		const TArray<FPlacedFurnishing>& Used,
		const TArray<SolidStrongholdGrid::FHex>& Hexes,
		TFunctionRef<FTransform(const FVector2D& Mid, float YawDeg)> ToLocal);

	/** Hex space is the owner's local space. */
	FTransform FloorLocal(const FVector2D& Mid, float YawDeg);
}
