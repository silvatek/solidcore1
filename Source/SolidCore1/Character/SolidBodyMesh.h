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

	/** A tube along a bone. Depth scales squash the section across WideAxis, and may differ at each end. */
	struct FSolidFittedCone
	{
		FVector Start = FVector::ZeroVector;
		FVector End = FVector::ZeroVector;
		float RadiusStart = 0.f;
		float RadiusEnd = 0.f;
		bool bValid = false;
		/** 1 keeps a round section. Smaller flattens the axis perpendicular to WideAxis. */
		float DepthScaleStart = 1.f;
		float DepthScaleEnd = 1.f;
		/** Direction that keeps the full diameter. Side-to-side for the hip. */
		FVector WideAxis = FVector::RightVector;
	};

	/** Round cone around Points. A mirrored point set produces a mirrored cone. */
	FSolidFittedCone FitCone(const FVector* Points, int32 Count);

	/** The only bone in the left thigh piece. calf_l runs from the hip to the knee. */
	inline const TCHAR* LeftThighBone = TEXT("calf_l");

	/** The only bone in the right thigh piece. The cone matches the left thigh. */
	inline const TCHAR* RightThighBone = TEXT("calf_r");

	/** Diameters of a thigh cone, in centimetres. The hip is the wide end. */
	inline constexpr float LeftThighTopDiameterCm = 20.f;
	inline constexpr float LeftThighBottomDiameterCm = 15.f;

	/**
	 * Truncated cone the length of calf_l or calf_r.
	 * That length is the distance from the hip to the knee, so Hip is the wide end and Knee is the bone.
	 * The piece does not include the parent bone. Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone LeftThighCone(const FVector& Hip, const FVector& Knee);

	/** The only bone in the left calf piece. foot_l runs from the knee to the ankle. */
	inline const TCHAR* LeftCalfBone = TEXT("foot_l");

	/** The only bone in the right calf piece. The cone matches the left calf. */
	inline const TCHAR* RightCalfBone = TEXT("foot_r");

	/** Diameters of a calf cone, in centimetres. The knee is the wide end. */
	inline constexpr float CalfTopDiameterCm = LeftThighBottomDiameterCm;
	inline constexpr float CalfBottomDiameterCm = 12.f;

	/**
	 * Truncated cone the length of foot_l or foot_r.
	 * Knee is the parent joint and Ankle is the bone. The wide end is the knee.
	 * The piece does not include the parent bone. Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone CalfCone(const FVector& Knee, const FVector& Ankle);

	/** The only bone in the left foot cylinder. ball_l runs from the ankle to the toe. */
	inline const TCHAR* LeftBallBone = TEXT("ball_l");

	/** The only bone in the right foot cylinder. The cylinder matches the left foot. */
	inline const TCHAR* RightBallBone = TEXT("ball_r");

	/** Diameter of a foot cylinder, in centimetres. Both ends are the same. */
	inline constexpr float BallDiameterCm = 10.f;

	/**
	 * Cylinder the length of ball_l or ball_r.
	 * Ankle is the parent joint and Toe is the bone. Both ends are BallDiameterCm across.
	 * The piece does not include the parent bone. Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone BallCylinder(const FVector& Ankle, const FVector& Toe);

	/** The only bone in the left toe cone. extra_finger_l continues past ball_l. */
	inline const TCHAR* LeftExtraFingerBone = TEXT("extra_finger_l");

	/** The only bone in the right toe cone. The cone matches the left toe. */
	inline const TCHAR* RightExtraFingerBone = TEXT("extra_finger_r");

	/** Diameters of a toe cone, in centimetres. The end towards the ankle is the wide end. */
	inline constexpr float ExtraFingerAnkleDiameterCm = BallDiameterCm;
	inline constexpr float ExtraFingerToeDiameterCm = 5.f;

	/**
	 * Truncated cone the length of extra_finger_l or extra_finger_r.
	 * AnkleEnd is the parent joint and ToeEnd is the bone. The wide end is towards the ankle.
	 * The piece does not include the parent bone. Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone ExtraFingerCone(const FVector& AnkleEnd, const FVector& ToeEnd);

	/** Shirt material slot. Trousers are the next slot, then skin. */
	inline constexpr uint8 ShirtMaterialSlot = 0;
	inline constexpr uint8 TrouserMaterialSlot = 1;

	/** The three bones named by the hip piece. The mesh itself is the length of spine_01. */
	inline const TCHAR* HipLeftBone = TEXT("thigh_l");
	inline const TCHAR* HipRightBone = TEXT("thigh_r");
	inline const TCHAR* HipSpineBone = TEXT("spine_01");

	/** Hip section, in centimetres. The bottom is wider from side to side. Front-to-back stays the same. */
	inline constexpr float HipSideDiameterCm = 50.f;
	inline constexpr float HipTopSideDiameterCm = 30.f;
	inline constexpr float HipDepthDiameterCm = 20.f;

	/**
	 * Shirt piece the length of spine_01. The parent joint is the bottom: 50 cm side to side and 20 cm front to back.
	 * spine_01 is the top: 30 cm side to side and 20 cm front to back. The full width is the mesh forward axis.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone HipCone(const FVector& SpineStart, const FVector& SpineEnd);

	/**
	 * True when the ray meets the cone closer than InOutRayT.
	 * A depth scale below 1 uses an oval section, full across WideAxis and squashed across it.
	 * The two scales are the start and the end of the piece.
	 * InOutRayT is the closest hit so far, and becomes this hit when the ray meets the cone.
	 */
	bool RayHitsCone(
		const FSolidFittedCone& Cone,
		const FVector& RayOrigin,
		const FVector& RayDir,
		float& InOutRayT);

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
