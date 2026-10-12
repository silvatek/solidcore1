#pragma once

#include "CoreMinimal.h"
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
		return 20.f;
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

	struct FPlacement
	{
		EKind Kind = EKind::Fireplace;
		int32 Col = 0;
		int32 Row = 0;
		/** Second cell. Same as Col/Row for a one-hex piece. */
		int32 SpanCol = 0;
		int32 SpanRow = 0;
	};

	/** Starter set on the Meagre den, clear of the captain at (0, 0). */
	inline void MeagreStarter(TArray<FPlacement>& OutPlacements)
	{
		OutPlacements.Reset();
		OutPlacements.Add({ EKind::Fireplace, 2, 0, 2, 0 });
		OutPlacements.Add({ EKind::Bed, 0, -1, 1, -1 });
		OutPlacements.Add({ EKind::Chest, 1, 1, 1, 1 });
	}
}
