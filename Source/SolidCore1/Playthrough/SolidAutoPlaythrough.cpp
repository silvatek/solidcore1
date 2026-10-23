#include "SolidAutoPlaythrough.h"

#include "SolidCharacter.h"
#include "SolidClickMove.h"
#include "SolidGameMode.h"
#include "GameFramework/PlayerController.h"
#include "SolidGameState.h"
#include "SolidGroundMove.h"
#include "Monster/SolidEncounter.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidRoad.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldChart.h"
#include "Towns/SolidTowns.h"
#include "Algo/Reverse.h"

namespace
{
	constexpr float ProgressCm = 1.f;
	/** A step this far onto a ramp is refused. The walk stays on the grass. */
	constexpr float RampProbeCm = 80.f;
	constexpr float SegmentStepCm = 50.f;
	constexpr TCHAR EncounterName[] = TEXT("Iglin Bridge");

	bool IsIglinRelionRoad(const SolidRoad::FDefinition& Definition)
	{
		const int32 From = Definition.From.LocationId;
		const int32 To = Definition.To.LocationId;
		return (From == SolidTowns::Iglin && To == SolidTowns::Relion)
			|| (From == SolidTowns::Relion && To == SolidTowns::Iglin);
	}

	const SolidRoad::FRoute* FindIglinRelionRoad(const USolidWorldChart& Chart)
	{
		for (const SolidRoad::FRoute& Route : Chart.GetRoads())
		{
			if (IsIglinRelionRoad(Route.Definition))
			{
				return &Route;
			}
		}
		return nullptr;
	}

	bool IsIglinBridge(const FSolidEncounter& Encounter)
	{
		return Encounter.Definition.Name
			&& FCString::Strcmp(Encounter.Definition.Name, EncounterName) == 0;
	}

	bool OnIglinRamp(const ASolidGameMode& Mode, const FVector2D& Point)
	{
		for (const TObjectPtr<ASolidBridge>& Actor : Mode.GetBridges())
		{
			const ASolidBridge* Bridge = Actor.Get();
			if (!Bridge || !Bridge->HasBridge() || !Bridge->GetBridge().bIglinRelion)
			{
				continue;
			}
			if (SolidGroundMove::OnRamp(Bridge->GetBridge(), Point.X, Point.Y))
			{
				return true;
			}
		}
		return false;
	}

