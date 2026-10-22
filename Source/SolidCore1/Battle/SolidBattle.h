#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Company/SolidSquad.h"
#include "Monster/SolidMonsterGroup.h"

namespace SolidInventory
{
	struct FLoadout;
}

/** What one line in the battle log records. */
enum class ESolidBattleLogKind : uint8
{
	Prepare,
	Attack,
	Abandon,
};

/**
 * One line in the battle log.
 * A prepare, an attack, or a prepare that was abandoned. A later screen can show Text().
 */
struct FSolidBattleLogEntry
{
	/** Battle time when this happened. 0 is the start. The unit is weapon Speed. */
	float Time = 0.f;
	ESolidBattleLogKind Kind = ESolidBattleLogKind::Prepare;
	/** Captain, a companion's name, or the monster's kind. */
	FString Actor;
	/** Who an attack struck. Empty for a prepare or an abandon. */
	FString Target;
	/** Weapon an attack used. Empty for a prepare or an abandon. */
	FString Weapon;

	/** One line a later screen can show. */
	FString Text() const;
};

/**
 * One battle. It links one squad, the captain's loadout, and one monster group.
 * The squad and the group already hold their battle plans. This reads the active plan on each.
 * Power is weapon Power. Defence is armour Strength. A later gauge can show the totals.
 * Each fighter prepares and recovers on their own weapon Speed. The blow lands when that prepare ends.
 * The log records those moments, in order.
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

	/** Battle time. 0 is the start. The unit is weapon Speed. */
	float Time = 0.f;

	/** What has happened, in order. A later screen can show these lines. */
	TArray<FSolidBattleLogEntry> Log;

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
	 * Move each fighter forward by Delta, in the same unit as weapon Speed.
	 * A fighter whose plan stores attack prepares, lands their own blow, and recovers.
	 * Prepare and recover each take that fighter's Speed. A retreat abandons a prepare when it ends.
	 * An empty main hand does not prepare. Each of those moments is appended to the log.
	 */
	void Advance(float Delta);

	/**
	 * Move time forward to the next moment the log gains a line.
	 * The opening prepares are at time 0. Later moments are the end of a prepare or a recover.
	 * False when nobody will act.
	 */
	bool AdvanceToNextEvent();

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

	void Record(
		float When,
		ESolidBattleLogKind Kind,
		const FString& Actor,
		const FString& Target,
		const FString& Weapon);

	/** Time until the next logged moment. Negative when nobody will act. */
	float TimeUntilNextEvent();

	void AdvanceCycle(
		FCycle& Cycle,
		float Speed,
		float Delta,
		bool bAttacking,
		float Now,
		float Arrived,
		TFunctionRef<void(float When)> BeginPrepare,
		TFunctionRef<void(float When)> LandBlow,
		TFunctionRef<void(float When)> Abandon);

	FCycle CaptainCycle;
	TMap<int32, FCycle> CompanionCycles;
	TMap<FSolidMonster*, FCycle> MonsterCycles;
};
