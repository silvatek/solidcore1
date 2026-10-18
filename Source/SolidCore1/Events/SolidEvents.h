#pragma once

#include "Events/SolidEvent.h"
#include "Events/SolidMessages.h"
#include "Towns/SolidTowns.h"

/**
 * The shipped events.
 * An enter-town entry fires when the captain steps onto that numbered WorldMap.txt cell,
 * or leaves one numbered town for another. The parameter is that digit.
 * Standing still, or walking between cells of the same town, does not re-fire.
 */
namespace SolidEvents
{
	inline const FEvent Definitions[] = {
		EnterTown(IdEnterIglin, TEXT("Enter Iglin"), SolidTowns::Iglin, ACT_0)
			.SetPlayerTitle(TitleOrphan),
		// Either ramp of the Iglin bridge. The deck and the gap past the lip do not count.
		ReachBrokenBridge(IdReachBrokenBridge, TEXT("Reach broken bridge"), ACT_0)
			.Require(IdEnterIglin)
			.DisplayMessage(SolidMessages::IdKilledByMonsters),
		OnMessageClosed(IdKilledByMonsters, TEXT("Killed by monsters"), SolidMessages::IdKilledByMonsters, ACT_0)
			.EnableCave()
			.ClearInventory()
			.EnableStrongholdSize(ESolidStrongholdSize::Meagre),
		// Opens the Iglin tunnel mouths. It does not send the captain inside.
		// The portal to the grass north of the X mouth records this event without waiting for the close.
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
