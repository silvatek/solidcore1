#include "SolidMonsterGroup.h"
#include "SolidRaptor.h"
#include "SolidGroundMove.h"

namespace SolidMonsterGroupPrivate
{
	/** Degrees so someone at From looks at To. A point on the target returns 0. */
	float YawFacing(const FVector2D& From, const FVector2D& To)
	{
		const FVector2D Delta = To - From;
		if (Delta.IsNearlyZero())
		{
			return 0.f;
		}
		return FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	}

	/** Horizontal distance from a ramp sample to the deck. An empty deck uses the ramp's first sample. */
	float DistanceFromDeck(const FVector& Sample, const TArray<FVector>& Deck, const FVector& Join)
	{
		const FVector2D At(Sample.X, Sample.Y);
		if (Deck.Num() == 0)
		{
			return FVector2D::Distance(At, FVector2D(Join.X, Join.Y));
		}

		float Best = TNumericLimits<float>::Max();
		for (const FVector& Point : Deck)
		{
			Best = FMath::Min(Best, FVector2D::Distance(At, FVector2D(Point.X, Point.Y)));
		}
		return Best;
	}

	/** The ramp sample farthest from the deck. False when the ramp is empty. */
	bool OuterEnd(const TArray<FVector>& Ramp, const TArray<FVector>& Deck, FVector2D& OutXY)
	{
		if (Ramp.Num() == 0)
		{
			return false;
		}

		int32 BestIndex = 0;
		float BestDistance = -1.f;
		for (int32 Index = 0; Index < Ramp.Num(); ++Index)
		{
			const float Distance = DistanceFromDeck(Ramp[Index], Deck, Ramp[0]);
			if (Distance > BestDistance || (Distance == BestDistance && Index > BestIndex))
			{
				BestDistance = Distance;
				BestIndex = Index;
			}
		}
		OutXY = FVector2D(Ramp[BestIndex].X, Ramp[BestIndex].Y);
		return true;
	}

	/**
	 * The ramp whose outer end is closer to Iglin.
	 * False when no Iglin crossing has a ramp.
	 */
	bool IglinRampOuterEnd(const TArray<SolidBridge::FBridge>& Bridges, const FVector2D& IglinTownXY, FVector2D& OutXY)
	{
		bool bFound = false;
		float BestTown = 0.f;
		for (const SolidBridge::FBridge& Bridge : Bridges)
		{
			if (!Bridge.bIglinRelion)
			{
				continue;
			}

			FVector2D From = FVector2D::ZeroVector;
			FVector2D To = FVector2D::ZeroVector;
			const bool bFrom = OuterEnd(Bridge.RampFrom, Bridge.Deck, From);
			const bool bTo = OuterEnd(Bridge.RampTo, Bridge.Deck, To);
			if (!bFrom && !bTo)
			{
				continue;
			}

			FVector2D Outer = From;
			if (!bFrom)
			{
				Outer = To;
			}
			else if (bTo && FVector2D::Distance(To, IglinTownXY) < FVector2D::Distance(From, IglinTownXY))
			{
				Outer = To;
			}
			const float Town = FVector2D::Distance(Outer, IglinTownXY);
			if (!bFound || Town < BestTown)
			{
				bFound = true;
				BestTown = Town;
				OutXY = Outer;
			}
		}
		return bFound;
	}
}

bool FSolidMonsterGroup::WorldHasMonsters(const bool bClosedVisit)
{
	return !bClosedVisit;
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

int32 FSolidMonsterGroup::PlaceBesideIglinRamp(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const TArray<int32>& ForagePointIndices,
	const FVector2D& IglinTownXY,
	const TArray<SolidBridge::FBridge>& Bridges)
{
	Members.Reset();

	FVector2D Outer = FVector2D::ZeroVector;
	if (!SolidMonsterGroupPrivate::IglinRampOuterEnd(Bridges, IglinTownXY, Outer))
	{
		return 0;
	}

	TSet<int32> Taken;
	Taken.Reserve(TreePointIndices.Num() + ForagePointIndices.Num());
	for (const int32 Index : TreePointIndices)
	{
		Taken.Add(Index);
	}
	for (const int32 Index : ForagePointIndices)
	{
		Taken.Add(Index);
	}

	struct FCandidate
	{
		int32 Index = INDEX_NONE;
		float Distance = 0.f;
		FVector2D XY = FVector2D::ZeroVector;
	};

	TArray<FCandidate> Candidates;
	Candidates.Reserve(Points.Num());
	for (int32 Index = 0; Index < Points.Num(); ++Index)
	{
		const FSolidTerrainPoint& Point = Points[Index];
		if (Point.Biome != ESolidBiome::Grassland || SolidGroundMove::IsImpassable(Point.Biome))
		{
			continue;
		}
		if (Taken.Contains(Index))
		{
			continue;
		}

		const FVector2D XY(Point.X, Point.Y);
		const float Distance = FVector2D::Distance(XY, Outer);
		if (Distance < NearLimitCm || Distance > FarLimitCm)
		{
			continue;
		}

		FCandidate Candidate;
		Candidate.Index = Index;
		Candidate.Distance = Distance;
		Candidate.XY = XY;
		Candidates.Add(Candidate);
	}

	Candidates.Sort([](const FCandidate& A, const FCandidate& B)
	{
		if (A.Distance != B.Distance)
		{
			return A.Distance < B.Distance;
		}
		return A.Index < B.Index;
	});

	TArray<FVector2D> ChosenXY;
	ChosenXY.Reserve(BesideIglinCount);
	for (const FCandidate& Candidate : Candidates)
	{
		if (Members.Num() >= BesideIglinCount)
		{
			break;
		}

		bool bTooClose = false;
		for (const FVector2D& Have : ChosenXY)
		{
			if (FVector2D::Distance(Have, Candidate.XY) < SeparationCm)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		TUniquePtr<FSolidRaptor> Raptor = MakeUnique<FSolidRaptor>();
		Raptor->PointIndex = Candidate.Index;
		Raptor->XY = Candidate.XY;
		Raptor->YawDeg = SolidMonsterGroupPrivate::YawFacing(Candidate.XY, Outer);
		Raptor->Size = ESolidMonsterSize::Small;
		Members.Add(MoveTemp(Raptor));
		ChosenXY.Add(Candidate.XY);
	}
	return Members.Num();
}

void FSolidMonsterGroup::CollectReadyToSeed(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& Seeded,
	TArray<int32>& OutReady) const
{
	OutReady.Reset();
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		if (Seeded.Contains(Index))
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
