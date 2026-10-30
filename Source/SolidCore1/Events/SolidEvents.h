#pragma once

#include "Events/SolidEvent.h"
#include "Events/SolidMessages.h"
#include "Lairs/SolidLairId.h"
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
			.SetPlayerTitle(ESolidCaptainTitle::Orphan),
		// Either ramp of the Iglin bridge. The deck and the gap past the lip do not count.
		ReachBrokenBridge(IdReachBrokenBridge, TEXT("Reach broken bridge"), ACT_0)
			.Require(IdEnterIglin),
		OnMessageClosed(IdKilledByMonsters, TEXT("Killed by monsters"), SolidMessages::IdKilledByMonsters, ACT_0)
			.EnableCave()
			.ClearInventory()
			.EnableStrongholdSize(ESolidStrongholdSize::Meagre)
			.DisableBattlePlan(TEXT("Starter"))
			.EnableBattlePlans(TEXT("Attack"), TEXT("Defend")),
		// It does not send the captain inside.
		// The portal to the grass north of the Iglin Tunnels Entrance records this event without waiting for the close.
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
