#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"
#include "SolidBodyParams.h"

/**
 * One component of a body mesh.
 * A part's name comes from its type and its side. The skeleton bone inside it often has another name.
 * "Lower arm (L)" wraps hand_l. The cone runs from that bone's parent joint to the bone.
 * X is side to side, in centimetres. Y is front to back.
 * Fit measures the cones. Draw adds this part's triangles.
 */
namespace SolidBodyMesh
{
	enum class ESolidPartColour : uint8
	{
		Shirt,
		Trousers,
		Skin,
	};

	/** The body region a part shows. */
	enum class ESolidPartType : uint8
	{
		Thigh,
		Calf,
		Foot,
		Toe,
		Hip,
		Abdomen,
		Chest,
		Shoulders,
		UpperArm,
		LowerArm,
		Hand,
		Head,
	};

	/** Which side of the body a part is on. Core has no left or right. */
	enum class ESolidPartSide : uint8
	{
		Left,
		Right,
		Core,
	};

	/** "Thigh (L)", "Upper arm (R)", or "Hip" when the part is core. */
	FString PartName(ESolidPartType Type, ESolidPartSide Side);

	/**
	 * The mesh for a part.
	 * A tube is a limb or a spine piece. The shoulder, the head, and a hand have their own mesh.
	 * The joint positions are filled when the part is drawn.
	 */
	enum class ESolidPartShape : uint8
	{
		Tube,
		Shoulder,
		Head,
		Hand,
	};

	/** The mesh for a part type. The shoulders, a hand, and the head have their own mesh. */
	inline ESolidPartShape PartShape(const ESolidPartType Type)
	{
		switch (Type)
		{
		case ESolidPartType::Shoulders:
			return ESolidPartShape::Shoulder;
		case ESolidPartType::Hand:
			return ESolidPartShape::Hand;
		case ESolidPartType::Head:
			return ESolidPartShape::Head;
		default:
			return ESolidPartShape::Tube;
		}
	}

	/** Diameters of one end of a part, in centimetres. */
	struct FSolidPartSection
	{
		float X = 0.f;
		float Y = 0.f;

		static FSolidPartSection Round(float DiameterCm)
		{
			FSolidPartSection Section;
			Section.X = DiameterCm;
			Section.Y = DiameterCm;
			return Section;
		}

		static FSolidPartSection Oval(float SideCm, float DepthCm)
		{
			FSolidPartSection Section;
			Section.X = SideCm;
			Section.Y = DepthCm;
			return Section;
		}
	};

	inline FSolidPartSection RoundSection(const float DiameterCm)
	{
		return FSolidPartSection::Round(DiameterCm);
	}

	inline FSolidPartSection OvalSection(const float SideCm, const float DepthCm)
	{
		return FSolidPartSection::Oval(SideCm, DepthCm);
	}

	/** One vertex of the low-poly body. Bone is the joint that skins it. Slot is the part colour. */
	struct FSolidBodyVert
	{
		FVector Position = FVector::ZeroVector;
		int32 Bone = INDEX_NONE;
		uint8 Slot = 0;
	};

	struct FSolidBodyTri
	{
		int32 V[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
	};

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

	/**
	 * The shoulder part: a wide middle between the chest oval and the neck circle.
	 * Lower runs from the parent joint to that middle. Upper runs from the middle to neck_01.
	 */
	struct FSolidShoulderPiece
	{
		FSolidFittedCone Lower;
		FSolidFittedCone Upper;
		bool bValid = false;
	};

	/** The cones for one part. A shoulder fills both cones. */
	struct FSolidPartDraw
	{
		bool bShoulder = false;
		FSolidFittedCone Cone;
		FSolidShoulderPiece Shoulder;
	};

