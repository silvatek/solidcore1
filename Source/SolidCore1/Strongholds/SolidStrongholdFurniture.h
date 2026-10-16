#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"
#include "SolidStrongholdGrid.h"

/**
 * The first stronghold furnishings. Sizes are in hex cells of the floor grid.
 * A fireplace and a chest take one cell. A bed takes two neighboring cells.
 */
namespace SolidStrongholdFurniture
{
	enum class EKind : uint8
	{
		Fireplace,
		Bed,
		Chest,
	};

	inline int32 HexCount(const EKind Kind)
	{
		return Kind == EKind::Bed ? 2 : 1;
	}

	inline FLinearColor StoneColor()
	{
		return FLinearColor(0.45f, 0.45f, 0.47f);
	}

	inline FLinearColor EmberColor()
	{
		return FLinearColor(0.72f, 0.12f, 0.06f);
	}

	inline FLinearColor BeigeColor()
	{
		return FLinearColor(0.78f, 0.66f, 0.45f);
	}

	inline FLinearColor PillowColor()
	{
		return FLinearColor(0.94f, 0.93f, 0.90f);
	}

	/** Grey hearth, slightly inside one cell so the yellow outline still shows. */
	inline float FireplaceAcrossCm()
	{
		return SolidStrongholdGrid::FlatToFlatCm * 0.90f;
	}

	inline float FireplaceHearthCm()
	{
		return 30.f;
	}

	inline float FireplaceEmberRadiusCm()
	{
		return FireplaceAcrossCm() * 0.40f;
	}

	inline float FireplaceEmberHeightCm()
	{
		// Was 20 cm. About 60 percent of that height is gone.
		return 8.f;
	}

	/** Long axis covers two cells. */
	inline float BedLengthCm()
	{
		return SolidStrongholdGrid::FlatToFlatCm * 2.f - 20.f;
	}

	/** Narrow enough to pass through the flat the two cells share. */
	inline float BedWidthCm()
	{
		return SolidStrongholdGrid::SideCm() - SolidStrongholdGrid::LineThicknessCm * 1.5f;
	}

	inline float BedHeightCm()
	{
		return 42.f;
	}

	inline float PillowLengthCm()
	{
		return 52.f;
	}

	inline float PillowWidthCm()
	{
		return BedWidthCm() * 0.70f;
	}

	inline float PillowHeightCm()
	{
		return 14.f;
	}

	inline float ChestLengthCm()
	{
		return 76.f;
	}

	inline float ChestWidthCm()
	{
		return 48.f;
	}

	inline float ChestBodyHeightCm()
	{
		return 36.f;
	}

	/** Lid radius matches half the chest width, so the dome is a half cylinder. */
	inline float ChestLidRadiusCm()
	{
		return ChestWidthCm() * 0.5f;
	}

	/** One kind of furnishing. The mesh is built from Kind. A storage piece holds items. */
	struct FItem
	{
		const TCHAR* Name = TEXT("");
		EKind Kind = EKind::Fireplace;
		/** True when the piece holds items. The chest does. The fireplace and the bed do not. */
		bool bStorage = false;
	};

	/** Every furnishing the game knows. Strongholds point back into this list. */
	inline const FItem Catalog[] = {
		{ TEXT("Fireplace"), EKind::Fireplace, false },
		{ TEXT("Bed"), EKind::Bed, false },
		{ TEXT("Chest"), EKind::Chest, true },
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
	inline void UsedFurnishings(const FSolidStronghold& Stronghold, TArray<FUsed>& OutUsed)
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
}
