#pragma once

#include "SolidMonster.h"

class AActor;
class UAnimSequence;
class UMaterialInterface;
class USkeletalMesh;
class USkeletalMeshComponent;
class UWorld;

/**
 * A raptor: the green Velociraptor body beside the Iglin bridge for now.
 * Later sizes scale from Small. A later body builds on this skeleton the way a human mesh is built.
 */
class FSolidRaptor : public FSolidMonster
{
public:
	FSolidRaptor() = default;

	/**
	 * The drawn body is smaller than the vertex span.
	 * The span scale is about 0.10. Three times that scale is knee-high on the captain, about half a metre.
	 * Six times that scale is the Small reptile, the body agreed beside the captain and the 1 m post.
	 * A later size scales up or down from Small.
	 */
	static constexpr float SmallDrawnBodyFactor = 6.f;

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
