#include "SolidGroundMove.h"

#include "SolidGameMode.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidRoad.h"
#include "Terrain/SolidTerrainMap.h"
#include "Terrain/SolidTerrainStreamer.h"
#include "Terrain/SolidWorldMap.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	float DistanceToLine(const TArray<FVector>& Line, const FVector2D& Point)
	{
		if (Line.Num() == 0)
		{
			return TNumericLimits<float>::Max();
		}
		if (Line.Num() == 1)
		{
			return FVector2D::Distance(Point, FVector2D(Line[0].X, Line[0].Y));
		}

		float Best = TNumericLimits<float>::Max();
		for (int32 Index = 1; Index < Line.Num(); ++Index)
		{
			const FVector2D A(Line[Index - 1].X, Line[Index - 1].Y);
			const FVector2D B(Line[Index].X, Line[Index].Y);
			Best = FMath::Min(Best, SolidRoad::DistanceToSegmentCm(Point, A, B));
		}
		return Best;
	}

	struct FOpenWorld
	{
		const USolidWorldMap* Map = nullptr;
		FVector2D Min = FVector2D::ZeroVector;
		FVector2D Max = FVector2D::ZeroVector;
		TArray<SolidBridge::FBridge> Bridges;
	};

	bool GatherOpenWorld(UWorld* World, FOpenWorld& Out)
	{
		if (!World)
		{
			return false;
		}

		if (const ASolidGameMode* GameMode = World->GetAuthGameMode<ASolidGameMode>())
		{
			if (GameMode->IsInDungeon())
			{
				return false;
			}
			for (const TObjectPtr<ASolidBridge>& Actor : GameMode->GetBridges())
			{
				const ASolidBridge* Bridge = Actor.Get();
				if (Bridge && Bridge->HasBridge())
				{
					Out.Bridges.Add(Bridge->GetBridge());
				}
			}
		}

		ASolidTerrainStreamer* Streamer = ASolidTerrainStreamer::FindExisting(World);
		const USolidTerrainMap* Terrain = Streamer ? Streamer->GetTerrainMap() : nullptr;
		const USolidWorldMap* Map = Terrain ? Terrain->GetWorldMap() : nullptr;
		if (!Terrain || !Terrain->IsBuilt() || !Map || !Map->IsLoaded())
		{
			return false;
		}

		Out.Map = Map;
		Out.Min = Terrain->GetWorldMinXY();
		Out.Max = Terrain->GetWorldMaxXY();
		return true;
	}

	/**
	 * Ramp[0] is the lip at the deck. When bHoldAtLip is set, ground past that lip
	 * (toward the missing span) is not part of the ramp.
	 */
	bool CoversRamp(const TArray<FVector>& Line, const FVector2D& Point, const float Half, const bool bHoldAtLip)
	{
		if (Line.Num() == 0)
		{
			return false;
		}
		if (Line.Num() == 1)
		{
			return !bHoldAtLip && FVector2D::Distance(Point, FVector2D(Line[0].X, Line[0].Y)) <= Half;
		}

		for (int32 Index = 1; Index < Line.Num(); ++Index)
		{
			const FVector2D A(Line[Index - 1].X, Line[Index - 1].Y);
			const FVector2D B(Line[Index].X, Line[Index].Y);
			if (bHoldAtLip && Index == 1)
			{
				const FVector2D Span = B - A;
				const float Denom = Span.SizeSquared();
				if (Denom <= KINDA_SMALL_NUMBER)
				{
					continue;
				}
				const float Along = FVector2D::DotProduct(Point - A, Span) / Denom;
				if (Along < 0.f)
				{
					continue;
				}
			}
			if (SolidRoad::DistanceToSegmentCm(Point, A, B) <= Half)
			{
				return true;
			}
		}
		return false;
	}

	/** The deck corridor of a broken bridge, except the ramp surfaces that still meet the lip. */
	bool ClosesBrokenSpan(const SolidBridge::FBridge& Bridge, const FVector2D& Point)
	{
		if (!Bridge.bBroken)
		{
			return false;
		}
		const float Half = Bridge.WidthCm * 0.5f;
		if (Half <= 0.f)
		{
			return false;
		}
		return DistanceToLine(Bridge.Deck, Point) <= Half
			&& !CoversRamp(Bridge.RampFrom, Point, Half, true)
			&& !CoversRamp(Bridge.RampTo, Point, Half, true);
	}
}

bool SolidGroundMove::IsImpassableGround(
	const USolidWorldMap& Map,
	const float X,
	const float Y,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY)
{
	return IsImpassable(Map.SampleBiome(X, Y, WorldMinXY, WorldMaxXY));
}

bool SolidGroundMove::BridgeOpens(const SolidBridge::FBridge& Bridge, const float X, const float Y)
{
	const float Half = Bridge.WidthCm * 0.5f;
	if (Half <= 0.f)
	{
		return false;
	}
	const FVector2D Point(X, Y);
	const bool bDeck = !Bridge.bBroken && DistanceToLine(Bridge.Deck, Point) <= Half;
	return bDeck
		|| CoversRamp(Bridge.RampFrom, Point, Half, Bridge.bBroken)
		|| CoversRamp(Bridge.RampTo, Point, Half, Bridge.bBroken);
}

bool SolidGroundMove::OnRamp(const SolidBridge::FBridge& Bridge, const float X, const float Y)
{
	const float Half = Bridge.WidthCm * 0.5f;
	if (Half <= 0.f)
	{
		return false;
	}
	const FVector2D Point(X, Y);
	return CoversRamp(Bridge.RampFrom, Point, Half, true)
		|| CoversRamp(Bridge.RampTo, Point, Half, true);
}

