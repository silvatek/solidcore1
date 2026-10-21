#include "SolidMonster.h"
#include "SolidGroundMove.h"

namespace SolidMonsterPrivate
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

int32 SolidMonster::CollectChoices(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const TArray<int32>& ForagePointIndices,
	const FVector2D& IglinTownXY,
	const TArray<SolidBridge::FBridge>& Bridges,
	TArray<FChoice>& OutChoices)
{
	OutChoices.Reset();

	FVector2D Outer = FVector2D::ZeroVector;
	if (!SolidMonsterPrivate::IglinRampOuterEnd(Bridges, IglinTownXY, Outer))
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
	ChosenXY.Reserve(Count);
	for (const FCandidate& Candidate : Candidates)
	{
		if (OutChoices.Num() >= Count)
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

		FChoice Choice;
		Choice.PointIndex = Candidate.Index;
		Choice.XY = Candidate.XY;
		Choice.YawDeg = SolidMonsterPrivate::YawFacing(Candidate.XY, Outer);
		OutChoices.Add(Choice);
		ChosenXY.Add(Candidate.XY);
	}
	return OutChoices.Num();
}

void SolidMonster::CollectReadyToSeed(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<FChoice>& Choices,
	const TArray<int32>& Seeded,
	TArray<int32>& OutReady)
{
	OutReady.Reset();
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		if (Seeded.Contains(Index))
		{
			continue;
		}
		const int32 PointIndex = Choices[Index].PointIndex;
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
