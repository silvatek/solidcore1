#pragma once

#include "CoreMinimal.h"
#include "SolidGameState.h"
#include "SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"
#include "SolidTowerStronghold.h"

/**
 * Moving the company from the stronghold it has to a site.
 * A click offers a type only when that size is unlocked, larger, and already built.
 * Choosing a type commits. Esc never reaches this.
 * The swap itself is immediate, so it does not show a progress bar.
 */
namespace SolidStrongholdTransition
{
	/** A pair the game will one day offer. Meagre Fortress, Meagre Tower, and Humble Tower are outside it. */
	inline bool IsFinalPair(const FSolidStronghold& Stronghold)
	{
		if (Stronghold.Size == ESolidStrongholdSize::Meagre)
		{
			return Stronghold.Type == ESolidStrongholdType::Dungeon;
		}
		if (Stronghold.Size == ESolidStrongholdSize::Humble)
		{
			return Stronghold.Type == ESolidStrongholdType::Dungeon
				|| Stronghold.Type == ESolidStrongholdType::Fortress;
		}
		return true;
	}

	/** A final pair that already has an interior. Meagre Dungeon, and a tower from Average through Imperious. */
	inline bool IsBuilt(const FSolidStronghold& Stronghold)
	{
		if (!IsFinalPair(Stronghold))
		{
			return false;
		}
		if (Stronghold.Type == ESolidStrongholdType::Dungeon
			&& Stronghold.Size == ESolidStrongholdSize::Meagre)
		{
			return true;
		}
		SolidTowerStronghold::FPlan Plan;
		return SolidTowerStronghold::PlanFor(Stronghold, Plan);
	}

	enum class EOffer : uint8
	{
		None,
		Choose,
	};

