#pragma once

#include "SolidMonster.h"

class AActor;
class UAnimSequence;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UWorld;

/**
 * A raptor: the green Velociraptor.
 * Small stands beside the Iglin bridge. Immense is that same body at eight times the height.
 * The sizes between still draw as Small. A later body builds on this skeleton the way a human mesh is built.
 */
class FSolidRaptor : public FSolidMonster
{
public:
	FSolidRaptor() = default;

	/**
	 * The drawn body is smaller than the vertex span.
	 * The span scale is about 0.10. Three times that scale is knee-high on the captain, about half a metre.
	 * Six times that scale is the Small reptile, the body agreed beside the captain and the 1 m post.
	 * Immense is eight times that body, so the drawn height is 8 m. The sizes between still draw as Small.
	 */
	static constexpr float SmallDrawnBodyFactor = 6.f;
	/** Eight times SmallDrawnBodyFactor. The drawn Immense body is 8 m. */
	static constexpr float ImmenseDrawnBodyFactor = 48.f;
	/** This raptor's weapon. */
	static constexpr const TCHAR* Claws = TEXT("Claws");
	/** This raptor's armour. */
	static constexpr const TCHAR* Hide = TEXT("Hide");
	/** Hide Strength for Small and every size that still draws as Small. Play-testing chose 20. */
	static constexpr float HideStrength = 20.f;
	/** Hide Strength for an Immense raptor. */
	static constexpr float ImmenseHideStrength = 500.f;

	/** Stores claws and hide. Immense uses ImmenseHideStrength. Every other size uses HideStrength. */
	virtual void Arm(const TCHAR* WeaponName, const TCHAR* ArmourName, double StandardNormal) override;

	virtual float DrawnBodyFactor() const override;
	virtual const TCHAR* Kind() const override { return TEXT("Raptor"); }
	virtual AActor* Spawn(UWorld* World, const FVector& GroundLocation) const override;

	/** Uniform standing scale times DrawnBodyFactor. */
	float BodyScale(float SpanHeightCm) const;

	/** The green Velociraptor skeletal mesh from the Fab pack. Null when the pack is missing. */
	USkeletalMesh* FindMesh() const;

	/** The green skin for that mesh. Null when the pack is missing. */
	UMaterialInterface* FindGreen(USkeletalMesh* Mesh) const;

	/** The in-place idle clip for that mesh. Null when the pack is missing. */
	UAnimSequence* FindIdle(USkeletalMesh* Mesh) const;

	/**
	 * Mesh, scale, green skin, and looping idle on the body component.
	 * Leaves the capsule alone. Logs when the mesh is missing.
	 */
	void ApplyVisuals(USkeletalMeshComponent* Body, TObjectPtr<UAnimSequence>& ActiveIdle) const;

	virtual TUniquePtr<FSolidMonster> Clone() const override;
};
