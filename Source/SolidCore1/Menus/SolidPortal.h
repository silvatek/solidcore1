#pragma once

#include "CoreMinimal.h"
#include "Towns/SolidTowns.h"

/**
 * Town list for the main menu's Use portal entry.
 * The party lands on one cell of the chosen town, south of that cell's center
 * by the same plaza offset as the start-town spawn. A spread of cells can put
 * the centroid outside the town, so the landing uses a cell.
 */
namespace SolidPortal
{
	struct FChoice
	{
		int32 TownId = INDEX_NONE;
		FString Name;
	};

	inline int32 TownCount()
	{
		return SolidTowns::DefinitionCount;
	}

	inline void CollectChoices(TArray<FChoice>& OutChoices)
	{
		OutChoices.Reset();
		for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
		{
			FChoice Choice;
			Choice.TownId = Town.Id;
			Choice.Name = Town.Name;
			OutChoices.Add(Choice);
		}
	}

	inline bool FindChoice(const int32 Index, FChoice& OutChoice)
	{
		if (Index < 0 || Index >= SolidTowns::DefinitionCount)
		{
			return false;
		}
		const SolidTowns::FTownInfo& Town = SolidTowns::Definitions[Index];
		OutChoice.TownId = Town.Id;
		OutChoice.Name = Town.Name;
		return true;
	}

	inline int32 WrapIndex(const int32 Index, const int32 Delta)
	{
		const int32 Count = TownCount();
		if (Count <= 0)
		{
			return 0;
		}

		int32 Wrapped = (Index + Delta) % Count;
		if (Wrapped < 0)
		{
			Wrapped += Count;
		}
		return Wrapped;
	}

	/**
	 * Northernmost cell (smallest row; row 0 is north). Ties take the easternmost
	 * (smallest column; column 0 is east).
	 */
	inline bool ChooseLandingCell(const TArray<FIntPoint>& Cells, FIntPoint& OutCell)
	{
		if (Cells.Num() == 0)
		{
			return false;
		}

		OutCell = Cells[0];
		for (const FIntPoint& Cell : Cells)
		{
			if (Cell.Y < OutCell.Y || (Cell.Y == OutCell.Y && Cell.X < OutCell.X))
			{
				OutCell = Cell;
			}
		}
		return true;
	}

	/** Cell center plus the plaza offset. The default offset matches the start-town spawn. */
	inline FVector2D LandingXY(const FVector2D& CellCenter, const FVector2D& PlazaOffset)
	{
		return CellCenter + PlazaOffset;
	}

	/** Planar shift that puts the captain on LandingXY. Companions keep their formation offset. */
	inline FVector PartyDelta(const FVector& CaptainLocation, const FVector2D& LandingXY)
	{
		return FVector(LandingXY.X - CaptainLocation.X, LandingXY.Y - CaptainLocation.Y, 0.f);
	}
}
