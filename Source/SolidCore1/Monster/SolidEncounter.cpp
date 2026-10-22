#include "SolidEncounter.h"
#include "SolidRaptor.h"
#include "SolidGroundMove.h"

namespace SolidEncounterPrivate
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

	bool ResolveSite(
		const SolidEncounter::ESite Site,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges,
		FVector2D& OutXY)
	{
		switch (Site)
		{
		case SolidEncounter::ESite::IglinBridgeRamp:
			return IglinRampOuterEnd(Bridges, IglinTownXY, OutXY);
		default:
			return false;
		}
	}

	TUniquePtr<FSolidMonster> MakeMonster(
		const SolidEncounter::EMonsterKind Kind,
		const ESolidMonsterSize Size)
	{
		switch (Kind)
		{
		case SolidEncounter::EMonsterKind::Raptor:
		{
			TUniquePtr<FSolidRaptor> Raptor = MakeUnique<FSolidRaptor>();
			Raptor->Size = Size;
			return Raptor;
		}
		default:
			return nullptr;
		}
	}

	/**
	 * True when XY is a corner of a quad that also has a point of Biome.
	 * The limit is one diagonal step of PointSpacingCm, plus a centimetre.
	 */
	bool SharesQuadWith(
		const TArray<FSolidTerrainPoint>& Points,
		const FVector2D& XY,
		const ESolidBiome Biome,
		const float PointSpacingCm)
	{
		const float Limit = FMath::Max(PointSpacingCm, 1.f) * FMath::Sqrt(2.f) + 1.f;
		const float LimitSq = Limit * Limit;
		for (const FSolidTerrainPoint& Point : Points)
		{
			if (Point.Biome != Biome)
			{
				continue;
			}
			const float DX = Point.X - XY.X;
			const float DY = Point.Y - XY.Y;
			if (DX * DX + DY * DY <= LimitSq)
			{
				return true;
			}
		}
		return false;
	}

	/** Members of one group on grassland in the band around Outer. Skips Taken points. */
	int32 PlaceGroup(
		FSolidMonsterGroup& Group,
		const SolidEncounter::FGroup& Spec,
		const SolidEncounter::FDefinition& Definition,
		const TArray<FSolidTerrainPoint>& Points,
		TSet<int32>& Taken,
		const FVector2D& Outer,
		const float PointSpacingCm)
	{
		Group.Members.Reset();
		if (Spec.Count <= 0)
		{
			return 0;
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
			if (Distance < Definition.NearLimitCm || Distance > Definition.FarLimitCm)
			{
				continue;
			}
			// The mountain shelf is 10 m. A shared quad is that cliff.
			if (SharesQuadWith(Points, XY, ESolidBiome::Mountain, PointSpacingCm))
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
		ChosenXY.Reserve(Spec.Count);
		for (const FCandidate& Candidate : Candidates)
		{
			if (Group.Members.Num() >= Spec.Count)
			{
				break;
			}

			bool bTooClose = false;
			for (const FVector2D& Have : ChosenXY)
			{
				if (FVector2D::Distance(Have, Candidate.XY) < Definition.SeparationCm)
				{
					bTooClose = true;
					break;
				}
			}
			if (bTooClose)
			{
				continue;
			}

			TUniquePtr<FSolidMonster> Monster = MakeMonster(Spec.Kind, Spec.Size);
			if (!Monster)
			{
				continue;
			}
			Monster->PointIndex = Candidate.Index;
			Monster->XY = Candidate.XY;
			Monster->YawDeg = YawFacing(Candidate.XY, Outer);
			Taken.Add(Candidate.Index);
			Group.Members.Add(MoveTemp(Monster));
			ChosenXY.Add(Candidate.XY);
		}
		return Group.Members.Num();
	}
}

bool SolidEncounter::WorldHasEncounters(const bool bClosedVisit)
{
	return !bClosedVisit;
}

FSolidEncounter::FSolidEncounter(const SolidEncounter::FDefinition& InDefinition)
	: Definition(InDefinition)
{
	Groups.SetNum(FMath::Max(0, Definition.GroupCount));
}

int32 FSolidEncounter::Place(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const TArray<int32>& ForagePointIndices,
	const FVector2D& IglinTownXY,
	const TArray<SolidBridge::FBridge>& Bridges,
	const float PointSpacingCm)
{
	Location = FVector2D::ZeroVector;
	Groups.SetNum(FMath::Max(0, Definition.GroupCount));
	for (FSolidMonsterGroup& Group : Groups)
	{
		Group.Members.Reset();
		Group.Seeded.Reset();
	}

	FVector2D Outer = FVector2D::ZeroVector;
	if (!SolidEncounterPrivate::ResolveSite(Definition.Site, IglinTownXY, Bridges, Outer))
	{
		return 0;
	}

	Location = Outer;

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

	int32 Placed = 0;
	for (int32 GroupIndex = 0; GroupIndex < Definition.GroupCount; ++GroupIndex)
	{
		if (!Groups.IsValidIndex(GroupIndex))
		{
			break;
		}
		Placed += SolidEncounterPrivate::PlaceGroup(
			Groups[GroupIndex],
			Definition.Groups[GroupIndex],
			Definition,
			Points,
			Taken,
			Outer,
			PointSpacingCm);
	}
	return Placed;
}
