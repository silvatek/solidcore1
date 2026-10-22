#pragma once

#include "CoreMinimal.h"
#include "Terrain/SolidBridge.h"
#include "Terrain/SolidTerrainTypes.h"

/**
 * Where the three reptiles stand, and how tall one is.
 * The choice is pure data. The actor is spawned later, and only on the open world.
 */
namespace SolidMonster
{
	/** Standing height of the downloaded mesh after a uniform scale (cm). */
	inline constexpr float StandingHeightCm = 100.f;
	/**
	 * Capsule radius (cm). The same proportion as 60 cm on a 220 cm body, rounded.
	 * It stays under the half height, and the actor origin is the feet.
	 */
	inline constexpr float CapsuleRadiusCm = 27.f;
	/** Half of the standing height. The capsule bottom is the actor origin (cm). */
	inline constexpr float CapsuleHalfHeightCm = StandingHeightCm * 0.5f;
	/** Nearest a point may be to the Iglin ramp's outer end (cm). */
	inline constexpr float NearLimitCm = 800.f;
	/** Farthest a point may be from that outer end (cm). */
	inline constexpr float FarLimitCm = 4000.f;
	/** Later points stay at least this far from one already chosen (cm). */
	inline constexpr float SeparationCm = 400.f;
	/** How many reptiles stand beside the Iglin ramp. */
	inline constexpr int32 Count = 3;

	/**
	 * Marketplace skeletons face the mesh +Y.
	 * This yaw lines that forward up with the actor, which looks at the ramp.
	 */
	inline constexpr float MeshFacingYawDeg = -90.f;

	/** One chosen standing place. Yaw looks at the Iglin ramp's outer end. */
	struct FChoice
	{
		int32 PointIndex = INDEX_NONE;
		FVector2D XY = FVector2D::ZeroVector;
		/** Degrees. Yaw only. */
		float YawDeg = 0.f;
	};

	/** A closed visit has no monsters. The tunnels stay empty. */
	inline bool WorldHasMonsters(const bool bClosedVisit)
	{
		return !bClosedVisit;
	}

	/**
	 * A sample span taller than this is not one body.
	 * The densest band inside this limit sets the height, so far samples do not shrink the mesh.
	 */
	inline constexpr float SpanOutlierCm = 1000.f;

	/** Unscaled bottom and height of the body, in centimetres. */
	struct FStandingSpan
	{
		float BottomZ = 0.f;
		float Height = 0.f;
	};

	/**
	 * Standing span of sample Z values. The samples are sorted.
	 * Fewer than 50 samples, or a span of SpanOutlierCm or less, uses the full range.
	 * A taller span uses the band of that limit which holds the most samples.
	 */
	inline FStandingSpan StandingSpan(TArray<float>& SamplesZ)
	{
		FStandingSpan Span;
		if (SamplesZ.Num() == 0)
		{
			return Span;
		}

		SamplesZ.Sort();
		const int32 Last = SamplesZ.Num() - 1;
		const float FullHeight = SamplesZ[Last] - SamplesZ[0];
		if (FullHeight <= SpanOutlierCm || SamplesZ.Num() < 50)
		{
			Span.BottomZ = SamplesZ[0];
			Span.Height = FullHeight;
			return Span;
		}

		int32 BestLow = 0;
		int32 BestHigh = 0;
		int32 BestCount = 0;
		int32 End = 0;
		for (int32 Start = 0; Start <= Last; ++Start)
		{
			if (End < Start)
			{
				End = Start;
			}
			while (End < Last && SamplesZ[End + 1] - SamplesZ[Start] <= SpanOutlierCm)
			{
				++End;
			}
			const int32 BandCount = End - Start + 1;
			if (BandCount > BestCount)
			{
				BestCount = BandCount;
				BestLow = Start;
				BestHigh = End;
			}
		}

		Span.BottomZ = SamplesZ[BestLow];
		Span.Height = SamplesZ[BestHigh] - SamplesZ[BestLow];
		return Span;
	}

	/** Uniform scale that makes a mesh bounds height the standing height. A zero height stays at 1. */
	inline float UniformScale(const float BoundsHeightCm)
	{
		if (BoundsHeightCm <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}
		return StandingHeightCm / BoundsHeightCm;
	}

	/**
	 * The drawn body is smaller than the vertex span.
	 * The span scale is about 0.10. Three times that scale is knee-high on the captain, about half a metre.
	 * Six times that scale is the Small reptile, the body agreed beside the captain and the 1 m post.
	 * A later size scales up or down from Small.
	 */
	inline constexpr float DrawnBodyFactor = 6.f;

	/**
	 * Relative Z that puts the scaled bounds bottom on the actor origin.
	 * BoundsOriginZ and BoundsExtentZ are the mesh bounds before the scale.
	 */
	inline float MeshFootOffsetCm(const float BoundsOriginZ, const float BoundsExtentZ, const float Scale)
	{
		const float Bottom = BoundsOriginZ - BoundsExtentZ;
		return -Bottom * Scale;
	}

	/**
	 * The nearest grassland points to the Iglin ramp's outer end.
	 * Walkable grassland only, inside the band, clear of trees and foraging piles, and 4 m apart.
	 * A bridge list with no Iglin crossing writes nothing.
	 * @return Number of choices written.
	 */
	int32 CollectChoices(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<int32>& TreePointIndices,
		const TArray<int32>& ForagePointIndices,
		const FVector2D& IglinTownXY,
		const TArray<SolidBridge::FBridge>& Bridges,
		TArray<FChoice>& OutChoices);

	/**
	 * Choice indexes whose fog is now below full, and that have no monster yet.
	 * A point that stays at full fog is never listed.
	 */
	void CollectReadyToSeed(
		const TArray<FSolidTerrainPoint>& Points,
		const TArray<FChoice>& Choices,
		const TArray<int32>& Seeded,
		TArray<int32>& OutReady);
}
