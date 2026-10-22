#pragma once

#include "CoreMinimal.h"
#include "SolidMonster.h"
#include "SolidMonsterGroup.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Encounter definitions and a runtime encounter built from one.
 * Definitions are a static list, the way SolidEvents::Definitions are.
 * Place fills the runtime groups on the open world.
 */
namespace SolidEncounter
{
	inline constexpr int32 MaxGroupsPerEncounter = 4;
	inline constexpr int32 MaxEncounters = 32;

	/** Where the encounter finds its focus on the open world. */
	enum class ESite : uint8
	{
		/** Outer end of the Iglin–Relion ramp nearer Iglin. */
		IglinBridgeRamp,
	};

	/** Which subclass Place creates for a group member. */
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
	 * One shipped encounter: a name, a site, and the groups that stand there.
	 * Constexpr-friendly, like SolidEvents::FEvent. Members are not stored here.
	 */
	struct FDefinition
	{
		const TCHAR* Name = TEXT("");
		ESite Site = ESite::IglinBridgeRamp;
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
}

/**
 * One encounter in play: its definition, the focus once placed, and the groups.
 * Built from a SolidEncounter::FDefinition. Place fills the members.
 */
class FSolidEncounter
{
public:
	FSolidEncounter() = default;

	explicit FSolidEncounter(const SolidEncounter::FDefinition& InDefinition);

	/** The shipped definition this encounter was built from. */
	SolidEncounter::FDefinition Definition;
	/**
	 * World centimetres of the encounter focus once Place has run.
	 * For Iglin Bridge that is the Iglin ramp's outer end. Zero until then.
	 */
	FVector2D Location = FVector2D::ZeroVector;
	/** Groups that act at this place. Empty members until Place. */
	TArray<FSolidMonsterGroup> Groups;

	/**
	 * Fill Groups from Definition on the open world.
	 * Resolves Definition.Site to Location and places each group's members.
	 * @return Total members placed across every group.
	 */
	int32 Place(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges,
		float PointSpacingCm = 200.f);
};
