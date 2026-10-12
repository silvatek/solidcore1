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
		/** The captain's nameplate. Outcast, then Vagrant, then Ranger, then Captain. */
		CaptainTitle,
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
		const TCHAR* Name = TEXT("");
		ETrigger Trigger = ETrigger::EnterTown;
		int32 Parameter = INDEX_NONE;
		FChange Changes[MaxChangesPerEvent] = {};
		int32 ChangeCount = 0;
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
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleVagrant },
			},
			3,
		},
		{
			TEXT("Enter Kanfold"),
			ETrigger::EnterTown,
			2,
			{
				{ EChange::MaxPartySize, ESolidSight::Raven, 2 },
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleRanger },
			},
			2,
		},
		{
			TEXT("Enter Dragongate"),
			ETrigger::EnterTown,
			5,
			{
				{ EChange::MaxPartySize, ESolidSight::Raven, RosterCount },
				{ EChange::CaptainTitle, ESolidSight::Raven, 0, TitleCaptain },
			},
			2,
		},
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEvents, "Raise SolidEvents::MaxEvents");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const TCHAR* RosterName(const int32 Slot)
	{
		return (Slot >= 0 && Slot < RosterCount) ? RosterNames[Slot] : nullptr;
	}
}
