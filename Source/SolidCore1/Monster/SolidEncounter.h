#pragma once

#include "CoreMinimal.h"
#include "SolidMonsterGroup.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * A named place where one or more monster groups stand.
 * The catalog lists each encounter. Place fills its groups on the open world.
 */
class FSolidEncounter
{
public:
	FSolidEncounter() = default;

	/** Display and catalog name, such as "Iglin Bridge". */
	FString Name;
	/**
	 * World centimetres of the encounter focus once Place has run.
	 * For Iglin Bridge that is the Iglin ramp's outer end. Zero until then.
	 */
	FVector2D Location = FVector2D::ZeroVector;
	/** Groups that act at this place. Empty members until Place. */
	TArray<FSolidMonsterGroup> Groups;

	/** Nearest a member may be to the encounter focus (cm). */
	static constexpr float NearLimitCm = 800.f;
	/** Farthest a member may be from that focus (cm). */
	static constexpr float FarLimitCm = 4000.f;
	/** Later members stay at least this far from one already chosen (cm). */
	static constexpr float SeparationCm = 400.f;
	/** How many Small raptors the Iglin Bridge encounter places. */
	static constexpr int32 IglinRaptorCount = 3;

	/**
	 * Fill Groups for this encounter on the open world.
	 * Iglin Bridge places three Small raptors on grassland beside the Iglin ramp,
	 * clear of trees and foraging piles, and sets Location to that outer end.
	 * @return Total members placed across every group.
	 */
	int32 Place(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges);
};

namespace SolidEncounter
{
	/** A closed visit has no encounters. The tunnels stay empty. */
	bool WorldHasEncounters(bool bClosedVisit);

	/** The Iglin Bridge encounter: one empty raptor group until Place. */
	FSolidEncounter IglinBridge();

	/**
	 * Every shipped encounter.
	 * Initially only Iglin Bridge. Group members are empty until Place.
	 */
	TArray<FSolidEncounter> Catalog();
}
