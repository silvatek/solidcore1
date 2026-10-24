#include "SolidEncounter.h"
#include "SolidEncounters.h"
#include "SolidRaptor.h"
#include "SolidGroundMove.h"
#include "Inventory/SolidItem.h"
#include "SolidGameState.h"

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
			Raptor->Arm(FSolidRaptor::Claws, FSolidRaptor::Hide, FSolidItem::StartingDraw);
			return Raptor;
		}
		default:
			return nullptr;
		}
	}

	/** The members the definition names. Where they stand waits for Place. */
	void Recruit(FSolidMonsterGroup& Group, const SolidEncounter::FGroup& Spec)
	{
		Group.Members.Reset();
		for (int32 Index = 0; Index < Spec.Count; ++Index)
		{
			TUniquePtr<FSolidMonster> Monster = MakeMonster(Spec.Kind, Spec.Size);
			if (!Monster)
			{
				break;
			}
			Group.Members.Add(MoveTemp(Monster));
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
		if (Spec.Count <= 0 || Group.Members.Num() <= 0)
		{
			Group.Members.Reset();
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
		ChosenXY.Reserve(Group.Members.Num());
		int32 Placed = 0;
		for (const FCandidate& Candidate : Candidates)
		{
			if (Placed >= Group.Members.Num())
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

			FSolidMonster* Monster = Group.Get(Placed);
			if (!Monster)
			{
				continue;
			}
			Monster->PointIndex = Candidate.Index;
			Monster->XY = Candidate.XY;
			Monster->YawDeg = YawFacing(Candidate.XY, Outer);
			Taken.Add(Candidate.Index);
			ChosenXY.Add(Candidate.XY);
			++Placed;
		}
		Group.Members.SetNum(Placed);
		return Placed;
	}
}

bool SolidEncounter::WorldHasEncounters(const bool bClosedVisit)
{
	return !bClosedVisit;
}

bool SolidEncounter::IsDisabled(const USolidGameState& State, const TCHAR* Name)
{
	if (!Name || !Name[0])
	{
		return false;
	}
	for (const TCHAR* Listed : State.DisabledEncounters)
	{
		if (Listed && FCString::Stricmp(Listed, Name) == 0)
		{
			return true;
		}
	}
	return false;
}

void SolidEncounter::Disable(USolidGameState& State, const TCHAR* Name)
{
	if (!Name || !Name[0] || IsDisabled(State, Name))
	{
		return;
	}
	if (const FDefinition* Def = FindByName(Name))
	{
		State.DisabledEncounters.Add(Def->Name);
		return;
	}
	State.DisabledEncounters.Add(Name);
}

FSolidEncounter::FSolidEncounter(const SolidEncounter::FDefinition& InDefinition)
	: Definition(InDefinition)
{
	Groups.SetNum(FMath::Max(0, Definition.GroupCount));
	for (int32 GroupIndex = 0; GroupIndex < Definition.GroupCount && Groups.IsValidIndex(GroupIndex); ++GroupIndex)
	{
		Groups[GroupIndex].GivePlan(Definition.Groups[GroupIndex].Plan);
		SolidEncounterPrivate::Recruit(Groups[GroupIndex], Definition.Groups[GroupIndex]);
	}
}

int32 FSolidEncounter::Place(
	const TArray<FSolidTerrainPoint>& Points,
	const TArray<int32>& TreePointIndices,
	const TArray<int32>& ForagePointIndices,
	const FVector2D& Focus,
	const float PointSpacingCm)
{
	Location = Focus;
	Groups.SetNum(FMath::Max(0, Definition.GroupCount));
	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		FSolidMonsterGroup& Group = Groups[GroupIndex];
		Group.Seeded.Reset();
		if (GroupIndex < Definition.GroupCount)
		{
			Group.GivePlan(Definition.Groups[GroupIndex].Plan);
			SolidEncounterPrivate::Recruit(Group, Definition.Groups[GroupIndex]);
		}
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
			Focus,
			PointSpacingCm);
	}
	return Placed;
}
