#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Company/SolidSquad.h"
#include "Monster/SolidMonsterGroup.h"

namespace SolidInventory
{
	struct FLoadout;
}

/**
 * One battle. It links one squad, the captain's loadout, and one monster group.
 * The squad and the group already hold their battle plans. This reads the active plan on each.
 * Power is weapon Power. Defence is armour Strength. A later gauge can show the totals.
 * Each fighter prepares and recovers on their own weapon Speed. The blow lands when that prepare ends.
 */
class FSolidBattle
{
public:
	FSolidBattle() = default;
	FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup);

	/** The squad in this battle. Empty when that squad has gone. */
	TWeakObjectPtr<USolidSquad> Squad;

	/**
	 * The captain's worn boxes and pack. The play state owns the loadout.
	 * Null when this battle has not been given one.
	 */
	SolidInventory::FLoadout* Inventory = nullptr;

	/** The monster group in this battle. The encounter owns the group. */
	FSolidMonsterGroup* Group = nullptr;

	/** The squad's active plan, or null when this battle has no squad. */
	const FSolidBattlePlan* GetSquadPlan() const;

	/** The group's active plan, or null when this battle has no group. */
	const FSolidBattlePlan* GetGroupPlan() const;

	/** Power of the captain's main hand, plus each companion the squad has out. */
	int32 GetSquadPower() const;

	/** Armour Strength of the captain's body, plus each companion the squad has out. */
	float GetSquadDefence() const;

	/** Power of every monster in the group. */
	int32 GetGroupPower() const;

	/** Armour Strength of every monster in the group. */
	float GetGroupDefence() const;

	/**
	 * Move each fighter forward by Time, in the same unit as weapon Speed.
	 * A fighter whose plan stores attack prepares, lands their own blow, and recovers.
	 * Prepare and recover each take that fighter's Speed. A retreat abandons a prepare when it ends.
	 * An empty main hand does not prepare.
	 */
	void Advance(float Time);

private:
	/** Where one fighter is in prepare or recover. */
	struct FCycle
	{
		enum class EStep : uint8
		{
			Prepare,
			Recover,
		};

		EStep Step = EStep::Prepare;
		float TimeLeft = 0.f;
		bool bEngaged = false;
		/** The monster chosen when this squad member started preparing. */
		FSolidMonster* Target = nullptr;
	};

	void AdvanceCycle(
		FCycle& Cycle,
		float Speed,
		float Time,
		bool bAttacking,
		TFunctionRef<void()> BeginPrepare,
		TFunctionRef<void()> LandBlow);

	FCycle CaptainCycle;
	TMap<int32, FCycle> CompanionCycles;
	TMap<FSolidMonster*, FCycle> MonsterCycles;
};
