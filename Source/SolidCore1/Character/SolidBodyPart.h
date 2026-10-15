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
	 * One mesh piece. A tube is the default: a limb or a spine piece.
	 * The head, a hand, and the shoulders are their own parts and draw their own mesh.
	 * Bones[0] is the length of the piece. The other bones are wrapped inside it.
	 * Start is the parent joint. End is Bones[0]. Mid is used by the shoulders.
	 * Apply sizes the part and those bones. Fit measures the cones. Draw adds the triangles.
	 */
	class FSolidBodyPart
	{
	public:
		FSolidBodyPart()
			: Type(ESolidPartType::Thigh)
			, Side(ESolidPartSide::Core)
			, Name(PartName(Type, Side))
		{
			Bones = FSolidBodyPart::PartBones();
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

		virtual ~FSolidBodyPart() = default;
		FSolidBodyPart(const FSolidBodyPart&) = default;
		FSolidBodyPart& operator=(const FSolidBodyPart&) = default;
		FSolidBodyPart(FSolidBodyPart&&) = default;
		FSolidBodyPart& operator=(FSolidBodyPart&&) = default;

		/** A copy that keeps this part's own mesh. The body clones with this. */
		virtual TUniquePtr<FSolidBodyPart> Clone() const;

		ESolidPartType Type = ESolidPartType::Thigh;
		ESolidPartSide Side = ESolidPartSide::Core;
		/** Comes from Type and Side. "Thigh (L)", "Upper arm (R)", or "Hip". */
		FString Name;
		TArray<FName> Bones;
		/** Comes from the part type. The same mapping chooses the subclass. */
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
		 * The head, a hand, and the shoulders replace this list.
		 */
		virtual TArray<FName> PartBones() const;

		/** Shirt, trousers, or skin. The companion's colours fill those slots. */
		uint8 MaterialSlot() const;

		/**
		 * Size this part, then size the bones it wraps.
		 * A thigh, calf, foot, and toe use leg length and bulk.
		 * The hip uses height, hip width, and torso girth.
		 * An arm uses arm length and bulk. The abdomen and the chest use height and torso girth.
		 * Bones[0] takes the length. The other bones spread by the part's side.
		 * Call once. A listed bone that is missing from the skeleton is left out.
		 * The head, a hand, and the shoulders size themselves.
		 */
		virtual void Apply(const FSolidBodyParams& Params);

		/**
		 * The cone for this part.
		 * From is the parent joint and To is Bones[0]. The other bones are the segments.
		 * The shoulders measure their own piece.
		 */
		virtual FSolidPartDraw Fit(
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

		/**
		 * Add this part's triangles. A tube fits its cone, then draws it.
		 * The positions are the joints on this call.
		 * WeightBone is the parent joint. The child joint only rotates, so a tube is skinned there.
		 * SpanBone is Bones[0].
		 * The head, a hand, and the shoulders draw their own mesh.
		 */
		virtual void Draw(
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

	protected:
		/** Bones[0] takes Length. The other bones spread by SideScale. */
		void SizeBones(float Length, float SideScale);

		void AddTri(TArray<FSolidBodyTri>& Tris, int32 A, int32 B, int32 C) const;
	};

	/** The head bone, then every eye bone. Head size scales the part and those bones. The mesh is a sphere. */
	class FSolidHeadPart : public FSolidBodyPart
	{
	public:
		FSolidHeadPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InSkeleton);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
		/**
		 * A sphere along world up, skinned to the head bone.
		 * From is the parent joint, the base of the head bone. To is the head joint.
		 * The height is HeadHeightInBoneLengths times that length.
		 * The horizontal radius is the length of the eye_jnt bones.
		 */
		virtual void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis) override;
	};

	/**
	 * The fingers on one hand. The first is a finger whose parent is the hand bone.
	 * Empty when no finger meets the hand. The Viking lengths stay. The mesh is an ellipsoid.
	 */
	class FSolidHandPart : public FSolidBodyPart
	{
	public:
		FSolidHandPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InSkeleton);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
		/**
		 * One ellipsoid skinned to the hand bone.
		 * From is the hand joint, where the finger meets the hand bone and the lower arm ends.
		 * To is the top of that finger bone. The long axis runs from From to To.
		 * The wide axis follows the other fingers. The thin axis is the palm thickness.
		 */
		virtual void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis) override;
	};

	/**
	 * neck_01, both clavicles, and both upper arms.
	 * Neck length, shoulder width, and torso girth size it. The neck circle uses shoulder width on both axes.
	 * Fit writes the mid diameters, then the mesh draws both cones.
	 */
	class FSolidShoulderPart : public FSolidBodyPart
	{
	public:
		FSolidShoulderPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InSkeleton);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
		virtual FSolidPartDraw Fit(
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis) override;
		/** Fit the shoulder piece, then draw its two cones. */
		virtual void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis) override;
	};

	/** The part for this type. A head, a hand, and the shoulders are their own classes. Anything else is a tube. */
	TUniquePtr<FSolidBodyPart> MakeBodyPart(
		ESolidPartType InType,
		ESolidPartSide InSide,
		ESolidPartColour InColour,
		const FSolidPartSection& InStart,
		const FSolidPartSection& InEnd,
		const TArray<FSolidDeformBone>& InSkeleton);
}