	bool SegmentOnIglinRamp(const ASolidGameMode& Mode, const FVector2D& From, const FVector2D& To)
	{
		const float Length = FVector2D::Distance(From, To);
		const int32 Steps = FMath::Max(1, FMath::CeilToInt(Length / SegmentStepCm));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Steps);
			if (OnIglinRamp(Mode, FMath::Lerp(From, To, T)))
			{
				return true;
			}
		}
		return false;
	}

	/** True when a placed Iglin Bridge member can see this ground point. */
	bool IglinBridgeDetects(const ASolidGameMode& Mode, const FVector2D& Here)
	{
		for (const FSolidEncounter& Encounter : Mode.GetEncounters())
		{
			if (!IsIglinBridge(Encounter))
			{
				continue;
			}
			for (const FSolidMonsterGroup& Group : Encounter.Groups)
			{
				for (int32 Index = 0; Index < Group.Num(); ++Index)
				{
					const FSolidMonster* Monster = Group.Get(Index);
					if (Monster && Monster->Detects(Here))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** The placed member closest to Iglin. That is the one the walk from town meets. */
	const FSolidMonster* ClosestIglinBridgeMonster(const ASolidGameMode& Mode, const FVector2D& IglinXY)
	{
		const FSolidMonster* Found = nullptr;
		float Best = TNumericLimits<float>::Max();
		for (const FSolidEncounter& Encounter : Mode.GetEncounters())
		{
			if (!IsIglinBridge(Encounter))
			{
				continue;
			}
			for (const FSolidMonsterGroup& Group : Encounter.Groups)
			{
				for (int32 Index = 0; Index < Group.Num(); ++Index)
				{
					const FSolidMonster* Monster = Group.Get(Index);
					if (!Monster || Monster->PointIndex == INDEX_NONE)
					{
						continue;
					}
					const float Dist = FVector2D::DistSquared(Monster->XY, IglinXY);
					if (Dist < Best)
					{
						Best = Dist;
						Found = Monster;
					}
				}
			}
		}
		return Found;
	}

	/**
	 * A point inside that monster's detection, toward Iglin, and off the ramps.
	 * Standing there starts the fight. Standing on a ramp would show the message instead.
	 */
	bool EncounterApproach(
		const ASolidGameMode& Mode,
		const FSolidMonster& Monster,
		const FVector2D& IglinXY,
		FVector2D& OutPoint)
	{
		const FVector2D TowardIglin = IglinXY - Monster.XY;
		const float Length = TowardIglin.Size();
		if (Length < 1.f || Monster.DetectionRange <= 1.f)
		{
			return false;
		}

		const FVector2D Direction = TowardIglin / Length;
		const FVector2D Perp(-Direction.Y, Direction.X);
		const float Limit = Monster.DetectionRange - 1.f;
		auto Consider = [&Mode, &Monster, Limit, &OutPoint](const FVector2D& Point)
		{
			if (FVector2D::Distance(Point, Monster.XY) > Limit || OnIglinRamp(Mode, Point))
			{
				return false;
			}
			OutPoint = Point;
			return true;
		};

		const float Preferred = FMath::Min(Monster.DetectionRange * 0.5f, Limit);
		if (Consider(Monster.XY + Direction * Preferred))
		{
			return true;
		}
		for (float Distance = Preferred; Distance <= Limit; Distance += SegmentStepCm)
		{
			if (Consider(Monster.XY + Direction * Distance))
			{
				return true;
			}
		}
		const float Sides[] = {1.f, -1.f};
		for (float Distance = Preferred; Distance <= Limit; Distance += SegmentStepCm)
		{
			for (const float Side : Sides)
			{
				for (float Across = SegmentStepCm; Across <= Limit; Across += SegmentStepCm)
				{
					if (Consider(Monster.XY + Direction * Distance + Perp * Side * Across))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** Slide the approach sideways until the straight line from the road stays off the ramps. */
	bool ClearOfRamp(
		const ASolidGameMode& Mode,
		const FSolidMonster& Monster,
		const FVector2D& From,
		const FVector2D& Approach,
		FVector2D& OutPoint)
	{
		if (!SegmentOnIglinRamp(Mode, From, Approach))
		{
			OutPoint = Approach;
			return true;
		}

		const FVector2D Delta = Approach - From;
		const float DeltaLength = Delta.Size();
		const FVector2D Perp = DeltaLength > 1.f
			? FVector2D(-Delta.Y, Delta.X) / DeltaLength
			: FVector2D(1.f, 0.f);
		const float Limit = Monster.DetectionRange - 1.f;
		const float Sides[] = {1.f, -1.f};
		for (const float Side : Sides)
		{
			for (float Across = 100.f; Across <= 2000.f; Across += 100.f)
			{
				const FVector2D Candidate = Approach + Perp * Side * Across;
				if (FVector2D::Distance(Candidate, Monster.XY) > Limit || OnIglinRamp(Mode, Candidate))
				{
					continue;
				}
				if (SegmentOnIglinRamp(Mode, From, Candidate))
				{
					continue;
				}
				OutPoint = Candidate;
				return true;
			}
		}
		return false;
	}
}

void FSolidAutoPlaythrough::CollectSteps(TArray<EStep>& OutSteps)
{
	OutSteps.Reset();
	OutSteps.Add(EStep::WalkToMonsterEncounter);
	OutSteps.Add(EStep::WakeInCave);
}

const TCHAR* FSolidAutoPlaythrough::NameOf(const EStep Step)
{
	switch (Step)
	{
	case EStep::WalkToMonsterEncounter:
		return TEXT("Walk to the monster encounter");
	case EStep::WakeInCave:
		return TEXT("Wake in the cave");
	default:
		return TEXT("");
	}
}

bool FSolidAutoPlaythrough::CanStart(const USolidGameState& State)
{
	return !State.bAutoPlaythroughDim;
}

void FSolidAutoPlaythrough::Start(USolidGameState& State)
{
	State.bAutoPlaythroughDim = true;
	bRunning = true;
	StepIndex = 0;
	ResetWalk();
}

void FSolidAutoPlaythrough::Stop()
{
	bRunning = false;
	ResetWalk();
}

void FSolidAutoPlaythrough::Tick(const float DeltaSeconds, ASolidGameMode& Mode, ASolidCharacter& Captain)
{
	if (!bRunning)
	{
		return;
	}

	switch (CurrentStep())
	{
	case EStep::WalkToMonsterEncounter:
		TickWalk(DeltaSeconds, Mode, Captain);
		break;
	case EStep::WakeInCave:
		TickWake(Mode);
		break;
	default:
		break;
	}
}

FSolidAutoPlaythrough::EStep FSolidAutoPlaythrough::CurrentStep() const
{
	TArray<EStep> Steps;
	CollectSteps(Steps);
	return Steps.IsValidIndex(StepIndex) ? Steps[StepIndex] : EStep::WakeInCave;
}

void FSolidAutoPlaythrough::Advance()
{
	++StepIndex;
	ResetWalk();
	TArray<EStep> Steps;
	CollectSteps(Steps);
	if (!Steps.IsValidIndex(StepIndex))
	{
		Stop();
	}
}

void FSolidAutoPlaythrough::ResetWalk()
{
	bWalkReady = false;
	PointIndex = 0;
	Points.Reset();
	bHasBestDistance = false;
	BestDistanceCm = 0.f;
	StallAccumulated = 0.f;
}

void FSolidAutoPlaythrough::NoteDistance(const float DeltaSeconds, const float DistanceCm)
{
	if (!bHasBestDistance || DistanceCm < BestDistanceCm - ProgressCm)
	{
		BestDistanceCm = DistanceCm;
		bHasBestDistance = true;
		StallAccumulated = 0.f;
		return;
	}

	StallAccumulated += DeltaSeconds;
	if (StallAccumulated >= StallSeconds)
	{
		Stop();
	}
}

bool FSolidAutoPlaythrough::BuildWalk(const ASolidGameMode& Mode, const FVector& CaptainLocation)
{
	UWorld* World = Mode.GetWorld();
	ASolidTerrainStreamer* Streamer = World ? ASolidTerrainStreamer::FindExisting(World) : nullptr;
	USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
	USolidWorldChart* Chart = Terrain ? Terrain->GetWorldMap() : nullptr;
	if (!Chart || !Chart->IsLoaded() || Mode.GetBridges().Num() == 0)
	{
		return false;
	}

	const SolidRoad::FRoute* Road = FindIglinRelionRoad(*Chart);
	if (!Road || Road->Cells.Num() == 0)
	{
		return false;
	}

	const FVector2D WorldMin = Terrain->GetWorldMinXY();
	const FVector2D WorldMax = Terrain->GetWorldMaxXY();
	FVector2D IglinXY = FVector2D::ZeroVector;
	if (!Chart->GetLocationWorldXY(SolidTowns::Iglin, WorldMin, WorldMax, IglinXY))
	{
		return false;
	}

	const FSolidMonster* Monster = ClosestIglinBridgeMonster(Mode, IglinXY);
	FVector2D Approach = FVector2D::ZeroVector;
	if (!Monster || !EncounterApproach(Mode, *Monster, IglinXY, Approach))
	{
		return false;
	}

	TArray<FIntPoint> Cells = Road->Cells;
	if (Road->Definition.From.LocationId != SolidTowns::Iglin)
	{
		Algo::Reverse(Cells);
	}

	const FVector2D Paint = SolidRoad::StripPaintOffset();
	TArray<FVector2D> Centers;
	Centers.Reserve(Cells.Num());
	for (const FIntPoint& Cell : Cells)
	{
		FVector2D Center = FVector2D::ZeroVector;
		if (Chart->CellToWorldXY(Cell, WorldMin, WorldMax, Center))
		{
			Centers.Add(Center + Paint);
		}
	}
	if (Centers.Num() == 0)
	{
		return false;
	}

	int32 LastSafe = INDEX_NONE;
	for (int32 Index = 0; Index < Centers.Num(); ++Index)
	{
		if (OnIglinRamp(Mode, Centers[Index]))
		{
			break;
		}
		if (LastSafe != INDEX_NONE && SegmentOnIglinRamp(Mode, Centers[LastSafe], Centers[Index]))
		{
			break;
		}
		LastSafe = Index;
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Closest = 0;
	int32 Start = 0;
	if (LastSafe != INDEX_NONE)
	{
		float BestApproach = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index <= LastSafe; ++Index)
		{
			const float Dist = FVector2D::DistSquared(Centers[Index], Approach);
			if (Dist < BestApproach)
			{
				BestApproach = Dist;
				Closest = Index;
			}
		}

		float Nearest = TNumericLimits<float>::Max();
		for (int32 Index = 0; Index <= Closest; ++Index)
		{
			const float Dist = FVector2D::DistSquared(Centers[Index], Here);
			if (Dist < Nearest)
			{
				Nearest = Dist;
				Start = Index;
			}
		}
	}

	Points.Reset();
	if (LastSafe != INDEX_NONE)
	{
		Points.Reserve(Closest - Start + 2);
		for (int32 Index = Start; Index <= Closest; ++Index)
		{
			Points.Add(Centers[Index]);
		}
	}

	const FVector2D From = Points.Num() > 0 ? Points.Last() : Here;
	FVector2D Final = Approach;
	ClearOfRamp(Mode, *Monster, From, Approach, Final);
	Points.Add(Final);
	PointIndex = 0;
	bWalkReady = true;
	bHasBestDistance = false;
	StallAccumulated = 0.f;
	return Points.Num() > 0;
}

void FSolidAutoPlaythrough::TickWalk(
	const float DeltaSeconds,
	ASolidGameMode& Mode,
	ASolidCharacter& Captain)
{
	const FVector Location = Captain.GetActorLocation();
	const FVector2D Here(Location.X, Location.Y);
	if (IglinBridgeDetects(Mode, Here))
	{
		Advance();
		return;
	}
	if (!bWalkReady && !BuildWalk(Mode, Location))
	{
		return;
	}
	if (!Points.IsValidIndex(PointIndex))
	{
		Stop();
		return;
	}

	const FVector2D Target = Points[PointIndex];
	const bool bLast = PointIndex + 1 >= Points.Num();
	if (!bLast && SolidClickMove::HasArrived(Here, Target, ArriveCm))
	{
		++PointIndex;
		bHasBestDistance = false;
		StallAccumulated = 0.f;
		return;
	}

	NoteDistance(DeltaSeconds, FVector2D::Distance(Here, Target));
	if (!bRunning)
	{
		return;
	}

	if (Captain.IsTrueSight())
	{
		// The last point sits inside detection. Keep steering until a raptor sees him.
		const float SteerCm = bLast ? 1.f : ArriveCm;
		const FVector2D Direction = SolidClickMove::PlanarDirection(Here, Target, SteerCm);
		if (Direction.IsNearlyZero() || OnIglinRamp(Mode, Here + Direction * RampProbeCm))
		{
			return;
		}
		const FRotator View = Captain.GetControlRotation();
		const float DesiredYaw = FVector(Direction.X, Direction.Y, 0.f).Rotation().Yaw;
		Captain.SimulateMouseLook(
			FMath::FindDeltaAngleDegrees(View.Yaw, DesiredYaw),
			FMath::FindDeltaAngleDegrees(View.Pitch, 0.f));
		Captain.SimulateKey(EKeys::W);
		return;
	}

	if (SegmentOnIglinRamp(Mode, Here, Target))
	{
		return;
	}
	APlayerController* PlayerController = Cast<APlayerController>(Captain.GetController());
	FVector2D Screen = FVector2D::ZeroVector;
	if (PlayerController
		&& PlayerController->ProjectWorldLocationToScreen(FVector(Target.X, Target.Y, Location.Z), Screen))
	{
		Captain.SimulateLeftClick(Screen.X, Screen.Y);
	}
}

void FSolidAutoPlaythrough::TickWake(const ASolidGameMode& Mode)
{
	if (Mode.HasPlacedCaptainInStronghold())
	{
		Stop();
	}
}
