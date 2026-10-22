#include "SolidMonsterGroup.h"

namespace
{
	/** Monster battle plans. Built once. A group finds its plan here by name. */
	const TArray<FSolidBattlePlan>& BattlePlans()
	{
		static const TArray<FSolidBattlePlan> Plans = []
		{
			TArray<FSolidBattlePlan> Built;
			Built.Add({
				TEXT("Attack"),
				ESolidBattleFormation::Mob,
				ESolidBattleSpacing::Standard,
				ESolidAggression::Attack,
				ESolidTargetStrategy::Random});
			return Built;
		}();
		return Plans;
	}
}

void FSolidMonsterGroup::GivePlan(const TCHAR* Name)
{
	Available.Reset();
	ActivePlan = INDEX_NONE;
	if (!Name || !Name[0])
	{
		return;
	}
	const FSolidBattlePlan* Plan = BattlePlans().FindByPredicate([Name](const FSolidBattlePlan& Candidate)
	{
		return Candidate.Is(Name);
	});
	if (Plan)
	{
		Available.Add(*Plan);
		ActivePlan = 0;
	}
}

FSolidMonster* FSolidMonsterGroup::Get(const int32 Index)
{
	if (!Members.IsValidIndex(Index))
	{
		return nullptr;
	}
	return Members[Index].Get();
}

const FSolidMonster* FSolidMonsterGroup::Get(const int32 Index) const
{
	if (!Members.IsValidIndex(Index))
	{
		return nullptr;
	}
	return Members[Index].Get();
}

const FSolidBattlePlan* FSolidMonsterGroup::GetActivePlan() const
{
	return Available.IsValidIndex(ActivePlan) ? &Available[ActivePlan] : nullptr;
}

void FSolidMonsterGroup::CollectReadyToSeed(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& AlreadySeeded,
	TArray<int32>& OutReady) const
{
	OutReady.Reset();
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		if (AlreadySeeded.Contains(Index))
		{
			continue;
		}
		const FSolidMonster* Monster = Members[Index].Get();
		if (!Monster)
		{
			continue;
		}
		const int32 PointIndex = Monster->PointIndex;
		if (!Points.IsValidIndex(PointIndex))
		{
			continue;
		}
		if (!(Points[PointIndex].Fog < 1.f))
		{
			continue;
		}
		OutReady.Add(Index);
	}
}
