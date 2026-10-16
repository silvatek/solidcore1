#pragma once

#include "CoreMinimal.h"
#include "SolidSight.h"

/**
 * Events change the company/party configuration when a trigger fires.
 * Each event fires once, the first time its trigger becomes true,
 * and only after every event it requires has already fired.
 *
 * Enter town: the captain steps onto a numbered WorldMap.txt cell
 * (or leaves one numbered town for another). The parameter is that digit.
 * Standing still, or walking between cells of the same town, does not re-fire.
 * Reach broken bridge: the captain stands on either ramp of the Iglin bridge.
 * The parameter is unused. The deck and the gap past the lip do not count.
 * A visit before the requirements are met does not fire. A later visit does.
 * Each event belongs to an act. Iglin and Reach broken bridge are act 0.
 * Relion is act 2, Kanfold is act 3, and Dragongate is act 4.
 */
namespace SolidEvents
{
	inline constexpr int32 MaxChangesPerEvent = 4;
	inline constexpr int32 MaxRequiresPerEvent = 4;
	inline constexpr int32 MaxEvents = 16;
	inline constexpr int32 EventIdLength = 8;

	/** Stable ids. Eight hex digits. Requirements name these. */
	inline constexpr const TCHAR* IdEnterIglin = TEXT("c4e1a7b2");
	inline constexpr const TCHAR* IdReachBrokenBridge = TEXT("a17c4e58");
	inline constexpr const TCHAR* IdEnterRelion = TEXT("9f30d86a");
	inline constexpr const TCHAR* IdEnterKanfold = TEXT("2b8e15c7");
	inline constexpr const TCHAR* IdEnterDragongate = TEXT("e6a4f093");

	enum class ETrigger : uint8
	{
		EnterTown,
		/** The captain stands on a ramp of the Iglin bridge. */
		ReachBrokenBridge,
	};

	enum class EChange : uint8
	{
		/** Unlock a sight and switch the captain to it. */
		EnableSight,
		/** How many companions may be in the party. */
		MaxPartySize,
		/** The captain's nameplate. Outcast, then Vagrant, then Ranger, then Captain. */
		CaptainTitle,
		/** Draw the center of a broken bridge and open that crossing. */
		RepairBridge,
	};

	/** Nameplate before any title event. The sequence ends at Captain. */
	inline constexpr const TCHAR* TitleOutcast = TEXT("Outcast");
	inline constexpr const TCHAR* TitleVagrant = TEXT("Vagrant");
	inline constexpr const TCHAR* TitleRanger = TEXT("Ranger");
	inline constexpr const TCHAR* TitleCaptain = TEXT("Captain");

	struct FChange
	{
		EChange Kind = EChange::MaxPartySize;
		ESolidSight Sight = ESolidSight::Raven;
		int32 PartySize = 0;
		const TCHAR* Title = nullptr;
	};

	struct FEvent
	{
		/** Eight hex digits. */
		const TCHAR* Id = TEXT("");
		const TCHAR* Name = TEXT("");
		ETrigger Trigger = ETrigger::EnterTown;
		int32 Parameter = INDEX_NONE;
		FChange Changes[MaxChangesPerEvent] = {};
		int32 ChangeCount = 0;
		/** Event ids that must already have fired. Empty means no requirement. */
		const TCHAR* Requires[MaxRequiresPerEvent] = {};
		int32 RequireCount = 0;
		/** Story act this event belongs to. */
		int32 Act = 0;
	};

	/**
	 * Companions join in this order as Max Party Size grows.
	 * Size 1 is Sam. Size 2 is Sam and Alex. Size 5 is the whole company.
	 */
	inline const TCHAR* RosterNames[] = {
		TEXT("Sam"),
		TEXT("Alex"),
		TEXT("Max"),
		TEXT("Morgan"),
		TEXT("Quinn"),
	};

	inline constexpr int32 RosterCount = UE_ARRAY_COUNT(RosterNames);

	inline const FEvent Definitions[] = {
		{
			IdEnterIglin,
			TEXT("Enter Iglin"),
			ETrigger::EnterTown,
			0,
			{},
			0,
			{},
			0,
			0,
		},
		{
			IdReachBrokenBridge,
			TEXT("Reach broken bridge"),
			ETrigger::ReachBrokenBridge,
			INDEX_NONE,
			{},
			0,
			{ IdEnterIglin },
			1,
			0,
		},
		{
			IdEnterRelion,
			TEXT("Enter Relion"),
			ETrigger::EnterTown,
			1,
			{
				{ EChange::EnableSight, ESolidSight::Raven, 0 },
				{ EChange::MaxPartySize, ESolidSight::Raven, 1 },
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleVagrant },
				{ EChange::RepairBridge },
			},
			4,
			{ IdEnterIglin },
			1,
			2,
		},
		{
			IdEnterKanfold,
			TEXT("Enter Kanfold"),
			ETrigger::EnterTown,
			2,
			{
				{ EChange::EnableSight, ESolidSight::Raven, 0 },
				{ EChange::MaxPartySize, ESolidSight::Raven, 2 },
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleRanger },
			},
			3,
			{},
			0,
			3,
		},
		{
			IdEnterDragongate,
			TEXT("Enter Dragongate"),
			ETrigger::EnterTown,
			5,
			{
				{ EChange::EnableSight, ESolidSight::Raven, 0 },
				{ EChange::MaxPartySize, ESolidSight::Raven, RosterCount },
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleCaptain },
			},
			3,
			{},
			0,
			4,
		},
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEvents, "Raise SolidEvents::MaxEvents");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const TCHAR* RosterName(const int32 Slot)
	{
		return (Slot >= 0 && Slot < RosterCount) ? RosterNames[Slot] : nullptr;
	}

	inline int32 FindIndexById(const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < DefinitionCount; ++Index)
		{
			if (FCString::Strcmp(Definitions[Index].Id, Id) == 0)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}
