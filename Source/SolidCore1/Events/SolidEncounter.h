#pragma once

#include "CoreMinimal.h"
#include "Monster/SolidMonster.h"
#include "Monster/SolidMonsterGroup.h"
#include "Terrain/SolidTerrainTypes.h"

class USolidGameState;

/**
 * Encounter catalog rows and a runtime encounter built from one.
 * StaticEncounters is the static list, the way SolidEvents::Definitions lists events.
 * The constructor fills each group's plan and its members. Place chooses where they stand.
 */
namespace SolidEncounter
{
	inline constexpr int32 MaxGroupsPerEncounter = 4;
	inline constexpr int32 MaxEncounters = 32;

	/** Which subclass a group member is. */
	enum class EMonsterKind : uint8
	{
		Raptor,
	};

	/** One group in a catalog row: kind, size, and how many members. */
	struct FGroup
	{
		EMonsterKind Kind = EMonsterKind::Raptor;
		ESolidMonsterSize Size = ESolidMonsterSize::Small;
		int32 Count = 0;
		/** Name of the battle plan this group uses. Empty until a row sets one. */
		const TCHAR* Plan = TEXT("");
	};

	/**
	 * One shipped encounter: a name, the groups that stand there, and one event it can wait on.
	 * Constexpr-friendly, like SolidEvents::FEvent. Members are not stored here.
	 * Where they stand is the chart mark. Place takes that position.
	 * An empty event id is placed with the others. A second Require keeps the later id.
	 */
	struct FStaticEncounter
	{
		const TCHAR* Name = TEXT("");
		FGroup Groups[MaxGroupsPerEncounter] = {};
		int32 GroupCount = 0;
		/** Nearest a member may be to the encounter focus (cm). */
		float NearLimitCm = 800.f;
		/** Farthest a member may be from that focus (cm). */
		float FarLimitCm = 4000.f;
		/** Later members stay at least this far from one already chosen (cm). */
		float SeparationCm = 400.f;
		/** Event id this encounter waits on. Empty means it is placed with the others. */
		const TCHAR* EventId = TEXT("");

		constexpr FStaticEncounter WithGroup(
			const EMonsterKind Kind,
			const ESolidMonsterSize Size,
			const int32 Count,
			const TCHAR* Plan = TEXT("")) const
		{
			FStaticEncounter Row = *this;
			if (Row.GroupCount < MaxGroupsPerEncounter)
			{
				Row.Groups[Row.GroupCount].Kind = Kind;
				Row.Groups[Row.GroupCount].Size = Size;
				Row.Groups[Row.GroupCount].Count = Count;
				Row.Groups[Row.GroupCount].Plan = Plan;
				++Row.GroupCount;
			}
			return Row;
		}

		/** Depend on this event. A later call keeps that later id. Empty clears the wait. */
		constexpr FStaticEncounter Require(const TCHAR* InEventId) const
		{
			FStaticEncounter Row = *this;
			Row.EventId = (InEventId && InEventId[0]) ? InEventId : TEXT("");
			return Row;
		}

		/**
		 * True when this row is placed.
		 * No event id is placed. Fired is bFired at FindIndexById. An unknown id, or a missing state, is not.
		 */
		bool HasFired(const USolidGameState* State) const;
	};

	/** A closed visit has no encounters. The tunnels stay empty. */
	bool WorldHasEncounters(bool bClosedVisit);

	/** True when Disable has recorded this encounter name. */
	bool IsDisabled(const USolidGameState& State, const TCHAR* Name);
	/** ChooseMonsters skips this name for the rest of the game. */
	void Disable(USolidGameState& State, const TCHAR* Name);
}

/**
 * One encounter in play: its catalog row, the focus once placed, and the groups.
 * Built from a SolidEncounter::FStaticEncounter.
 * The groups hold their plans and their members. Place chooses their points.
 */
class FSolidEncounter
{
public:
	FSolidEncounter() = default;

	explicit FSolidEncounter(const SolidEncounter::FStaticEncounter& InDefinition);

	/** The catalog row this encounter was built from. */
	SolidEncounter::FStaticEncounter Definition;
	/** World centimetres of the encounter focus once Place has run. */
	FVector2D Location = FVector2D::ZeroVector;
	/** Groups from the catalog row. Members are armed. Place chooses their points. */
	TArray<FSolidMonsterGroup> Groups;

	/**
	 * Choose a point for each member already in the groups.
	 * Focus is the mark position, in centimetres. A member with no point is dropped.
	 * @return Total members placed across every group.
	 */
	int32 Place(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& Focus,
		float PointSpacingCm = 200.f);
};

namespace SolidEncounter
{
	/**
	 * Place one mark when the row stands, and append it to Encounters.
	 * A closed visit, a disabled name, or an unfired event appends nothing.
	 * The same name already standing at this focus stays, and is not added again.
	 * Two focuses of one name are two encounters.
	 * @return Members placed on this call. Zero when the row stays out, or when no point fits.
	 */
	int32 PlaceMark(
		TArray<FSolidEncounter>& Encounters,
		const FStaticEncounter& Definition,
		bool bDisabled,
		bool bClosedVisit,
		const USolidGameState* State,
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& Focus,
		float PointSpacingCm);
}
