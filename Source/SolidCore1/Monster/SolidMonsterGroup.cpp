#include "SolidMonsterGroup.h"

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
