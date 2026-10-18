#pragma once

#include "Events/SolidEvent.h"
#include "Events/SolidMessages.h"
#include "Towns/SolidTowns.h"

/**
 * The shipped events.
 * Enter town: the captain steps onto a numbered WorldMap.txt cell
 * (or leaves one numbered town for another). The parameter is that digit.
 * Standing still, or walking between cells of the same town, does not re-fire.
 * Reach broken bridge: the captain stands on either ramp of the Iglin bridge.
 * The parameter is unused. The deck and the gap past the lip do not count.
 * That event shows a message. Closing that message is its own event.
 * Killed by monsters closes the monster message, opens the starting cave,
 * sends the captain inside, clears his inventory down to Rags, and enables Meagre.
 * Entering Iglin titles the captain Orphan. It does not enable raven sight.
 * Driven from the village fires on a later entry to Iglin, after that close,
 * shows "You are driven from the village", titles the captain Outcast,
 * and opens the Iglin tunnel mouths. It does not send him inside.
 * The portal to the grass north of the X mouth records that same event
 * without waiting for the close.
 * Nothing before Enter Relion enables raven sight. Relion is the first event that does.
 * A visit before the requirements are met does not fire. A later visit does.
 * Each event belongs to an act. Iglin, Reach broken bridge, and Killed by monsters
 * are ACT_0. Driven from the village is ACT_1.
 * Relion is ACT_2, Kanfold is ACT_3, and Dragongate is ACT_4.
 * Killed by monsters enables Meagre. Enter Relion enables Humble.
 * Enter Kanfold enables Average. Enter Dragongate still lists one
 * EnableStrongholdSize change for every size, including those three.
 * The duplicates stay until each size has only one event.
 */
namespace SolidEvents
{
	inline const FEvent Definitions[] = {
		EnterTown(IdEnterIglin, TEXT("Enter Iglin"), SolidTowns::Iglin, ACT_0)
			.SetPlayerTitle(TitleOrphan),
		ReachBrokenBridge(IdReachBrokenBridge, TEXT("Reach broken bridge"), ACT_0)
			.Require(IdEnterIglin)
			.DisplayMessage(SolidMessages::IdKilledByMonsters),
		OnMessageClosed(IdKilledByMonsters, TEXT("Killed by monsters"), SolidMessages::IdKilledByMonsters, ACT_0)
			.EnableCave()
			.ClearInventory()
			.EnableStrongholdSize(ESolidStrongholdSize::Meagre),
		EnterTown(IdDrivenFromVillage, TEXT("Driven from the village"), SolidTowns::Iglin, ACT_1)
			.Require(IdKilledByMonsters)
			.DisplayMessage(SolidMessages::IdDrivenFromVillage)
			.SetPlayerTitle(TitleOutcast),
		EnterTown(IdEnterRelion, TEXT("Enter Relion"), SolidTowns::Relion, ACT_2)
			.Require(IdEnterIglin)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(1)
			.SetPlayerTitle(TitleVagrant)
			.RepairBridge()
			.EnableStrongholdSize(ESolidStrongholdSize::Humble),
		EnterTown(IdEnterKanfold, TEXT("Enter Kanfold"), SolidTowns::Kanfold, ACT_3)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(2)
			.SetPlayerTitle(TitleRanger)
			.EnableStrongholdSize(ESolidStrongholdSize::Average),
		EnterTown(IdEnterDragongate, TEXT("Enter Dragongate"), SolidTowns::Dragongate, ACT_4)
			.EnableSight(ESolidSight::Raven)
			.SetMaxPartySize(RosterCount)
			.SetPlayerTitle(TitleCaptain)
			.EnableStrongholdSize(ESolidStrongholdSize::Meagre)
			.EnableStrongholdSize(ESolidStrongholdSize::Humble)
			.EnableStrongholdSize(ESolidStrongholdSize::Average)
			.EnableStrongholdSize(ESolidStrongholdSize::Impressive)
			.EnableStrongholdSize(ESolidStrongholdSize::Grand)
			.EnableStrongholdSize(ESolidStrongholdSize::Stately)
			.EnableStrongholdSize(ESolidStrongholdSize::Imperious),
	};

	static_assert(UE_ARRAY_COUNT(Definitions) <= MaxEvents, "Raise SolidEvents::MaxEvents");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

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
