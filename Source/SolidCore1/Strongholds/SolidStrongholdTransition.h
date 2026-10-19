#pragma once

#include "CoreMinimal.h"
#include "SolidGameState.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidStrongholdGrid.h"

/**
 * Moving the company from the stronghold it has to a site.
 * A click offers a type only when that size is unlocked, larger, and the subclass can build it.
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
		return IsFinalPair(Stronghold)
			&& SolidStronghold::TypeOf(Stronghold.Type).Supported(Stronghold.Size);
	}

	enum class EOffer : uint8
	{
		None,
		Choose,
	};

	/**
	 * Types a click on a site of this size may list.
	 * None leaves the site as it is and does not ask.
	 */
	inline EOffer Consider(
		const FGameState& State,
		const ESolidStrongholdSize SiteSize,
		TArray<ESolidStrongholdType>& OutTypes)
	{
		OutTypes.Reset();
		const int32 SiteIndex = static_cast<int32>(SiteSize);
		const int32 ActiveIndex = static_cast<int32>(State.ActiveStronghold->Size);
		if (SiteIndex <= ActiveIndex || !SolidEvents::IsStrongholdSizeUnlocked(State, SiteSize))
		{
			return EOffer::None;
		}

		for (const ESolidStrongholdType Type : SolidStronghold::Types)
		{
			if (SolidStronghold::TypeOf(Type).Supported(SiteSize))
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

		TArray<SolidStrongholdGrid::FHex> Hexes;
		SolidStronghold::TypeOf(Type).CollectGroundHexes(SiteSize, Hexes);
		if (Hexes.Num() == 0)
		{
			return false;
		}
		TArray<SolidStrongholdFurniture::FUsed> Moved = State.Furnishings;
		if (Moved.Num() == 0)
		{
			Moved = SolidStrongholdFurniture::StartingFurnishings();
		}
		if (!PlaceOnFloor(Moved, Hexes))
		{
			return false;
		}

		State.PreviousStrongholds.Add(State.ActiveStronghold->Clone());
		State.ActiveStronghold = SolidStronghold::Make(
			Type,
			SiteSize,
			SiteCell,
			SolidStronghold::WorldXY(SiteCell));
		State.Furnishings = Moved;
		return true;
	}

	/** Strongholds stay shut until the monster message closes. */
	inline bool StrongholdsEnabled(const FGameState& State)
	{
		return State.bStrongholdsEnabled;
	}

	/** The company chest is open on the inventory page while the captain is inside the base that holds it. */
	inline bool CompanyChestOpen(const FGameState& State, const bool bInStronghold)
	{
		if (!bInStronghold || !State.ActiveStronghold)
		{
			return false;
		}
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
		if (State.ActiveStronghold->Type == ESolidStrongholdType::Dungeon)
		{
			return State.ActiveStronghold->AcceptsEnter(State, State.ActiveStronghold->Cell);
		}
		return State.ActiveStronghold->Type == ESolidStrongholdType::Tower;
	}
}
