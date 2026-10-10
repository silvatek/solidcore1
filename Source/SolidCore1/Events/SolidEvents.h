#pragma once

#include "CoreMinimal.h"
#include "SolidSight.h"

/**
 * Events change the company/party configuration when a trigger fires.
 * Each event fires once, the first time its trigger becomes true.
 *
 * Enter town: the captain steps onto a numbered WorldMap.txt cell
 * (or leaves one numbered town for another). The parameter is that digit.
 * Standing still, or walking between cells of the same town, does not re-fire.
 */
namespace SolidEvents
{
	inline constexpr int32 MaxChangesPerEvent = 4;
	inline constexpr int32 MaxEvents = 16;

	enum class ETrigger : uint8
	{
		EnterTown,
	};

	enum class EChange : uint8
	{
		/** Unlock a sight and switch the captain to it. */
		EnableSight,
		/** How many companions may be in the party. */
		MaxPartySize,
	};

	struct FChange
	{
		EChange Kind = EChange::MaxPartySize;
		ESolidSight Sight = ESolidSight::Raven;
		int32 PartySize = 0;
	};

	struct FEvent
	{
		const TCHAR* Name = TEXT("");
		ETrigger Trigger = ETrigger::EnterTown;
		int32 Parameter = INDEX_NONE;
		FChange Changes[MaxChangesPerEvent] = {};
		int32 ChangeCount = 0;
	};

	/**
	 * Companions join in this order as Max Party Size grows.
	 * Size 1 is Sam. Size 2 is Sam and Alex.
	 */
	inline const TCHAR* RosterNames[] = {
		TEXT("Sam"),
		TEXT("Alex"),
	};

	inline constexpr int32 RosterCount = UE_ARRAY_COUNT(RosterNames);

	inline const FEvent Definitions[] = {
		{
			TEXT("Enter Iglin"),
			ETrigger::EnterTown,
			0,
		},
		{
			TEXT("Enter Relion"),
			ETrigger::EnterTown,
			1,
			{
				{ EChange::EnableSight, ESolidSight::Raven, 0 },
				{ EChange::MaxPartySize, ESolidSight::Raven, 1 },
			},
			2,
		},
		{
			TEXT("Enter Kanfold"),
			ETrigger::EnterTown,
			2,
			{
				{ EChange::MaxPartySize, ESolidSight::Raven, 2 },
			},
			1,
		},
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEvents, "Raise SolidEvents::MaxEvents");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	/** Live configuration. The game starts in true sight with an empty party. */
	struct FState
	{
		int32 MaxPartySize = 0;
		bool bTrueSightEnabled = true;
		bool bRavenSightEnabled = false;
		/** Numbered cell the captain is standing on, or INDEX_NONE. */
		int32 OccupiedTown = INDEX_NONE;
		bool bFired[MaxEvents] = {};
		/** Definition indexes, in the order the events fired. */
		int32 FiredOrder[MaxEvents] = {};
		int32 FiredCount = 0;
	};

	struct FResult
	{
		bool bFired = false;
		/** An Enable sight change ran. Sight is the mode to switch to. */
		bool bSetSight = false;
		ESolidSight Sight = ESolidSight::True;
		int32 MaxPartySize = 0;
	};

	inline const TCHAR* RosterName(const int32 Slot)
	{
		return (Slot >= 0 && Slot < RosterCount) ? RosterNames[Slot] : nullptr;
	}

	inline int32 PartyCount(const FState& State)
	{
		return FMath::Clamp(State.MaxPartySize, 0, RosterCount);
	}

	inline bool IsSightEnabled(const FState& State, const ESolidSight Sight)
	{
		switch (Sight)
		{
		case ESolidSight::True:
			return State.bTrueSightEnabled;
		case ESolidSight::Raven:
			return State.bRavenSightEnabled;
		default:
			return false;
		}
	}

	inline void ApplyChange(FState& State, const FChange& Change, FResult& Result)
	{
		switch (Change.Kind)
		{
		case EChange::EnableSight:
			if (Change.Sight == ESolidSight::Raven)
			{
				State.bRavenSightEnabled = true;
			}
			else
			{
				State.bTrueSightEnabled = true;
			}
			Result.bSetSight = true;
			Result.Sight = Change.Sight;
			break;
		case EChange::MaxPartySize:
			State.MaxPartySize = FMath::Max(0, Change.PartySize);
			Result.MaxPartySize = State.MaxPartySize;
			break;
		default:
			break;
		}
	}

	/**
	 * The captain is now on TownId (INDEX_NONE when the cell is not numbered).
	 * A change of town fires every unfired EnterTown event whose parameter matches.
	 */
	inline FResult NotifyOccupiedTown(FState& State, const int32 TownId)
	{
		FResult Result;
		Result.MaxPartySize = State.MaxPartySize;
		if (State.OccupiedTown == TownId)
		{
			return Result;
		}

		State.OccupiedTown = TownId;
		if (TownId == INDEX_NONE)
		{
			return Result;
		}

		for (int32 Index = 0; Index < DefinitionCount; ++Index)
		{
			if (State.bFired[Index])
			{
				continue;
			}

			const FEvent& Event = Definitions[Index];
			if (Event.Trigger != ETrigger::EnterTown || Event.Parameter != TownId)
			{
				continue;
			}

			State.bFired[Index] = true;
			if (State.FiredCount < MaxEvents)
			{
				State.FiredOrder[State.FiredCount] = Index;
				++State.FiredCount;
			}
			Result.bFired = true;
			const int32 ChangeCount = FMath::Min(Event.ChangeCount, MaxChangesPerEvent);
			for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
			{
				ApplyChange(State, Event.Changes[ChangeIndex], Result);
			}
		}

		Result.MaxPartySize = State.MaxPartySize;
		return Result;
	}
}

/** Alias so UCLASS headers can store the configuration without a nested type name. */
using FSolidEventConfiguration = SolidEvents::FState;