bool SolidGroundMove::AllowsFooting(
	const USolidWorldMap& Map,
	const float X,
	const float Y,
	const FVector2D WorldMinXY,
	const FVector2D WorldMaxXY,
	const TArray<SolidBridge::FBridge>& Bridges)
{
	const FVector2D Point(X, Y);
	for (const SolidBridge::FBridge& Bridge : Bridges)
	{
		if (ClosesBrokenSpan(Bridge, Point))
		{
			return false;
		}
	}
	if (!IsImpassableGround(Map, X, Y, WorldMinXY, WorldMaxXY))
	{
		return true;
	}
	if (Map.SampleBiome(X, Y, WorldMinXY, WorldMaxXY) != ESolidBiome::River)
	{
		return false;
	}
	for (const SolidBridge::FBridge& Bridge : Bridges)
	{
		if (BridgeOpens(Bridge, X, Y))
		{
			return true;
		}
	}
	return false;
}

FVector2D SolidGroundMove::ClampPlanar(
	const FVector2D& From,
	const FVector2D& Desired,
	const float ProbeCm,
	TFunctionRef<bool(float X, float Y)> Allows)
{
	if (Desired.IsNearlyZero() || ProbeCm <= 0.f)
	{
		return FVector2D::ZeroVector;
	}

	auto StepOpens = [&Allows, &From, ProbeCm](const FVector2D& Step)
	{
		if (Step.IsNearlyZero())
		{
			return false;
		}
		const FVector2D Direction = Step.GetSafeNormal();
		return Allows(From.X + Direction.X * ProbeCm, From.Y + Direction.Y * ProbeCm);
	};

	if (StepOpens(Desired))
	{
		return Desired;
	}

	const FVector2D AlongX(Desired.X, 0.f);
	const FVector2D AlongY(0.f, Desired.Y);
	const bool bX = StepOpens(AlongX);
	const bool bY = StepOpens(AlongY);
	if (bX && bY)
	{
		// Each axis is open and the diagonal is not, so this is a corner.
		return FMath::Abs(Desired.X) >= FMath::Abs(Desired.Y) ? AlongX : AlongY;
	}
	if (bX)
	{
		return AlongX;
	}
	if (bY)
	{
		return AlongY;
	}
	return FVector2D::ZeroVector;
}

bool SolidGroundMove::AllowsStep(const ACharacter* Character, const FVector2D& Desired)
{
	if (!Character || Desired.IsNearlyZero())
	{
		return false;
	}

	UWorld* World = Character->GetWorld();
	FOpenWorld Ground;
	if (!GatherOpenWorld(World, Ground) || !Ground.Map)
	{
		return true;
	}

	const USolidWorldMap& Map = *Ground.Map;
	const FVector Location = Character->GetActorLocation();
	const float Probe = Character->GetCapsuleComponent()
		? Character->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 42.f;
	const FVector2D Clamped = ClampPlanar(
		FVector2D(Location.X, Location.Y),
		Desired,
		Probe,
		[&Map, &Ground](const float X, const float Y)
		{
			return AllowsFooting(Map, X, Y, Ground.Min, Ground.Max, Ground.Bridges);
		});
	if (Clamped.IsNearlyZero())
	{
		return false;
	}
	return FVector2D::DotProduct(Clamped.GetSafeNormal(), Desired.GetSafeNormal()) > 0.99f;
}

void SolidGroundMove::Hold(ACharacter* Character)
{
	if (!Character)
	{
		return;
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	UWorld* World = Character->GetWorld();
	FOpenWorld Ground;
	if (!GatherOpenWorld(World, Ground) || !Ground.Map)
	{
		return;
	}

	const USolidWorldMap& Map = *Ground.Map;
	FVector Location = Character->GetActorLocation();
	const float Probe = Character->GetCapsuleComponent()
		? Character->GetCapsuleComponent()->GetScaledCapsuleRadius()
		: 42.f;
	auto Allows = [&Map, &Ground](const float X, const float Y)
	{
		return AllowsFooting(Map, X, Y, Ground.Min, Ground.Max, Ground.Bridges);
	};

	// The movement tick can step onto the channel before this runs. Put the
	// pawn back on the last open ground, or a push into the bank only creeps.
	struct FBank
	{
		FVector Location = FVector::ZeroVector;
		bool bSet = false;
	};
	static TMap<TWeakObjectPtr<ACharacter>, FBank> Banks;
	for (auto It = Banks.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	FBank& Bank = Banks.FindOrAdd(Character);
	if (Allows(Location.X, Location.Y))
	{
		Bank.Location = Location;
		Bank.bSet = true;
	}
	else if (Bank.bSet)
	{
		Character->SetActorLocation(Bank.Location, false, nullptr, ETeleportType::TeleportPhysics);
		Move->StopMovementImmediately();
		Location = Bank.Location;
	}

	const FVector2D From(Location.X, Location.Y);
	const FVector Pending = Character->ConsumeMovementInputVector();
	const FVector2D ClampedInput = ClampPlanar(From, FVector2D(Pending.X, Pending.Y), Probe, Allows);
	if (!ClampedInput.IsNearlyZero())
	{
		Character->AddMovementInput(
			FVector(ClampedInput.X, ClampedInput.Y, 0.f).GetSafeNormal(),
			ClampedInput.Size());
	}

	const FVector Velocity = Move->Velocity;
	const FVector2D ClampedVelocity = ClampPlanar(From, FVector2D(Velocity.X, Velocity.Y), Probe, Allows);
	Move->Velocity = FVector(ClampedVelocity.X, ClampedVelocity.Y, Velocity.Z);
}
