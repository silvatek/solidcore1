#include "SolidAutoPlaythrough.h"

#include "SolidCharacter.h"
#include "SolidClickMove.h"
#include "SolidGameMode.h"
#include "SolidGameState.h"
#include "SolidGroundMove.h"
#include "SolidHUD.h"
#include "Events/SolidMessages.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidRoad.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldChart.h"
#include "Towns/SolidTowns.h"
#include "Algo/Reverse.h"
#include "GameFramework/PlayerController.h"

namespace
{
	constexpr float ProgressCm = 1.f;

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

	/** The ramp whose buried end is closer to Iglin. The point is on that ramp, off the lip. */
	bool IglinRampPoint(const SolidBridge::FBridge& Bridge, const FVector2D& IglinXY, FVector2D& OutPoint)
	{
		const TArray<FVector>* Ramp = nullptr;
		float Best = TNumericLimits<float>::Max();
		auto Consider = [&Ramp, &Best, &IglinXY](const TArray<FVector>& Line)
		{
			if (Line.Num() == 0)
			{
				return;
			}
			const FVector& Buried = Line.Last();
			const float Dist = FVector2D::DistSquared(IglinXY, FVector2D(Buried.X, Buried.Y));
			if (Dist < Best)
			{
				Best = Dist;
				Ramp = &Line;
			}
		};
		Consider(Bridge.RampFrom);
		Consider(Bridge.RampTo);
		if (!Ramp || Ramp->Num() == 0)
		{
			return false;
		}

		const int32 Index = Ramp->Num() == 1
			? 0
			: FMath::Clamp(Ramp->Num() / 2, 1, Ramp->Num() - 1);
		const FVector& Point = (*Ramp)[Index];
		OutPoint = FVector2D(Point.X, Point.Y);
		return true;
	}

	bool OnIglinRamp(const ASolidGameMode& Mode, const FVector& Location)
	{
		for (const TObjectPtr<ASolidBridge>& Actor : Mode.GetBridges())
		{
			const ASolidBridge* Bridge = Actor.Get();
			if (!Bridge || !Bridge->HasBridge() || !Bridge->GetBridge().bIglinRelion)
			{
				continue;
			}
			if (SolidGroundMove::OnRamp(Bridge->GetBridge(), Location.X, Location.Y))
			{
				return true;
			}
		}
		return false;
	}

	ASolidHUD* HudFor(ASolidCharacter& Captain)
	{
		APlayerController* Controller = Cast<APlayerController>(Captain.GetController());
		return Controller ? Cast<ASolidHUD>(Controller->GetHUD()) : nullptr;
	}
}

void FSolidAutoPlaythrough::CollectSteps(TArray<EStep>& OutSteps)
{
	OutSteps.Reset();
	OutSteps.Add(EStep::WalkToIglinRamp);
	OutSteps.Add(EStep::CloseMonsterMessage);
	OutSteps.Add(EStep::WakeInCave);
}

const TCHAR* FSolidAutoPlaythrough::NameOf(const EStep Step)
{
	switch (Step)
	{
	case EStep::WalkToIglinRamp:
		return TEXT("Walk to the Iglin ramp");
	case EStep::CloseMonsterMessage:
		return TEXT("Close the monster message");
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
	case EStep::WalkToIglinRamp:
		TickWalk(DeltaSeconds, Mode, Captain);
		break;
	case EStep::CloseMonsterMessage:
		TickMessage(Captain);
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

	FVector2D RampPoint = FVector2D::ZeroVector;
	bool bRamp = false;
	for (const TObjectPtr<ASolidBridge>& Actor : Mode.GetBridges())
	{
		const ASolidBridge* Bridge = Actor.Get();
		if (!Bridge || !Bridge->HasBridge() || !Bridge->GetBridge().bIglinRelion)
		{
			continue;
		}
		if (IglinRampPoint(Bridge->GetBridge(), IglinXY, RampPoint))
		{
			bRamp = true;
			break;
		}
	}
	if (!bRamp)
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

	int32 Closest = 0;
	float BestRamp = TNumericLimits<float>::Max();
	for (int32 Index = 0; Index < Centers.Num(); ++Index)
	{
		const float Dist = FVector2D::DistSquared(Centers[Index], RampPoint);
		if (Dist < BestRamp)
		{
			BestRamp = Dist;
			Closest = Index;
		}
	}

	const FVector2D Here(CaptainLocation.X, CaptainLocation.Y);
	int32 Start = 0;
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

	Points.Reset();
	Points.Reserve(Closest - Start + 2);
	for (int32 Index = Start; Index <= Closest; ++Index)
	{
		Points.Add(Centers[Index]);
	}
	Points.Add(RampPoint);
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
	if (OnIglinRamp(Mode, Location))
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

	const FVector2D Here(Location.X, Location.Y);
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
		// The last point sits on the ramp. Keep steering until the ramp check ends the step.
		const float SteerCm = bLast ? 1.f : ArriveCm;
		const FVector2D Direction = SolidClickMove::PlanarDirection(Here, Target, SteerCm);
		if (!Direction.IsNearlyZero())
		{
			Captain.AddMovementInput(FVector(Direction.X, Direction.Y, 0.f), 1.f);
		}
		return;
	}

	Captain.SetClickMoveDestination(FVector(Target.X, Target.Y, Location.Z));
}

void FSolidAutoPlaythrough::TickMessage(ASolidCharacter& Captain)
{
	ASolidHUD* HUD = HudFor(Captain);
	if (!HUD || !HUD->IsMessageVisible() || HUD->GetMessageId() != SolidMessages::IdKilledByMonsters)
	{
		return;
	}

	Advance();
	HUD->CloseMessage();
}

void FSolidAutoPlaythrough::TickWake(const ASolidGameMode& Mode)
{
	if (Mode.HasPlacedCaptainInStronghold())
	{
		Stop();
	}
}
