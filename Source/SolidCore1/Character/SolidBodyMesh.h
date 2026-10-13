#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"

class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Builds a low-poly humanoid on the Viking skeleton.
 * The Viking sculpt is not copied. Bone names stay, so Anim_Viking_* still plays.
 * One transient skeleton copy serves every body, so the Viking skeleton asset is not modified.
 * A seed draws every channel across SolidBody::ParamMin .. ParamMax.
 * The same seed rebuilds the same body.
 * The captain stays on SK_Viking for now. Companions wear the generated body.
 */
namespace SolidBodyMesh
{
	inline constexpr uint32 CaptainSeed = 0x0C0A77A1u;

	/** Stable id for a companion name. Case does not matter. */
	uint32 SeedForName(const FString& Name);

	/** Bind-pose role from a bone name. IK helpers stay Root. */
	ESolidBodyBoneRole RoleForBoneName(const FString& BoneName);

	/** Twist bones only spin a limb. They are not a shoulder plate or a shin. */
	bool IsTwistBoneName(const FString& BoneName);

	/** Fingers, thumbs, and toes. The calf and the ankle are not digits. */
	bool IsDigitBoneName(const FString& BoneName);

	/**
	 * Bottom radius of the hip cylinder, in centimetres.
	 * It reaches the outer face of the thigh (hip joint plus the thigh's radius).
	 * A wide pelvis floor can stay wider than that span. The drawn body does not:
	 * it uses SplitHalfWidth so the hip equals the thighs.
	 */
	float HipBottomRadius(float PelvisHalfWidth, float HipLateral, float ThighRadius);

	/**
	 * Skull cross-section as a fraction of head length.
	 * The chin ring is HeadChinSlice of that, and the neck cone ends on it.
	 */
	inline constexpr float HeadRadiusOverHeight = 0.46f;
	inline constexpr float HeadChinSlice = 0.55f;

	/** Joint cross-sections in centimetres when the matching channel is 1. */
	inline constexpr float JointWaist = 12.f;
	inline constexpr float JointChest = 15.f;
	inline constexpr float JointNeck = 8.f;
	inline constexpr float JointThigh = 9.f;
	inline constexpr float JointKnee = 6.4f;
	inline constexpr float JointAnkle = 4.6f;
	inline constexpr float JointShoulder = 5.2f;
	inline constexpr float JointElbow = 4.2f;
	inline constexpr float JointWrist = 3.4f;

	/** One radius per joint, before a split widens the pelvis or the shoulder line. */
	struct FSolidConeProfile
	{
		float Waist = 0.f;
		float Chest = 0.f;
		float Neck = 0.f;
		float Jaw = 0.f;
		float Thigh = 0.f;
		float Knee = 0.f;
		float Ankle = 0.f;
		float Shoulder = 0.f;
		float Elbow = 0.f;
		float Wrist = 0.f;
	};

	/** Drawn head length in centimetres. HeadSize 1 is one head. */
	float HeadHeightCm(const FSolidBodyParams& Params);

	/** Radius of the skull's bottom ring, where the neck cone ends. */
	float HeadChinRadius(const FSolidBodyParams& Params);

	/** Limb and torso joint radii for these proportions. The jaw matches the chin ring. */
	FSolidConeProfile ConeProfile(const FSolidBodyParams& Params);

	/**
	 * Half the width of one cone where it meets two equal cones.
	 * ChildLateral is the child's axis out from the parent's axis.
	 * The parent's diameter is then the outer span of both children.
	 */
	float SplitHalfWidth(float ChildLateral, float ChildRadius);

	/**
	 * Spine joint along the waist-to-chest chain.
	 * Index 0 is the waist and LastIndex is the chest. One spine is the chest.
	 */
	float SpineJointRadius(float Waist, float Chest, int32 Index, int32 LastIndex);

	/**
	 * One radius per joint, in centimetres.
	 * The cone from a parent to a child uses the parent's radius, then the child's,
	 * so adjoining ends match. The pelvis matches both thighs. The chest matches
	 * both arms. The neck keeps its own radius and ends on the chin.
	 */
	void JointRadii(
		const TArray<FSolidDeformBone>& Bones,
		const TArray<FVector>& Positions,
		const FSolidBodyParams& Params,
		TArray<float>& OutRadii);

	/**
	 * Bones that share a key are one mesh piece.
	 * The head includes the eyes. Each shoulder includes the clavicle, the upper arm,
	 * and the shoulder joints. spine_01, spine_02, and spine_03 are separate pieces.
	 * Empty when the bone is not a body piece.
	 */
	FString SegmentKey(const FString& BoneName);

	/** A truncated cone that contains a set of points and stops at their span. */
	struct FSolidFittedCone
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		float RadiusStart = 0.f;
		float RadiusEnd = 0.f;
		bool bValid = false;
	};

	/** Round cone around Points. A mirrored point set produces a mirrored cone. */
	FSolidFittedCone FitCone(const FVector* Points, int32 Count);

	/** The left thigh is this bone. Its length is the distance from its parent joint. */
	inline const TCHAR* LeftThighBone = TEXT("thigh_twist_01_l");

	/** Diameters of the left thigh cone, in centimetres. The hip end is the wide end. */
	inline constexpr float LeftThighTopDiameterCm = 20.f;
	inline constexpr float LeftThighBottomDiameterCm = 15.f;

	/**
	 * Truncated cone for the left thigh.
	 * Hip is the parent joint and FarEnd is the bone. The wide end is at the hip.
	 * Invalid when that bone is shorter than a centimetre.
	 */
	FSolidFittedCone LeftThighCone(const FVector& Hip, const FVector& FarEnd);

	/** Start and end radii of the cone from Parent to Child. Both ends are joint radii. */
	void SegmentRadii(
		const TArray<float>& Radii,
		int32 Parent,
		int32 Child,
		float& OutStart,
		float& OutEnd);

	/**
	 * Transient low-poly mesh owned by Outer.
	 * Source supplies the Viking skeleton and bind pose. Its mesh and skeleton stay unchanged.
	 * Null when Source has no skeleton, or the body has no wearable LOD.
	 */
	USkeletalMesh* Build(const USkeletalMesh* Source, const FSolidBodyParams& Params, UObject* Outer);

	/**
	 * Replace the component mesh with a low-poly body.
	 * Shirt, trouser, and skin colors are plain matte materials. The same proportions reuse one mesh.
	 * Seed is the name id written to the log. Params is the body that is built.
	 * Leaves the current mesh when the body cannot be built.
	 * The captain does not call this.
	 */
	void ApplySeed(
		USkeletalMeshComponent* MeshComp,
		uint32 Seed,
		const FSolidBodyParams& Params,
		UObject* Outer,
		const TCHAR* DebugLabel,
		const FLinearColor& TorsoColor,
		const FLinearColor& LegColor,
		const FLinearColor& SkinColor);

	/** Stable face glyph for a companion name. One of a few emoji code points, including U+1F611. */
	uint32 FaceCodepointForName(const FString& Name);

	/** UTF-16 text for a Unicode code point, including characters above U+FFFF. */
	FString FaceStringForCodepoint(uint32 Codepoint);
}
