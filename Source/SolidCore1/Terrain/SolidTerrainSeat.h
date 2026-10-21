#pragma once

#include "CoreMinimal.h"

class AActor;
class USolidTerrainMap;
class UStaticMeshComponent;

/**
 * The buried part of an object that sits on open-world terrain.
 * The bottom ends 10 cm under the lowest terrain sample in the footprint.
 * The mesh above the ground stays where it is.
 * A bridge ramp is not a seat. A captain and a companion are not seats.
 */
namespace SolidTerrainSeat
{
	/** How far the buried bottom sits under the lowest terrain sample. */
	inline constexpr float BurialCm = 10.f;

	/**
	 * Ground contact in the actor's local XY, in centimetres.
	 * A round footprint uses HalfXCm as its radius.
	 */
	struct FFootprint
	{
		FVector2D LocalCenter = FVector2D::ZeroVector;
		float HalfXCm = 0.f;
		float HalfYCm = 0.f;
		bool bRound = false;
	};

	/** Step between samples along an edge or a rim. */
	inline float SampleStepCm(const float PointSpacing)
	{
		return FMath::Max(PointSpacing * 0.5f, 1.f);
	}

	/** Rim samples on a round footprint. Zero when the radius is a point. */
	inline int32 RoundSampleCount(const float RadiusCm, const float PointSpacing)
	{
		const float Radius = FMath::Max(RadiusCm, 0.f);
		if (Radius <= KINDA_SMALL_NUMBER)
		{
			return 0;
		}
		return FMath::Max(4, FMath::CeilToInt(2.f * PI * Radius / SampleStepCm(PointSpacing)));
	}

	/** True when the world XY lies in the footprint. */
	bool Contains(
		const FVector& ActorLocation,
		float YawDeg,
		const FFootprint& Footprint,
		float WorldX,
		float WorldY);

	/**
	 * Lowest terrain height under the footprint, in centimetres.
	 * Zero when the terrain is not built.
	 */
	float LowestCm(
		const USolidTerrainMap* Terrain,
		const FVector& ActorLocation,
		float YawDeg,
		const FFootprint& Footprint);

	/**
	 * How far below the actor origin the ground mesh must reach.
	 * Zero when the terrain is missing, or when the origin is already below that depth.
	 */
	float DropCm(
		const USolidTerrainMap* Terrain,
		float SurfaceBias,
		const FVector& ActorLocation,
		float YawDeg,
		const FFootprint& Footprint);

	/** DropCm for an actor that has a terrain streamer. Zero otherwise. */
	float DropCm(const AActor* Actor, const FFootprint& Footprint);

	/**
	 * Lengthen a center-pivoted mesh downward.
	 * BottomZ and TopZ are the part's local extents before this call.
	 * The top stays. ShapeHeightCm is the unscaled mesh height.
	 */
	void BuryCentered(
		UStaticMeshComponent* Part,
		float BottomZ,
		float TopZ,
		float DropCm,
		float ShapeHeightCm);
}