	/**
	 * Types a click on a site of this size may list.
	 * None leaves the stone as it is and does not ask.
	 */
	inline EOffer Consider(
		const FGameState& State,
		const ESolidStrongholdSize SiteSize,
		TArray<ESolidStrongholdType>& OutTypes)
	{
		OutTypes.Reset();
		const int32 SiteIndex = static_cast<int32>(SiteSize);
		const int32 ActiveIndex = static_cast<int32>(State.ActiveStronghold.Size);
		if (SiteIndex <= ActiveIndex || !SolidEvents::IsStrongholdSizeUnlocked(State, SiteSize))
		{
			return EOffer::None;
		}

		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			const FSolidStronghold Pair{ Type, SiteSize };
			if (IsBuilt(Pair))
			{
				OutTypes.Add(Type);
			}
		}
		return OutTypes.Num() > 0 ? EOffer::Choose : EOffer::None;
	}

	inline bool AreNeighbors(const SolidStrongholdGrid::FHex& A, const SolidStrongholdGrid::FHex& B)
	{
		return FMath::IsNearlyEqual(
			FVector2D::Distance(A.Center, B.Center),
			SolidStrongholdGrid::FlatToFlatCm,
			15.f);
	}

	/**
	 * Put the carried pieces on kept cells of one floor.
	 * A bed takes two neighbors. The fireplace and the chest take one cell each.
	 */
	inline bool PlaceOnFloor(
		TArray<SolidStrongholdFurniture::FUsed>& Pieces,
		const TArray<SolidStrongholdGrid::FHex>& Hexes)
	{
		TArray<SolidStrongholdFurniture::FUsed> Next = Pieces;
		TArray<int32> Taken;
		for (SolidStrongholdFurniture::FUsed& Piece : Next)
		{
			const SolidStrongholdFurniture::FItem* Item = SolidStrongholdFurniture::ItemAt(Piece.ItemIndex);
			if (!Item)
			{
				return false;
			}

			if (SolidStrongholdFurniture::HexCount(Item->Kind) >= 2)
			{
				int32 Anchor = INDEX_NONE;
				int32 Span = INDEX_NONE;
				for (int32 Index = 0; Index < Hexes.Num() && Anchor == INDEX_NONE; ++Index)
				{
					if (Taken.Contains(Index))
					{
						continue;
					}
					for (int32 Other = 0; Other < Hexes.Num(); ++Other)
					{
						if (Other == Index || Taken.Contains(Other) || !AreNeighbors(Hexes[Index], Hexes[Other]))
						{
							continue;
						}
						Anchor = Index;
						Span = Other;
						break;
					}
				}
				if (Anchor == INDEX_NONE)
				{
					return false;
				}
				Taken.Add(Anchor);
				Taken.Add(Span);
				const FVector2D Delta = Hexes[Span].Center - Hexes[Anchor].Center;
				Piece.Level = 0;
				Piece.X = Hexes[Anchor].Col;
				Piece.Y = Hexes[Anchor].Row;
				Piece.RotationDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
			}
			else
			{
				int32 Anchor = INDEX_NONE;
				for (int32 Index = 0; Index < Hexes.Num(); ++Index)
				{
					if (!Taken.Contains(Index))
					{
						Anchor = Index;
						break;
					}
				}
				if (Anchor == INDEX_NONE)
				{
					return false;
				}
				Taken.Add(Anchor);
				Piece.Level = 0;
				Piece.X = Hexes[Anchor].Col;
				Piece.Y = Hexes[Anchor].Row;
				Piece.RotationDeg = 0.f;
			}
		}

		Pieces = Next;
		return true;
	}

	/**
	 * The site becomes the company base. Furnishings move onto its ground floor.
	 * The chest keeps its slots. The captain's pack stays with him.
	 * Returns false when the click would not have offered that type.
	 */
	inline bool Commit(
		FGameState& State,
		const FIntPoint SiteCell,
		const ESolidStrongholdSize SiteSize,
		const ESolidStrongholdType Type)
	{
		TArray<ESolidStrongholdType> Types;
		if (Consider(State, SiteSize, Types) != EOffer::Choose || !Types.Contains(Type))
		{
			return false;
		}

		const FSolidStronghold Next{ Type, SiteSize };
		SolidTowerStronghold::FPlan Plan;
		if (!SolidTowerStronghold::PlanFor(Next, Plan))
		{
			return false;
		}

		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidTowerStronghold::CollectKeptHexes(Plan, 0, Hexes);
		TArray<SolidStrongholdFurniture::FUsed> Moved = State.Furnishings;
		if (Moved.Num() == 0)
		{
			Moved = SolidStrongholdFurniture::StartingFurnishings();
		}
		if (!PlaceOnFloor(Moved, Hexes))
		{
			return false;
		}

		FLeftStronghold Left;
		Left.Stronghold = State.ActiveStronghold;
		Left.Cell = State.ActiveStrongholdCell;
		State.LeftStrongholds.Add(Left);
		State.ActiveStronghold = Next;
		State.ActiveStrongholdCell = SiteCell;
		State.Furnishings = Moved;
		return true;
	}

	/** F10 Enter cave and the stronghold mouth. True only while that stronghold is still the company base and open. */
	inline bool CompanyCaveAcceptsEnter(const FGameState& State)
	{
		return State.bCaveEnabled
			&& State.ActiveStronghold.Type == ESolidStrongholdType::Dungeon
			&& State.ActiveStronghold.Size == ESolidStrongholdSize::Meagre
			&& State.ActiveStrongholdCell == SolidStronghold::StartingCell();
	}

	/** A tower door the company has left is not a passage. Any other tower door stays one. */
	inline bool TowerDoorAcceptsEnter(const FGameState& State, const FIntPoint Cell)
	{
		for (const FLeftStronghold& Left : State.LeftStrongholds)
		{
			if (Left.Cell == Cell && Left.Stronghold.Type == ESolidStrongholdType::Tower)
			{
				return false;
			}
		}
		return true;
	}

	/** The company chest is open on the inventory page while the captain is inside the base that holds it. */
	inline bool CompanyChestOpen(
		const FGameState& State,
		const bool bInDungeon,
		const bool bInTower,
		const FIntPoint TowerCell)
	{
		bool bHasChest = false;
		for (const SolidStrongholdFurniture::FUsed& Piece : State.Furnishings)
		{
			if (SolidStrongholdFurniture::IsStorage(SolidStrongholdFurniture::ItemAt(Piece.ItemIndex)))
			{
				bHasChest = true;
				break;
			}
		}
		if (!bHasChest)
		{
			return false;
		}
		if (bInDungeon)
		{
			return CompanyCaveAcceptsEnter(State);
		}
		if (bInTower)
		{
			return State.ActiveStronghold.Type == ESolidStrongholdType::Tower
				&& State.ActiveStrongholdCell == TowerCell;
		}
		return false;
	}
}
