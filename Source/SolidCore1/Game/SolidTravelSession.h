#pragma once

#include "CoreMinimal.h"
#include "SolidSight.h"

/** What survives the trip between the open world and a closed visit. */
struct FSolidTravelSession
{
	TArray<int32> SelectedCompanionIndices;
	/** Eight keys, with INDEX_NONE where a key is empty. Empty leaves the starter assignment. */
	TArray<int32> AssignedBattlePlanIndices;
	int32 ActiveBattlePlanSlot = 0;
	/** False when the active plan was dropped back on Plans. */
	bool bBattlePlanActive = true;
	/** Formation kept while no plan is active. Line is 0. */
	uint8 HeldFormation = 0;
	/** Standard spacing is 1. */
	uint8 HeldSpacing = 1;
	ESolidSight Sight = ESolidSight::True;
	/** Next loaded world is the company stronghold interior. */
	bool bInStronghold = false;
	/** Next loaded world is the Iglin tunnels. */
	bool bInLair = false;
	/** Next open-world spawn stands on the grass in front of one tunnel mouth. */
	bool bReturnToLairMouth = false;
	/** X or Y. The end to enter, or the end to stand at after leaving. */
	TCHAR LairEnd = 0;
	/** Next open-world spawn stands just outside the stronghold entrance, facing away. */
	bool bExitStronghold = false;
};
