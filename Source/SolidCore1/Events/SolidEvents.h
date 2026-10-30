#pragma once

#include "Events/SolidEvent.h"
#include "Events/SolidMessages.h"
#include "Lairs/SolidLairId.h"
#include "Towns/SolidTowns.h"

/**
 * The shipped story.
 * Each StoryEvent fires once. After it has fired, a later trigger for that same event does nothing.
 * An enter-town event fires when the captain steps onto that town, or leaves one town for another.
 * Standing still, or walking within the same town, does not fire it again.
 */
namespace SolidEvents
{
	inline const FEvent StoryEvents[] = {
		EnterTown(IdEnterIglin, TEXT("Enter Iglin"), SolidTowns::Iglin, ACT_0)
			.SetPlayerTitle(ESolidCaptainTitle::Orphan),
		OnCaptainDeath(IdKilledByMonsters, TEXT("Killed by monsters"), ACT_0)
			.EnableCave()
			.ClearInventory()
			.EnableStrongholdSize(ESolidStrongholdSize::Meagre)
			.DisableBattlePlan(TEXT("Starter"))
			.EnableBattlePlans(TEXT("Attack"), TEXT("Defend")),
		// The return to Iglin. The death already sent the captain inside.
		EnterTown(IdDrivenFromVillage, TEXT("Driven from the village"), SolidTowns::Iglin, ACT_1)
			.Require(IdKilledByMonsters)
			.DisplayMessage(SolidMessages::IdDrivenFromVillage)
			.SetPlayerTitle(ESolidCaptainTitle::Outcast)
			.EnableLair(SolidLair::IdIglinTunnels),
		EnterTown(IdEnterRelion, TEXT("Enter Relion"), SolidTowns::Relion, ACT_2)
			.Require(IdEnterIglin)
			.EnableSight(ESolidSight::Raven)
			.SetMaxSquadSize(1)
			.AddCompanion(TEXT("Sam"))
			.SetPlayerTitle(ESolidCaptainTitle::Vagrant)
			.RepairBridge()
			.EnableStrongholdSize(ESolidStrongholdSize::Humble)
			.EnableBattlePlan(TEXT("Parade")),
		EnterTown(IdEnterKanfold, TEXT("Enter Kanfold"), SolidTowns::Kanfold, ACT_3)
			.EnableSight(ESolidSight::Raven)
			.SetMaxSquadSize(2)
			.AddCompanion(TEXT("Alex"))
			.SetPlayerTitle(ESolidCaptainTitle::Ranger)
			.EnableStrongholdSize(ESolidStrongholdSize::Average)
			.DisableBattlePlans(TEXT("Starter"), TEXT("Attack"), TEXT("Defend"))
			.EnableBattlePlans(TEXT("Line"), TEXT("Column"), TEXT("Tight mob"), TEXT("Loose mob")),
		EnterTown(IdEnterDragongate, TEXT("Enter Dragongate"), SolidTowns::Dragongate, ACT_4)
			.EnableSights(ESolidSight::Falcon, ESolidSight::Eagle, ESolidSight::Raven)
			.AddCompanions(TEXT("Max"), TEXT("Morgan"), TEXT("Quinn"))
			.SetMaxSquadSize(RosterCount)
			.SetPlayerTitle(ESolidCaptainTitle::Captain)
			.EnableAllStrongholds()
			.DisableBattlePlans(TEXT("Starter"), TEXT("Attack"), TEXT("Defend"))
			.EnableBattlePlans(TEXT("Line"), TEXT("Column"), TEXT("Tight mob"), TEXT("Loose mob")),
		// No earlier event is required, so a portal here can try the sights.
		// Eagle is last, so the view switches to Eagle.
		EnterTown(IdEnterJethan, TEXT("Enter Jethan"), SolidTowns::Jethan, ACT_4)
			.EnableSights(ESolidSight::Raven, ESolidSight::Falcon, ESolidSight::Eagle)
			.ClearAllFog(),
	};

	static_assert(UE_ARRAY_COUNT(StoryEvents) <= MaxEvents, "Raise SolidEvents::MaxEvents");

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(StoryEvents);

	inline int32 FindIndexById(const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < DefinitionCount; ++Index)
		{
			if (FCString::Strcmp(StoryEvents[Index].Id, Id) == 0)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}
