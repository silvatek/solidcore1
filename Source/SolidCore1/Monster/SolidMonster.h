#pragma once

#include "CoreMinimal.h"

/**
 * Unscaled bottom and height of a body, in centimetres.
 * StandingSpan fills these from mesh or bone samples.
 */
struct FSolidMonsterStandingSpan
{
	float BottomZ = 0.f;
	float Height = 0.f;
};

/**
 * One monster: a place on the ground, a standing size, and the answers that kind is asked.
 * Raptor is the first subclass. Later kinds bring their own mesh, skeleton, and clips.
 */
class FSolidMonster
{
public:
	FSolidMonster() = default;
	virtual ~FSolidMonster() = default;
	FSolidMonster(const FSolidMonster&) = default;
	FSolidMonster& operator=(const FSolidMonster&) = default;

	/** Terrain point this monster stands on. INDEX_NONE until placed. */
	int32 PointIndex = INDEX_NONE;
	/** World centimetres of that point. Height is the ground under the actor. */
	FVector2D XY = FVector2D::ZeroVector;
	/** Degrees. Yaw only. Faces the place the group chose. */
	float YawDeg = 0.f;

	/** Standing height of the body after a uniform scale (cm). */
	static constexpr float StandingHeightCm = 100.f;
	/**
	 * Capsule radius (cm). The same proportion as 60 cm on a 220 cm body, rounded.
	 * It stays under the half height, and the actor origin is the feet.
	 */
	static constexpr float CapsuleRadiusCm = 27.f;
	/** Half of the standing height. The capsule bottom is the actor origin (cm). */
	static constexpr float CapsuleHalfHeightCm = StandingHeightCm * 0.5f;
	/**
	 * Marketplace skeletons face the mesh +Y.
	 * This yaw lines that forward up with the actor.
	 */
	static constexpr float MeshFacingYawDeg = -90.f;
	/**
	 * A sample span taller than this is not one body.
	 * The densest band inside this limit sets the height, so far samples do not shrink the mesh.
	 */
	static constexpr float SpanOutlierCm = 1000.f;

	/**
	 * Standing span of sample Z values. The samples are sorted.
	 * Fewer than 50 samples, or a span of SpanOutlierCm or less, uses the full range.
	 * A taller span uses the band of that limit which holds the most samples.
	 */
	static FSolidMonsterStandingSpan StandingSpan(TArray<float>& SamplesZ);

	/** Uniform scale that makes a mesh height the standing height. A zero height stays at 1. */
	static float UniformScale(float BoundsHeightCm);

	/**
	 * Relative Z that puts the scaled bounds bottom on the actor origin.
	 * BoundsOriginZ and BoundsExtentZ are the mesh bounds before the scale.
	 */
	static float MeshFootOffsetCm(float BoundsOriginZ, float BoundsExtentZ, float Scale);

	/**
	 * How much larger than the vertex-span scale the drawn body is.
	 * A kind that uses the downloaded mesh as-is returns 1.
	 */
	virtual float DrawnBodyFactor() const { return 1.f; }

	/** The kind name, such as "Raptor". */
	virtual const TCHAR* Kind() const = 0;

	/** A copy that keeps this subclass. */
	virtual TUniquePtr<FSolidMonster> Clone() const = 0;
};
