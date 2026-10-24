#pragma once

#include "CoreMinimal.h"
#include "SolidMonster.h"
#include "SolidMonsterGroup.h"
#include "Terrain/SolidTerrainTypes.h"

class USolidGameState;

/**
 * Encounter definitions and a runtime encounter built from one.
 * Definitions are a static list, the way SolidEvents::Definitions are.
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

	/** One group in a definition: kind, size, and how many members. */
	struct FGroup
	{
		EMonsterKind Kind = EMonsterKind::Raptor;
		ESolidMonsterSize Size = ESolidMonsterSize::Small;
		int32 Count = 0;
		/** Name of the battle plan this group uses. Empty until a definition sets one. */
		const TCHAR* Plan = TEXT("");
	};

	/**
	 * One shipped encounter: a name and the groups that stand there.
	 * Constexpr-friendly, like SolidEvents::FEvent. Members are not stored here.
	 * Where they stand is the chart mark. Place takes that position.
	 */
	struct FDefinition
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

		constexpr FDefinition WithGroup(
			const EMonsterKind Kind,
			const ESolidMonsterSize Size,
			const int32 Count,
			const TCHAR* Plan = TEXT("")) const
		{
			FDefinition Def = *this;
			if (Def.GroupCount < MaxGroupsPerEncounter)
			{
				Def.Groups[Def.GroupCount].Kind = Kind;
				Def.Groups[Def.GroupCount].Size = Size;
				Def.Groups[Def.GroupCount].Count = Count;
				Def.Groups[Def.GroupCount].Plan = Plan;
				++Def.GroupCount;
			}
			return Def;
		}
	};

	/** A closed visit has no encounters. The tunnels stay empty. */
	bool WorldHasEncounters(bool bClosedVisit);

	/** True when Disable has recorded this encounter name. */
	bool IsDisabled(const USolidGameState& State, const TCHAR* Name);
	/** ChooseMonsters skips this name for the rest of the game. */
	void Disable(USolidGameState& State, const TCHAR* Name);
}

/**
 * One encounter in play: its definition, the focus once placed, and the groups.
 * Built from a SolidEncounter::FDefinition.
 * The groups hold their plans and their members. Place chooses where those members stand.
 */
class FSolidEncounter
{
public:
	FSolidEncounter() = default;

	explicit FSolidEncounter(const SolidEncounter::FDefinition& InDefinition);

	/** The shipped definition this encounter was built from. */
	SolidEncounter::FDefinition Definition;
	/** World centimetres of the encounter focus once Place has run. */
	FVector2D Location = FVector2D::ZeroVector;
	/** Groups from the definition. Members are armed. Place chooses their points. */
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
