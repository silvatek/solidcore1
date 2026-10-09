#pragma once

#include "SolidTerrainMap.h"
#include "SolidTerrainFog.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * Shared fixtures for SolidCore1.Map.* / Fog map-backed automation tests.
 * Keep identical Build args so Map and MapMore suites stay comparable.
 */
namespace SolidTerrainTestHelpers
{
	/** ~128m half-extent so the grid includes full-fog cells beyond 50m. */
	inline USolidTerrainMap* MakeSmallMap()
	{
		USolidTerrainMap* Map = NewObject<USolidTerrainMap>();
		Map->Build(
			/*InSeed=*/1337,
			/*FrequencyScale=*/0.00012f,
			/*Amplitude=*/3000.f,
			/*BaseHeight=*/0.f,
			/*InGridWidth=*/65,
			/*InGridHeight=*/65,
			/*InPointSpacing=*/200.f,
			/*bForceRebuild=*/true);
		return Map;
	}

	/**
	 * Unit direction from FogOrigin toward the map interior that keeps DistCm
	 * probes inside world bounds (avoids edge clamp after Z-town fog origin moves).
	 */
	inline FVector2D InBoundsFogProbeDir(const USolidTerrainMap* Map, float DistCm)
	{
		const FVector2D Origin = Map->GetFogOriginXY();
		const FVector2D MinXY = Map->GetWorldMinXY();
		const FVector2D MaxXY = Map->GetWorldMaxXY();
		const FVector2D Center = (MinXY + MaxXY) * 0.5f;

		FVector2D Dir = Center - Origin;
		if (Dir.SizeSquared() < 1.f)
		{
			Dir = FVector2D(-1.f, 0.f);
		}
		Dir.Normalize();

		const FVector2D Probe = Origin + Dir * DistCm;
		const bool bInside =
			Probe.X >= MinXY.X && Probe.X <= MaxXY.X
			&& Probe.Y >= MinXY.Y && Probe.Y <= MaxXY.Y;
		if (bInside)
		{
			return Dir;
		}

		const float RoomNegX = Origin.X - MinXY.X;
		const float RoomPosX = MaxXY.X - Origin.X;
		const float RoomNegY = Origin.Y - MinXY.Y;
		const float RoomPosY = MaxXY.Y - Origin.Y;
		const float BestX = FMath::Max(RoomNegX, RoomPosX);
		const float BestY = FMath::Max(RoomNegY, RoomPosY);
		if (BestX >= BestY)
		{
			return (RoomNegX >= RoomPosX) ? FVector2D(-1.f, 0.f) : FVector2D(1.f, 0.f);
		}
		return (RoomNegY >= RoomPosY) ? FVector2D(0.f, -1.f) : FVector2D(0.f, 1.f);
	}

	/**
	 * Corner candidates outside the initial full-fog band around FogOrigin.
	 * Margin is small so opposite corners are >50m apart (trail apply radius).
	 */
	inline void CollectFullFogCandidates(const USolidTerrainMap* Map, TArray<FVector2D>& OutCandidates)
	{
		OutCandidates.Reset();
		if (!Map || !Map->IsBuilt())
		{
			return;
		}

		const FVector2D FogOrigin = Map->GetFogOriginXY();
		const FVector2D MinXY = Map->GetWorldMinXY();
		const FVector2D MaxXY = Map->GetWorldMaxXY();
		// Keep probes in-bounds for a 35m step toward fog origin, but stay near corners
		// so two candidates can be >50m apart on the 128m map.
		constexpr float MarginCm = 800.f;
		constexpr float FullFogCm = SolidTerrainFog::FullFogStartMeters * 100.f + 500.f;

		const FVector2D Corners[] = {
			FVector2D(MinXY.X + MarginCm, MinXY.Y + MarginCm),
			FVector2D(MaxXY.X - MarginCm, MinXY.Y + MarginCm),
			FVector2D(MinXY.X + MarginCm, MaxXY.Y - MarginCm),
			FVector2D(MaxXY.X - MarginCm, MaxXY.Y - MarginCm),
		};

		for (const FVector2D& Candidate : Corners)
		{
			if (FVector2D::Distance(Candidate, FogOrigin) < FullFogCm)
			{
				continue;
			}
			if (FMath::IsNearlyEqual(Map->SamplePoint(Candidate.X, Candidate.Y).Fog, 1.f))
			{
				OutCandidates.Add(Candidate);
			}
		}
	}

	/** Full-fog point farthest from FogOrigin (best trail center). */
	inline bool FindFullyFoggedTrailPoint(const USolidTerrainMap* Map, FVector2D& OutTrailXY)
	{
		TArray<FVector2D> Candidates;
		CollectFullFogCandidates(Map, Candidates);
		if (Candidates.Num() == 0)
		{
			return false;
		}

		const FVector2D FogOrigin = Map->GetFogOriginXY();
		int32 BestIndex = 0;
		float BestDistSq = -1.f;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const float DistSq = FVector2D::DistSquared(Candidates[Index], FogOrigin);
			if (DistSq > BestDistSq)
			{
				BestDistSq = DistSq;
				BestIndex = Index;
			}
		}
		OutTrailXY = Candidates[BestIndex];
		return true;
	}

	/**
	 * Full-fog point farthest from TrailXY and outside the 50m apply radius.
	 */
	inline bool FindUntouchedFullFogPoint(
		const USolidTerrainMap* Map,
		const FVector2D& TrailXY,
		FVector2D& OutUntouchedXY)
	{
		TArray<FVector2D> Candidates;
		CollectFullFogCandidates(Map, Candidates);
		constexpr float ApplyReachCm = SolidTerrainFog::FullFogStartMeters * 100.f + 100.f;

		int32 BestIndex = INDEX_NONE;
		float BestDistSq = -1.f;
		for (int32 Index = 0; Index < Candidates.Num(); ++Index)
		{
			const float DistSq = FVector2D::DistSquared(Candidates[Index], TrailXY);
			if (DistSq <= ApplyReachCm * ApplyReachCm)
			{
				continue;
			}
			if (DistSq > BestDistSq)
			{
				BestDistSq = DistSq;
				BestIndex = Index;
			}
		}
		if (BestIndex == INDEX_NONE)
		{
			return false;
		}
		OutUntouchedXY = Candidates[BestIndex];
		return true;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