	/**
	 * One mesh piece.
	 * Bones[0] is the length of the piece. The other bones are wrapped inside it.
	 * Start is the parent joint. End is Bones[0]. Mid is used by the shoulders.
	 * Apply sizes the part and those bones. Fit measures the cones. Draw adds the triangles.
	 */
	struct FSolidBodyPart
	{
		FSolidBodyPart()
			: Type(ESolidPartType::Thigh)
			, Side(ESolidPartSide::Core)
			, Name(PartName(Type, Side))
		{
			Bones = PartBones();
			Shape = PartShape(Type);
		}

		/** The type, the side, its colour, the diameters at each end, and the skeleton. PartBones fills the bone list. */
		FSolidBodyPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InSkeleton);

		ESolidPartType Type = ESolidPartType::Thigh;
		ESolidPartSide Side = ESolidPartSide::Core;
		/** Comes from Type and Side. "Thigh (L)", "Upper arm (R)", or "Hip". */
		FString Name;
		TArray<FName> Bones;
		/** Comes from the part type. Draw uses this and the joint positions. */
		ESolidPartShape Shape = ESolidPartShape::Tube;
		/** Copied from the body when the part is built. PartBones reads it. Apply sizes it. */
		TArray<FSolidDeformBone> Skeleton;
		ESolidPartColour Colour = ESolidPartColour::Shirt;
		FSolidPartSection Start;
		FSolidPartSection End;
		FSolidPartSection Mid;

		/**
		 * The bones this part wraps on its stored skeleton.
		 * A thigh, a spine piece, and the other fixed parts keep their Viking names.
		 * A paired part uses its side to choose left or right. A core part ignores the side.
		 * The head is the head bone, then every eye bone.
		 * A hand lists the fingers that belong to that hand.
		 * The first is a finger whose parent is the hand bone.
		 * The hand is empty when no finger meets the hand.
		 */
		TArray<FName> PartBones() const;

		/** Shirt, trousers, or skin. The companion's colours fill those slots. */
		uint8 MaterialSlot() const;

		/**
		 * Size this part from its type, then size the bones it wraps.
		 * Bones[0] takes the length. The other bones spread by the part's side.
		 * Call once. A listed bone that is missing from the skeleton is left out.
		 */
		void Apply(const FSolidBodyParams& Params);

		/**
		 * The cones for this part.
		 * From is the parent joint and To is Bones[0]. The other bones are the segments.
		 * A shoulder writes its mid diameters.
		 */
		FSolidPartDraw Fit(
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

		/**
		 * Add this part's triangles.
		 * Shape chooses the mesh. The positions are the joints on this call.
		 * The shoulders and every tube fit their cones, then draw them. The head and a hand measure themselves.
		 * WeightBone is the parent joint. The child joint only rotates, so a tube is skinned there.
		 * SpanBone is Bones[0]. The head is skinned to that bone.
		 * For the head, To is the head joint. The sphere starts at From, the base of that bone.
		 * The height is three lengths of the head bone. The horizontal radius is the eye_jnt length.
		 * For a hand, Bones[0] is a finger that meets the hand bone. From is that joint.
		 * To is the top of the finger. The ellipsoid runs from From to To and is skinned to the hand bone.
		 */
		void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

	private:
		void AddTri(TArray<FSolidBodyTri>& Tris, int32 A, int32 B, int32 C) const;

		/** Fit this part's cone, then draw that tube. */
		void DrawTube(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

		/** Fit the shoulder piece, then draw its two cones. */
		void DrawShoulder(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

		/**
		 * A sphere along world up, skinned to the head bone.
		 * The bottom pole is Neck, the parent joint, which is the base of the head bone.
		 * Base is the head joint. The head bone's length is the gap between them.
		 * The height is HeadHeightInBoneLengths times that length.
		 * The horizontal radius is the length of the eye_jnt bones.
		 */
		void DrawHead(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FVector& Neck,
			const FVector& Base,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount) const;

		/**
		 * One ellipsoid skinned to the hand bone.
		 * From is the hand joint, where the finger meets the hand bone and the lower arm ends.
		 * To is the top of that finger bone. The long axis runs from From to To.
		 * The wide axis follows the other fingers. The thin axis is the palm thickness.
		 */
		void DrawHand(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount) const;
	};
}
