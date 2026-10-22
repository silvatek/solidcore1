#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"
#include "Combat/SolidWeapon.h"
#include "Company/SolidSquad.h"
#include "Monster/SolidMonsterGroup.h"

class USolidGameState;

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
	Abort,
};

/**
 * One line in the battle log.
 * A prepare, an attack, a prepare that was abandoned, or an attack on someone already dead.
 */
struct FSolidBattleLogEntry
{
	/** Battle time when this happened. 0 is the start. The unit is weapon Speed. */
	float Time = 0.f;
	ESolidBattleLogKind Kind = ESolidBattleLogKind::Prepare;
	/** Captain, a companion's name, or the monster's kind. */
	FString Actor;
	/** Who an attack struck, or who an abort was aimed at. Empty for a prepare or an abandon. */
	FString Target;
	/** Weapon an attack used. Empty for a prepare, an abandon, or an abort. */
	FString Weapon;
	/** True when this attack left the target's armour at 0. */
	bool bKilled = false;

	/** One line a later screen can show. */
	FString Summary() const;
};

/**
 * One battle. It links one squad, the captain's loadout, and one monster group.
 * The squad and the group already hold their battle plans. This reads the active plan on each.
 * SquadWeapon and MonsterWeapon are the weapon each member fights with.
 * Power is that weapon's Power. Defence is armour Strength. A later gauge can show the totals.
 * Each fighter prepares and recovers on their own weapon Speed. The blow lands when that prepare ends.
 * That blow cannot drop armour Strength below 0. A blow that leaves it at 0 says kills.
 * An attack on armour already at 0 is aborted.
 * A blow on a fighter who is retreating is reduced by RetreatReduction.
 * A fighter keeps the same enemy while that enemy can still be struck.
 * The battle is finished when either side has no armour left. Advance then does nothing.
 * Blows already due in the moment that finishes it still land.
 * A blow that kills the captain records Killed by monsters on the play state, when that event has not already fired.
 * An escape will finish it too, once a side can leave.
 * The log records those moments, in order.
 */
class FSolidBattle
{
public:
	FSolidBattle() = default;
	FSolidBattle(USolidSquad* InSquad, FSolidMonsterGroup* InGroup);

	/** Fraction of Power a blow loses while the target is retreating. Play-testing chooses it. */
	static constexpr float RetreatReduction = 0.75f;

	/** The squad in this battle. Empty when that squad has gone. */
	TWeakObjectPtr<USolidSquad> Squad;

	/**
	 * The captain's worn boxes and pack. The play state owns the loadout.
	 * Null when this battle has not been given one.
	 */
	SolidInventory::FLoadout* Inventory = nullptr;

	/**
	 * The play state this battle records into.
	 * Null when this battle has not been given one.
	 */
	USolidGameState* State = nullptr;

	/** The monster group in this battle. The encounter owns the group. */
	FSolidMonsterGroup* Group = nullptr;

	/**
	 * How many selected boxes fight, from the top.
	 * INDEX_NONE fights every selected companion. Zero fights none.
	 * The live fight uses the squad cap, which starts at none.
	 */
	int32 FieldedCount = INDEX_NONE;

	/** Battle time. 0 is the start. The unit is weapon Speed. */
	float Time = 0.f;

	/** What has happened, in order. A later screen can show these lines. */
	TArray<FSolidBattleLogEntry> Log;

	/** The squad's active plan, or null when this battle has no squad. */
	const FSolidBattlePlan* GetSquadPlan() const;

	/** The group's active plan, or null when this battle has no group. */
	const FSolidBattlePlan* GetGroupPlan() const;

	/**
	 * The weapon that squad member fights with.
	 * Index 0 is the captain. Each later index is a companion the squad has out, in squad order.
	 * An empty hand, or a missing member, is an empty weapon.
	 */
	FSolidWeapon SquadWeapon(int32 Index) const;

	/**
	 * The weapon that monster fights with.
	 * The index is that member of the group. A missing monster is an empty weapon.
	 */
	FSolidWeapon MonsterWeapon(int32 Index) const;

	/** Power of the captain's main hand, plus each companion the squad has out. */
	int32 GetSquadPower() const;

	/** Armour Strength of the captain's body, plus each companion the squad has out. */
	float GetSquadDefence() const;

	/** Power of every monster in the group. */
	int32 GetGroupPower() const;

	/** Armour Strength of every monster in the group. */
	float GetGroupDefence() const;

	/**
	 * True when either side has no armour left.
	 * An escape finishes the battle too, once a side can leave.
	 */
	bool Finished() const;

	/**
	 * Move each fighter forward by Delta, in the same unit as weapon Speed.
	 * A fighter whose plan stores attack prepares, lands their own blow, and recovers.
	 * Prepare and recover each take that fighter's Speed. A retreat abandons a prepare when it ends.
	 * An empty main hand does not prepare. Each of those moments is appended to the log.
	 * A battle that is already finished does not move.
	 */
	void Advance(float Delta);

	/**
	 * Move time forward to the next moment the log gains a line.
	 * The opening prepares are at time 0. Later moments are the end of a prepare or a recover.
	 * False when nobody will act, and when the battle is already finished.
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
		const FString& Weapon,
		bool bKilled = false);

	/** Record Killed by monsters when the play state has not already fired it. */
	void NoteCaptainKilled() const;

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
