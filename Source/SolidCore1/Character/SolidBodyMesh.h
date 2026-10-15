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

		/** Construct one part, read its bones from Skeleton, and append it. */
		void AddBodyPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd)
		{
			FSolidBodyPart Part(InType, InSide, InColour, InStart, InEnd);
			Part.Bones = Part.PartBones(Skeleton);
			Parts.Add(MoveTemp(Part));
		}

		/**
		 * The body for this skeleton.
		 * Each part reads the bones it wraps.
		 * The head gains the eye bones. Each hand lists the fingers on that hand.
		 * The abdomen is spine_02. The chest is spine_03.
		 */
		static FSolidBodyMesh Make(const TArray<FSolidDeformBone>& Bones);

		const FSolidBodyPart* Find(const FString& Name) const;

		/**
		 * Ask every part to size itself and the bones it wraps.
		 * A bone with no part keeps its Viking translation.
		 */
		void Apply(TArray<FSolidDeformBone>& Bones, const FSolidBodyParams& Params);

		/** Draw every part whose bones are on this skeleton. */
		void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			const TArray<FSolidDeformBone>& Bones,
			const TArray<FVector>& Positions);

		/** The skeleton this body was fitted to. AddBodyPart reads it. */
		TArray<FSolidDeformBone> Skeleton;
	};

	/** Stable id for a companion name. Case does not matter. */
	uint32 SeedForName(const FString& Name);

	/** Twist bones only spin a limb. They are not a rising spine piece. */
	bool IsTwistBoneName(const FString& BoneName);

	/** An eye bone belongs to the head. eye_jnt and the eyelids both match. */
	bool IsEyeBoneName(const FString& BoneName);

	/** An eye joint. The head's horizontal radius is the length of this bone. */
	bool IsEyeJointBoneName(const FString& BoneName);

	/**
	 * A finger bone. thumb_, index_, middle_, ring_, pinky_, and little_ match.
	 * A twist does not. A toe does not.
	 */
	bool IsFingerBoneName(const FString& BoneName);

	/** The bone the head sphere is skinned to. The sphere's bottom is that bone's parent joint. */
	inline const TCHAR* HeadBone = TEXT("head");

	/** The sphere is this many head-bone lengths tall. The bottom is the base of the head bone. */
	inline constexpr float HeadHeightInBoneLengths = 3.f;

	/** Five times the first ring. Tubes, shoulders, and the head all use this many sides. */
	inline constexpr int32 BodyPartDetail = 5;
	inline constexpr int32 BodyPartSides = 10 * BodyPartDetail;
	inline constexpr int32 BodyPartSideLimit = 64;
	static_assert(BodyPartSides <= BodyPartSideLimit, "the side count fits the mesh clamp");

	/** Rings around the head, and rings from the chin pole to the crown, including both poles. */
	inline constexpr int32 HeadSideCount = BodyPartSides;
	/** An odd count puts one ring on the equator, halfway up. */
	inline constexpr int32 HeadRingCount = 9;
	static_assert(HeadRingCount >= 3 && HeadRingCount % 2 == 1, "the head keeps a ring halfway up");

	/** The only bone in the left thigh piece. calf_l runs from the hip to the knee. */
	inline const TCHAR* LeftThighBone = TEXT("calf_l");

	/** The only bone in the right thigh piece. The cone matches the left thigh. */
	inline const TCHAR* RightThighBone = TEXT("calf_r");

	/** Diameters of a thigh cone, in centimetres. The hip is the wide end. */
	inline constexpr float LeftThighTopDiameterCm = 20.f;
	inline constexpr float LeftThighBottomDiameterCm = 15.f;

	/** The only bone in the left calf piece. foot_l runs from the knee to the ankle. */
	inline const TCHAR* LeftCalfBone = TEXT("foot_l");

	/** The only bone in the right calf piece. The cone matches the left calf. */
	inline const TCHAR* RightCalfBone = TEXT("foot_r");

	/** Diameters of a calf cone, in centimetres. The knee is the wide end. */
	inline constexpr float CalfTopDiameterCm = LeftThighBottomDiameterCm;
	inline constexpr float CalfBottomDiameterCm = 12.f;

	/** The only bone in the left foot cylinder. ball_l runs from the ankle to the toe. */
	inline const TCHAR* LeftBallBone = TEXT("ball_l");

	/** The only bone in the right foot cylinder. The cylinder matches the left foot. */
	inline const TCHAR* RightBallBone = TEXT("ball_r");

	/** Diameter of a foot cylinder, in centimetres. Both ends are the same. */
	inline constexpr float BallDiameterCm = 10.f;

	/** The only bone in the left toe cone. It continues past ball_l. */
	inline const TCHAR* LeftToeBone = TEXT("extra_finger_l");

	/** The only bone in the right toe cone. The cone matches the left toe. */
	inline const TCHAR* RightToeBone = TEXT("extra_finger_r");

	/** Diameters of a toe cone, in centimetres. The end towards the ankle is the wide end. */
	inline constexpr float ToeAnkleDiameterCm = BallDiameterCm;
	inline constexpr float ToeTipDiameterCm = 5.f;

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

	/** Elbow cross-section when bulk is 1, in centimetres. The drawn upper arm is twice this at the shoulder. */
	inline constexpr float JointElbow = 4.2f;

	/** Wrist cross-section when bulk is 1, in centimetres. The drawn upper arm ends at twice this, and so does the lower arm. */
	inline constexpr float JointWrist = 3.4f;

	/** Drawn upper arm at the shoulder, in centimetres. */
	inline constexpr float UpperArmElbowDiameterCm = JointElbow * 2.f;

	/**
	 * Shared arm diameter, in centimetres.
	 * Narrow end of the upper arm, at the elbow. Wide end of the lower arm, at the wrist.
	 */
	inline constexpr float ForearmWristDiameterCm = JointWrist * 2.f;

	/**
	 * Lower-arm cone, in centimetres. The part is "Lower arm" and the bone inside it is hand_l or hand_r.
	 * Start is the elbow. End is the hand joint, the bottom of the hand bone. The hand mesh starts there.
	 */
	inline constexpr float LowerArmStartDiameterCm = 5.f;
	inline constexpr float LowerArmEndDiameterCm = ForearmWristDiameterCm;

	/** The lower arm ends on this bone. Finger bones meet it, and the hand mesh starts there. */
	inline const TCHAR* LeftHandBone = TEXT("hand_l");
	inline const TCHAR* RightHandBone = TEXT("hand_r");

	/**
	 * The bones inside one hand part.
	 * Each finger is three joints. The hand bone stays on the lower arm.
	 */
	inline const TCHAR* LeftFingerBones[] = {
		TEXT("thumb_01_l"), TEXT("thumb_02_l"), TEXT("thumb_03_l"),
		TEXT("index_01_l"), TEXT("index_02_l"), TEXT("index_03_l"),
		TEXT("middle_01_l"), TEXT("middle_02_l"), TEXT("middle_03_l"),
		TEXT("ring_01_l"), TEXT("ring_02_l"), TEXT("ring_03_l"),
		TEXT("pinky_01_l"), TEXT("pinky_02_l"), TEXT("pinky_03_l"),
	};
	inline const TCHAR* RightFingerBones[] = {
		TEXT("thumb_01_r"), TEXT("thumb_02_r"), TEXT("thumb_03_r"),
		TEXT("index_01_r"), TEXT("index_02_r"), TEXT("index_03_r"),
		TEXT("middle_01_r"), TEXT("middle_02_r"), TEXT("middle_03_r"),
		TEXT("ring_01_r"), TEXT("ring_02_r"), TEXT("ring_03_r"),
		TEXT("pinky_01_r"), TEXT("pinky_02_r"), TEXT("pinky_03_r"),
	};

	/**
	 * Cross-section of one hand ellipsoid, in centimetres.
	 * The long axis runs from the hand joint to the top of one finger bone.
	 * The wide axis follows the other fingers. The thin axis is the palm thickness.
	 */
	inline constexpr float HandWidthDiameterCm = 15.f;
	inline constexpr float HandThicknessDiameterCm = 3.f;

	/** The five bones inside the shoulder piece. The mesh itself is the length of neck_01. */
	inline const TCHAR* ShoulderNeckBone = TEXT("neck_01");
	inline const TCHAR* ShoulderLeftClavicleBone = TEXT("clavicle_l");
	inline const TCHAR* ShoulderRightClavicleBone = TEXT("clavicle_r");
	inline const TCHAR* ShoulderLeftUpperArmBone = TEXT("upperarm_l");
	inline const TCHAR* ShoulderRightUpperArmBone = TEXT("upperarm_r");
	/** spine_02's own piece, between the hips and the chest. */
	inline const TCHAR* SpineAbdomenBone = TEXT("spine_02");
	/** spine_03's own piece. The shoulder base copies that piece's top oval. */
	inline const TCHAR* SpineChestBone = TEXT("spine_03");

	/**
	 * Radius of the circle at neck_01, in centimetres.
	 * The circle is 10 cm across.
	 */
	inline constexpr float ShoulderTopRadiusCm = 5.f;

	/**
	 * How far the upper-arm bones sit inside the shoulder faces, in centimetres.
	 * The elbow radius, so the forearm starts inside the shoulder rather than through its side.
	 */
	inline constexpr float ShoulderUpperArmClearanceCm = UpperArmElbowDiameterCm * 0.5f;
	/** Rings around the shoulder piece. The bulge keeps bones inside the flat faces. */
	inline constexpr int32 ShoulderSideCount = BodyPartSides;

	/**
	 * Shirt piece the length of neck_01.
	 * Lower runs from the parent joint to the wide middle. Upper runs from there to neck_01.
	 * The base oval is BaseSideDiameterCm across and TorsoDepthDiameterCm front to back.
	 * The middle widens past both upper-arm bones, and the clavicles, by ShoulderUpperArmClearanceCm.
	 * It then narrows. neck_01 is a circle 10 cm across. WideAxis keeps the full width.
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
		const FVector& WideAxis,
		float BaseDepthDiameterCm = TorsoDepthDiameterCm,
		float TopDiameterCm = ShoulderTopRadiusCm * 2.f);

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

	/**
	 * Lowest point of the calves, feet, and toes, in mesh space.
	 * A horizontal foot sits one radius below its bone. Zero when none of those bones are present.
	 */
	float FootSoleZ(const TArray<FSolidDeformBone>& Bones, const TArray<FVector>& Positions);

	/** Mesh relative Z that puts that sole on the bottom of the capsule. */
	float FeetMeshOffset(float CapsuleHalfHeight, float SoleZ);

	/**
	 * Transient low-poly mesh owned by Outer.
	 * FSolidBodyMesh draws the triangles. This wraps them in a skeletal mesh.
	 * Source supplies the Viking skeleton and bind pose. Its mesh and skeleton stay unchanged.
	 * Null when Source has no skeleton, or the body has no wearable LOD.
	 * OutSoleZ is the sole of that body, in mesh space.
	 */
	USkeletalMesh* Build(
		const USkeletalMesh* Source,
		const FSolidBodyParams& Params,
		UObject* Outer,
		float& OutSoleZ);

	inline USkeletalMesh* Build(const USkeletalMesh* Source, const FSolidBodyParams& Params, UObject* Outer)
	{
		float SoleZ = 0.f;
		return Build(Source, Params, Outer, SoleZ);
	}

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
}
