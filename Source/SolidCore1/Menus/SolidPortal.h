#pragma once

#include "CoreMinimal.h"
#include "Lairs/SolidLair.h"
#include "Towns/SolidTowns.h"

/**
 * Places for the main menu's Use portal entry.
 * Towns come first, in SolidTowns order. The Iglin tunnels row follows them.
 * A town lands on one cell of that town, south of that cell's center
 * by the same plaza offset as the start-town spawn. A spread of cells can put
 * the centroid outside the town, so the landing uses a cell.
 * The tunnels row stands on the grass just north of the X mouth.
 */
namespace SolidPortal
{
	enum class EPlace : uint8
	{
		Town,
		TunnelMouth,
	};

	struct FChoice
	{
		EPlace Place = EPlace::Town;
		int32 TownId = INDEX_NONE;
		TCHAR TunnelEnd = 0;
		FString Name;
	};

	/** Rows after the towns. The Iglin tunnels mouth is the one extra place. */
	inline constexpr int32 ExtraPlaceCount = 1;

	/** Four meters north of the mouth's grass spot, still on that grass cell. */
	inline constexpr float NorthOfEntranceCm = 400.f;

	inline int32 TownCount()
	{
		return SolidTowns::DefinitionCount + ExtraPlaceCount;
	}

	inline void CollectChoices(TArray<FChoice>& OutChoices)
	{
		OutChoices.Reset();
		for (const SolidTowns::FTownInfo& Town : SolidTowns::Definitions)
		{
			FChoice Choice;
			Choice.Place = EPlace::Town;
			Choice.TownId = Town.Id;
			Choice.Name = Town.Name;
			OutChoices.Add(Choice);
		}

		FChoice Tunnels;
		Tunnels.Place = EPlace::TunnelMouth;
		Tunnels.TunnelEnd = USolidWorldMap::XMarker;
		Tunnels.Name = SolidLair::IglinTunnels.Name;
		OutChoices.Add(Tunnels);
	}

	inline bool FindChoice(const int32 Index, FChoice& OutChoice)
	{
		TArray<FChoice> Choices;
		CollectChoices(Choices);
		if (!Choices.IsValidIndex(Index))
		{
			return false;
		}
		OutChoice = Choices[Index];
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

	/** Grass just north of the mouth's standing spot. */
	inline FVector2D NorthOfEntrance(const FVector2D& MouthStandXY)
	{
		return FVector2D(MouthStandXY.X, MouthStandXY.Y + NorthOfEntranceCm);
	}

	/** The mouth faces the grass. The captain looks back at the stone. */
	inline float YawTowardMouth(const float MouthFacingYawDeg)
	{
		return FMath::UnwindDegrees(MouthFacingYawDeg + 180.f);
	}

	/** Planar shift that puts the captain on LandingXY. Companions keep their formation offset. */
	inline FVector SquadDelta(const FVector& CaptainLocation, const FVector2D& LandingXY)
	{
		return FVector(LandingXY.X - CaptainLocation.X, LandingXY.Y - CaptainLocation.Y, 0.f);
	}
}
