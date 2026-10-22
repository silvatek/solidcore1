#pragma once

#include "CoreMinimal.h"
#include "Combat/SolidArmour.h"
#include "Combat/SolidWeapon.h"

class AActor;
class UWorld;

/**
 * How large a monster is.
 * Small is the Iglin-bridge raptor. Later sizes scale up or down from Small.
 */
enum class ESolidMonsterSize : uint8
{
	Tiny,
	Small,
	Medium,
	Large,
	Huge,
	Immense,
};

namespace SolidMonster
{
	inline constexpr ESolidMonsterSize Sizes[] = {
		ESolidMonsterSize::Tiny,
		ESolidMonsterSize::Small,
		ESolidMonsterSize::Medium,
		ESolidMonsterSize::Large,
		ESolidMonsterSize::Huge,
		ESolidMonsterSize::Immense,
	};

	inline constexpr int32 SizeCount = UE_ARRAY_COUNT(Sizes);

	inline const TCHAR* SizeName(const ESolidMonsterSize Size)
	{
		switch (Size)
		{
		case ESolidMonsterSize::Tiny: return TEXT("Tiny");
		case ESolidMonsterSize::Small: return TEXT("Small");
		case ESolidMonsterSize::Medium: return TEXT("Medium");
		case ESolidMonsterSize::Large: return TEXT("Large");
		case ESolidMonsterSize::Huge: return TEXT("Huge");
		case ESolidMonsterSize::Immense: return TEXT("Immense");
		default: return TEXT("");
		}
	}

	/**
	 * Hidden power for one size. The roll is the one an item uses for quality.
	 * Tiny is the first step. Each larger size raises the middle the way the next grade does.
	 * The largest size narrows the high side. StandardNormal is one draw. A size returns at least 1.
	 */
	int32 PowerFor(ESolidMonsterSize Size, double StandardNormal);
}

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
 * One monster: a place on the ground, a standing size, a weapon, an armour, and the answers that kind is asked.
 * The weapon and the armour are this monster's. They are not items. Raptor is the first subclass.
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
	/** How large this monster is. The Iglin-bridge raptors are Small. */
	ESolidMonsterSize Size = ESolidMonsterSize::Small;
	/** This monster's weapon. Claws and teeth are names. Power is the size roll. */
	FSolidWeapon Weapon;
	/** This monster's armour. Strength is the starting figure. */
	FSolidArmour Armour;

	/**
	 * Stores the weapon and armour this monster keeps.
	 * Power is the size roll for StandardNormal. Speed and Strength are the starting figures.
	 * A kind replaces Strength when its armour is not that starting figure.
	 */
	virtual void Arm(const TCHAR* WeaponName, const TCHAR* ArmourName, double StandardNormal);

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

	/**
	 * The actor for this member, standing on GroundLocation.
	 * The subclass chooses the actor. A null world returns null.
	 */
	virtual AActor* Spawn(UWorld* World, const FVector& GroundLocation) const = 0;

	/** A copy that keeps this subclass. */
	virtual TUniquePtr<FSolidMonster> Clone() const = 0;
};
