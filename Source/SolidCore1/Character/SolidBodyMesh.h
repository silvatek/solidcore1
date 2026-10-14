#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"
#include "SolidBodyPart.h"

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
	/** The character. Parts holds every body part. Draw asks each one to add its triangles. */
	class FSolidBodyMesh
	{
	public:
		TArray<FSolidBodyPart> Parts;

		/**
		 * The body.
		 * RisingSpineBones are the spine bones above the hips, parent to child.
		 * An empty list leaves out the abdomen and the chest. The shoulder base then stays the chest width.
		 */
		static FSolidBodyMesh Make(const TArray<FName>& RisingSpineBones);

		/** Make the body for this skeleton. The rising spine is read from the hip upward. */
		static FSolidBodyMesh FromBones(const TArray<FSolidDeformBone>& Bones);

		const FSolidBodyPart* Find(const FString& Name) const;

		/** Draw every part whose bones are on this skeleton. */
		void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			const TArray<FSolidDeformBone>& Bones,
			const TArray<FVector>& Positions);
	};

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
	inline constexpr uint8 SkinMaterialSlot = 2;
	static_assert(ShirtMaterialSlot == 0 && TrouserMaterialSlot == 1 && SkinMaterialSlot == 2, "body material slots stay in order");

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

	/** A spine bone above the hips. spine_01 is the hip piece. A twist is not a slab. */
	bool IsRisingSpineBoneName(const FString& BoneName);

	/** clavicle_l and clavicle_r. A twist is not a clavicle, and neither is shoulder_jnt. */
	bool IsClavicleBoneName(const FString& BoneName);

	/** upperarm_l and upperarm_r. A twist is not the upper arm. */
	bool IsUpperArmBoneName(const FString& BoneName);

	/** lowerarm_l and lowerarm_r. A twist is not the forearm. */
	bool IsForearmBoneName(const FString& BoneName);

	/** hand_l and hand_r. A finger is not the hand, and neither is an IK hand. */
	bool IsHandBoneName(const FString& BoneName);

	/**
	 * Side-to-side width at the top of the spine, in centimetres.
	 * That is an average shoulder breadth: wider than the 30 cm waist, narrower than the 50 cm hips.
	 */
	inline constexpr float ChestSideDiameterCm = 40.f;

	/** Front-to-back of every spine piece above the hips. It matches the hip. */
	inline constexpr float TorsoDepthDiameterCm = HipDepthDiameterCm;

	/**
	 * Side-to-side diameters of one spine piece above spine_01, in centimetres.
	 * Step 0 is the first of those bones. Count is how many there are.
	 * The chain starts at the hip top and reaches the chest on the last bone.
	 * A count below 1 leaves both ends at the hip top.
	 */
	void TorsoSideDiameters(int32 Step, int32 Count, float& OutBottomCm, float& OutTopCm);

	/**
	 * Shirt piece the length of one spine bone above spine_01.
	 * Bottom is the parent joint and Top is the bone. SideBottomCm and SideTopCm are the full widths.
	 * Front-to-back stays TorsoDepthDiameterCm. The full width is the mesh forward axis.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone TorsoCone(
		const FVector& Bottom,
		const FVector& Top,
		float SideBottomCm,
		float SideTopCm);

	/**
	 * Clavicle where it leaves the chest, in centimetres.
	 * Thicker than the arm and thinner than the chest. The outer end matches the upper arm.
	 */
	inline constexpr float ClavicleRootDiameterCm = 14.f;

	/** Upper-arm diameters, in centimetres. The parent joint is the wide end. */
	inline constexpr float UpperArmShoulderDiameterCm = JointShoulder * 2.f;
	inline constexpr float UpperArmElbowDiameterCm = JointElbow * 2.f;

	/**
	 * Shared arm diameter, in centimetres.
	 * Narrow end of the upper arm, at the elbow. Wide end of the lower arm, at the wrist.
	 */
	inline constexpr float ForearmWristDiameterCm = JointWrist * 2.f;

	/**
	 * Lower-arm cone, in centimetres. The part is "Lower arm" and the bone inside it is hand_l or hand_r.
	 * Start is the elbow. End is the wrist. The wrist is the wide end.
	 */
	inline constexpr float LowerArmStartDiameterCm = 5.f;
	inline constexpr float LowerArmEndDiameterCm = ForearmWristDiameterCm;

	/**
	 * Cone the length of a clavicle bone. Not a drawn part. The shoulders wrap that bone.
	 * Parent is the wide end and Bone is the narrow end.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone ClavicleCone(const FVector& Parent, const FVector& Bone);

	/**
	 * Cone the length of upperarm_l or upperarm_r. Not a drawn part. The shoulders wrap that bone.
	 * The drawn upper arm is ForearmCone, along lowerarm.
	 * Parent is the wide end and Bone is the narrow end.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone UpperArmCone(const FVector& Parent, const FVector& Bone);

	/**
	 * Shirt cone for the "Upper arm" part. The skeleton bone is lowerarm_l or lowerarm_r.
	 * Parent is the shoulder and Bone is the elbow. The shoulder is the wide end.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone ForearmCone(const FVector& Parent, const FVector& Bone);

	/**
	 * Shirt cone for the "Lower arm" part. The skeleton bone is hand_l or hand_r.
	 * Parent is the elbow and Bone is the wrist. The elbow is the narrow end.
	 * Invalid when the bone is shorter than a centimetre.
	 */
	FSolidFittedCone HandCone(const FVector& Wrist, const FVector& Hand);

	/** The five bones inside the shoulder piece. The mesh itself is the length of neck_01. */
	inline const TCHAR* ShoulderNeckBone = TEXT("neck_01");
	inline const TCHAR* ShoulderLeftClavicleBone = TEXT("clavicle_l");
	inline const TCHAR* ShoulderRightClavicleBone = TEXT("clavicle_r");
	inline const TCHAR* ShoulderLeftUpperArmBone = TEXT("upperarm_l");
	inline const TCHAR* ShoulderRightUpperArmBone = TEXT("upperarm_r");
	/** spine_03's own piece. The shoulder base copies that piece's top oval. */
	inline const TCHAR* SpineChestBone = TEXT("spine_03");

	/**
	 * Radius of the circle at neck_01, in centimetres.
	 * The circle is 20 cm across.
	 */
	inline constexpr float ShoulderTopRadiusCm = 10.f;

	/**
	 * How far the upper-arm bones sit inside the shoulder faces, in centimetres.
	 * The elbow radius, so the forearm starts inside the shoulder rather than through its side.
	 */
	inline constexpr float ShoulderUpperArmClearanceCm = UpperArmElbowDiameterCm * 0.5f;
	/** Rings around the shoulder piece. The bulge keeps bones inside the flat faces. */
	inline constexpr int32 ShoulderSideCount = 10;

	/**
	 * Shirt piece the length of neck_01.
	 * Lower runs from the parent joint to the wide middle. Upper runs from there to neck_01.
	 * The base oval is BaseSideDiameterCm across and TorsoDepthDiameterCm front to back.
	 * The middle widens past both upper-arm bones, and the clavicles, by ShoulderUpperArmClearanceCm.
	 * It then narrows. neck_01 is a circle 20 cm across. WideAxis keeps the full width.
	 * Invalid when neck_01 is shorter than two centimetres.
	 * The shoulder part calls this while it draws.
	 */
	FSolidShoulderPiece ShoulderPiece(
		const FVector& NeckStart,
		const FVector& NeckEnd,
		const FVector* SegmentStart,
		const FVector* SegmentEnd,
		int32 SegmentCount,
		float BaseSideDiameterCm,
		const FVector& WideAxis);

	/** True when Point is inside the shoulder oval. The flat faces sit slightly inside this. */
	bool ShoulderContains(const FSolidShoulderPiece& Piece, const FVector& Point);

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

	/** Fit on the part. Hover and the older cone checks call this. */
	inline FSolidPartDraw DrawForPart(
		FSolidBodyPart& Part,
		const FVector& From,
		const FVector& To,
		const FVector* SegmentStart,
		const FVector* SegmentEnd,
		const int32 SegmentCount,
		const FVector& WideAxis)
	{
		return Part.Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	}

	/** Start and end radii of the cone from Parent to Child. Both ends are joint radii. */
	void SegmentRadii(
		const TArray<float>& Radii,
		int32 Parent,
		int32 Child,
		float& OutStart,
		float& OutEnd);

	/**
	 * Transient low-poly mesh owned by Outer.
	 * FSolidBodyMesh draws the triangles. This wraps them in a skeletal mesh.
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
