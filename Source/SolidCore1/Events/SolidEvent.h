#pragma once

#include "CoreMinimal.h"
#include "SolidSight.h"
#include "SolidStronghold.h"
#include "Company/SolidCaptain.h"

/**
 * One event: a trigger, the changes it applies, and the events that must already have fired.
 * Each event fires once, the first time its trigger becomes true,
 * and only after every event it requires has already fired.
 * EnterTown, ReachBrokenBridge, and OnMessageClosed build one.
 * A change helper fills the one value that change reads. The builder adds it.
 */
namespace SolidEvents
{
	/**
	 * Sight, squad size, and title, one unlock for every stronghold size,
	 * and the seven battle-plan edits on Enter Dragongate.
	 */
	inline constexpr int32 MaxChangesPerEvent = 3 + SolidStronghold::SizeCount + 7;
	static_assert(MaxChangesPerEvent == 17, "Enter Dragongate needs one slot per change");
	/** Lair ids an EnableLair change can record. One lair ships. */
	inline constexpr int32 MaxEnabledLairs = 4;
	inline constexpr int32 MaxRequiresPerEvent = 4;
	inline constexpr int32 MaxEvents = 16;

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
		/** How many companions may be in the squad. */
		MaxSquadSize,
		/** The captain's nameplate. Orphan, then Outcast, then Vagrant, then Ranger, then Captain. */
		CaptainTitle,
		/** Draw the center of a broken bridge and open that crossing. */
		RepairBridge,
		/** Open strongholds and send the captain into the active one. */
		EnableCave,
		/** Show the message named by MessageId. */
		ShowMessage,
		/** Empty the captain's worn boxes and pack, then put Rags on the body. */
		ClearInventory,
		/** Enable one stronghold size. Strongholds open. */
		EnableStrongholdSize,
		/** Open the lair named by LairId. */
		EnableLair,
		/** Turn one catalog plan on. The name is BattlePlanName. */
		EnableBattlePlan,
		/** Turn one catalog plan off. The name is BattlePlanName. */
		DisableBattlePlan,
	};

	struct FChange
	{
		EChange Kind = EChange::MaxSquadSize;
		ESolidSight Sight = ESolidSight::Raven;
		int32 SquadSize = 0;
		ESolidCaptainTitle Title = ESolidCaptainTitle::Outcast;
		/** Message id for a ShowMessage change. */
		const TCHAR* MessageId = nullptr;
		/** Size for an EnableStrongholdSize change. */
		ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;
		/** Lair id for an EnableLair change. Eight hex digits. */
		const TCHAR* LairId = nullptr;
		/** Plan name for an EnableBattlePlan or DisableBattlePlan change. */
		const TCHAR* BattlePlanName = nullptr;
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

		inline constexpr FChange SetMaxSquadSize(const int32 Size)
		{
			FChange Change;
			Change.Kind = EChange::MaxSquadSize;
			Change.SquadSize = Size;
			return Change;
		}

		inline constexpr FChange SetPlayerTitle(const ESolidCaptainTitle Name)
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

		inline constexpr FChange ClearInventory()
		{
			FChange Change;
			Change.Kind = EChange::ClearInventory;
			return Change;
		}

		inline constexpr FChange EnableStrongholdSize(const ESolidStrongholdSize Size)
		{
			FChange Change;
			Change.Kind = EChange::EnableStrongholdSize;
			Change.StrongholdSize = Size;
			return Change;
		}

		inline constexpr FChange EnableLair(const TCHAR* Id)
		{
			FChange Change;
			Change.Kind = EChange::EnableLair;
			Change.LairId = Id;
			return Change;
		}

		inline constexpr FChange EnableBattlePlan(const TCHAR* Name)
		{
			FChange Change;
			Change.Kind = EChange::EnableBattlePlan;
			Change.BattlePlanName = Name;
			return Change;
		}

		inline constexpr FChange DisableBattlePlan(const TCHAR* Name)
		{
			FChange Change;
			Change.Kind = EChange::DisableBattlePlan;
			Change.BattlePlanName = Name;
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

		constexpr FEvent SetMaxSquadSize(const int32 Size) const
		{
			return With(Change::SetMaxSquadSize(Size));
		}

		constexpr FEvent SetPlayerTitle(const ESolidCaptainTitle InName) const
		{
			return With(Change::SetPlayerTitle(InName));
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

		constexpr FEvent ClearInventory() const
		{
			return With(Change::ClearInventory());
		}

		constexpr FEvent EnableStrongholdSize(const ESolidStrongholdSize Size) const
		{
			return With(Change::EnableStrongholdSize(Size));
		}

		constexpr FEvent EnableLair(const TCHAR* InLairId) const
		{
			return With(Change::EnableLair(InLairId));
		}

		constexpr FEvent EnableBattlePlan(const TCHAR* PlanName) const
		{
			return With(Change::EnableBattlePlan(PlanName));
		}

		constexpr FEvent DisableBattlePlan(const TCHAR* PlanName) const
		{
			return With(Change::DisableBattlePlan(PlanName));
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
	 * Companions join in this order as Max Squad Size grows.
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

	inline const TCHAR* RosterName(const int32 Slot)
	{
		return (Slot >= 0 && Slot < RosterCount) ? RosterNames[Slot] : nullptr;
	}
}
