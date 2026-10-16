#pragma once

#include "CoreMinimal.h"
#include "Events/SolidMessages.h"
#include "SolidSight.h"
#include "Towns/SolidTowns.h"

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
 * That event shows a message. Closing that message is its own event.
 * Killed by monsters closes the monster message, opens the starting cave,
 * and sends the captain inside.
 * Entering Iglin titles the captain Orphan. It does not enable raven sight.
 * Driven from the village fires on a later entry to Iglin, after that close,
 * shows "You are driven from the village", and titles the captain Outcast.
 * Nothing before Enter Relion enables raven sight. Relion is the first event that does.
 * A visit before the requirements are met does not fire. A later visit does.
 * Each event belongs to an act. Iglin, Reach broken bridge, and Killed by monsters
 * are ACT_0. Driven from the village is ACT_1.
 * Relion is ACT_2, Kanfold is ACT_3, and Dragongate is ACT_4.
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
	inline constexpr const TCHAR* IdKilledByMonsters = TEXT("e51b8c36");
	inline constexpr const TCHAR* IdDrivenFromVillage = TEXT("8f3c2a17");
	inline constexpr const TCHAR* IdEnterRelion = TEXT("9f30d86a");
	inline constexpr const TCHAR* IdEnterKanfold = TEXT("2b8e15c7");
	inline constexpr const TCHAR* IdEnterDragongate = TEXT("e6a4f093");

	/** Story acts. Event definitions name these. */
	inline constexpr int32 ACT_0 = 0;
	inline constexpr int32 ACT_1 = 1;
	inline constexpr int32 ACT_2 = 2;
	inline constexpr int32 ACT_3 = 3;
	inline constexpr int32 ACT_4 = 4;

	enum class ETrigger : uint8
	{
		EnterTown,
		/** The captain stands on a ramp of the Iglin bridge. */
		ReachBrokenBridge,
		/** The captain closes the message named by MessageId. */
		MessageClosed,
	};

	enum class EChange : uint8
	{
		/** Unlock a sight and switch the captain to it. */
		EnableSight,
		/** How many companions may be in the party. */
		MaxPartySize,
		/** The captain's nameplate. Orphan, then Outcast, then Vagrant, then Ranger, then Captain. */
		CaptainTitle,
		/** Draw the center of a broken bridge and open that crossing. */
		RepairBridge,
		/** Open the starting cave and send the captain inside. */
		EnableCave,
		/** Show the message named by MessageId. */
		ShowMessage,
	};

	/** Nameplate before Enter Iglin. Driven from the village sets this again. */
	inline constexpr const TCHAR* TitleOutcast = TEXT("Outcast");
	/** Enter Iglin. */
	inline constexpr const TCHAR* TitleOrphan = TEXT("Orphan");
	inline constexpr const TCHAR* TitleVagrant = TEXT("Vagrant");
	inline constexpr const TCHAR* TitleRanger = TEXT("Ranger");
	inline constexpr const TCHAR* TitleCaptain = TEXT("Captain");

	struct FChange
	{
		EChange Kind = EChange::MaxPartySize;
		ESolidSight Sight = ESolidSight::Raven;
		int32 PartySize = 0;
		const TCHAR* Title = nullptr;
		/** Message id for a ShowMessage change. */
		const TCHAR* MessageId = nullptr;
	};

	/**
	 * Each helper fills the one value that change reads. The other fields stay unused.
	 * The event builder adds that change to an event.
	 */
	namespace Change
	{
		inline constexpr FChange EnableSight(const ESolidSight Mode)
		{
			FChange Change;
			Change.Kind = EChange::EnableSight;
			Change.Sight = Mode;
			return Change;
		}

		inline constexpr FChange SetMaxPartySize(const int32 Size)
		{
			FChange Change;
			Change.Kind = EChange::MaxPartySize;
			Change.PartySize = Size;
			return Change;
		}

		inline constexpr FChange SetPlayerTitle(const TCHAR* Name)
		{
			FChange Change;
			Change.Kind = EChange::CaptainTitle;
			Change.Title = Name;
			return Change;
		}

		inline constexpr FChange DisplayMessage(const TCHAR* MessageId)
		{
			FChange Change;
			Change.Kind = EChange::ShowMessage;
			Change.MessageId = MessageId;
			return Change;
		}

		inline constexpr FChange EnableCave()
		{
			FChange Change;
			Change.Kind = EChange::EnableCave;
			return Change;
		}

		inline constexpr FChange RepairBridge()
		{
			FChange Change;
			Change.Kind = EChange::RepairBridge;
			return Change;
		}
	}

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
		int32 Act = ACT_0;
		/** Message id for a MessageClosed trigger. Empty for every other trigger. */
		const TCHAR* MessageId = nullptr;

		constexpr FEvent EnableSight(const ESolidSight Mode) const
		{
			return With(Change::EnableSight(Mode));
		}

		constexpr FEvent SetMaxPartySize(const int32 Size) const
		{
			return With(Change::SetMaxPartySize(Size));
		}

		constexpr FEvent SetPlayerTitle(const TCHAR* Name) const
		{
			return With(Change::SetPlayerTitle(Name));
		}

		constexpr FEvent DisplayMessage(const TCHAR* InMessageId) const
		{
			return With(Change::DisplayMessage(InMessageId));
		}

		constexpr FEvent EnableCave() const
		{
			return With(Change::EnableCave());
		}

		constexpr FEvent RepairBridge() const
		{
			return With(Change::RepairBridge());
		}

		constexpr FEvent Require(const TCHAR* EventId) const
		{
			FEvent Event = *this;
			if (Event.RequireCount < MaxRequiresPerEvent)
			{
				Event.Requires[Event.RequireCount] = EventId;
				++Event.RequireCount;
			}
			return Event;
		}

	private:
		constexpr FEvent With(const FChange& Change) const
		{
			FEvent Event = *this;
			if (Event.ChangeCount < MaxChangesPerEvent)
			{
				Event.Changes[Event.ChangeCount] = Change;
				++Event.ChangeCount;
			}
			return Event;
		}
	};

	inline constexpr FEvent EnterTown(const TCHAR* Id, const TCHAR* Name, const int32 TownId, const int32 Act)
	{
		FEvent Event;
		Event.Id = Id;
		Event.Name = Name;
		Event.Trigger = ETrigger::EnterTown;
		Event.Parameter = TownId;
		Event.Act = Act;
		return Event;
	}

	inline constexpr FEvent ReachBrokenBridge(const TCHAR* Id, const TCHAR* Name, const int32 Act)
	{
		FEvent Event;
		Event.Id = Id;
		Event.Name = Name;
		Event.Trigger = ETrigger::ReachBrokenBridge;
		Event.Parameter = INDEX_NONE;
		Event.Act = Act;
		return Event;
	}

	inline constexpr FEvent OnMessageClosed(
		const TCHAR* Id,
		const TCHAR* Name,
		const TCHAR* MessageId,
		const int32 Act)
	{
		FEvent Event;
		Event.Id = Id;
		Event.Name = Name;
		Event.Trigger = ETrigger::MessageClosed;
		Event.Parameter = INDEX_NONE;
		Event.MessageId = MessageId;
		Event.Act = Act;
		return Event;
	}

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
		EnterTown(IdEnterIglin, TEXT("Enter Iglin"), SolidTowns::Iglin, ACT_0)
			.SetPlayerTitle(TitleOrphan),
		ReachBrokenBridge(IdReachBrokenBridge, TEXT("Reach broken bridge"), ACT_0)
			.DisplayMessage(SolidMessages::IdKilledByMonsters)
			.Require(IdEnterIglin),
		OnMessageClosed(IdKilledByMonsters, TEXT("Killed by monsters"), SolidMessages::IdKilledByMonsters, ACT_0)
			.EnableCave(),
		EnterTown(IdDrivenFromVillage, TEXT("Driven from the village"), SolidTowns::Iglin, ACT_1)
			.DisplayMessage(SolidMessages::IdDrivenFromVillage)
			.SetPlayerTitle(TitleOutcast)
			.Require(IdKilledByMonsters),
		EnterTown(IdEnterRelion, TEXT("Enter Relion"), SolidTowns::Relion, ACT_2)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(1)
			.SetPlayerTitle(TitleVagrant)
			.RepairBridge()
			.Require(IdEnterIglin),
		EnterTown(IdEnterKanfold, TEXT("Enter Kanfold"), SolidTowns::Kanfold, ACT_3)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(2)
			.SetPlayerTitle(TitleRanger),
		EnterTown(IdEnterDragongate, TEXT("Enter Dragongate"), SolidTowns::Dragongate, ACT_4)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(RosterCount)
			.SetPlayerTitle(TitleCaptain),
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
