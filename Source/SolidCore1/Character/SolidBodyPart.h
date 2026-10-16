#pragma once

#include "CoreMinimal.h"
#include "SolidBodyDeform.h"
#include "SolidBodyParams.h"

/**
 * One component of a body mesh.
 * A part's name comes from Name(). The skeleton bone inside it often has another name.
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
	 * One mesh piece. Draw builds a tube.
	 * Each part type is its own class and lists its own bones.
	 * The abdomen and the chest keep this sizing: height and torso girth.
	 * The head, a hand, and the shoulders draw their own mesh.
	 * Bones[0] is the length of the piece. The other bones are wrapped inside it.
	 * Start is the parent joint. End is Bones[0]. Mid is used by the shoulders.
	 * Apply sizes the part and those bones. Fit measures the cones. Draw adds the triangles.
	 * Base is the bottom of this mesh. The body keeps the lowest one.
	 */
	class FSolidBodyPart
	{
	public:
		FSolidBodyPart()
			: Type(ESolidPartType::Thigh)
			, Side(ESolidPartSide::Core)
		{
			Bones = FSolidBodyPart::PartBones();
			Shape = PartShape(Type);
		}

		/** The type, the side, its colour, the diameters at each end, and the original pose. PartBones fills the bone list. */
		FSolidBodyPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual ~FSolidBodyPart() = default;
		FSolidBodyPart(const FSolidBodyPart&) = default;
		FSolidBodyPart& operator=(const FSolidBodyPart&) = default;
		FSolidBodyPart(FSolidBodyPart&&) = default;
		FSolidBodyPart& operator=(FSolidBodyPart&&) = default;

		/** A copy that keeps this part's own mesh. The body clones with this. */
		virtual TUniquePtr<FSolidBodyPart> Clone() const;

		/** "Thigh (L)", "Upper arm (R)", or "Hip" when the part is core. */
		virtual FString Name() const;

		ESolidPartType Type = ESolidPartType::Thigh;
		ESolidPartSide Side = ESolidPartSide::Core;
		TArray<FName> Bones;
		/** Comes from the part type. A tube, the shoulders, the head, or a hand. */
		ESolidPartShape Shape = ESolidPartShape::Tube;
		/**
		 * Local transforms copied onto this part.
		 * Make stores the original pose, so PartBones can list the fingers and the eyes.
		 * Apply replaces it with the pose so far, then sizes the bones this part wraps.
		 */
		TArray<FSolidDeformBone> BoneLocals;
		ESolidPartColour Colour = ESolidPartColour::Shirt;
		FSolidPartSection Start;
		FSolidPartSection End;
		FSolidPartSection Mid;

		/**
		 * The bones this part wraps on BoneLocals.
		 * The base list is empty. Each part type replaces it.
		 * A paired part uses its side to choose left or right. A core part ignores the side.
		 */
		virtual TArray<FName> PartBones() const;

		/** Shirt, trousers, or skin. The companion's colours fill those slots. */
		uint8 MaterialSlot() const;

		/**
		 * Size this part, then size the bones it wraps.
		 * Height lengthens Bones[0]. Torso girth widens the section and spreads the other bones.
		 * The abdomen and the chest keep this. A limb, the hip, the head, a hand, and the shoulders size themselves.
		 * Call once. A listed bone that is missing from BoneLocals is left out.
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

		/**
		 * This part's base, in mesh space.
		 * Finds the bones it wraps, then measures the mesh. A tube uses its two end rings.
		 * The head, a hand, and the shoulders measure their own mesh.
		 * False when this part draws nothing.
		 */
		bool Base(
			const TArray<FSolidDeformBone>& CharacterBones,
			const TArray<FVector>& JointPositions,
			float& OutZ) const;

		/**
		 * The bottom of this part once the joints are known.
		 * Base finds those joints. A tube uses its two end rings.
		 * The head, a hand, and the shoulders replace this.
		 */
		virtual bool LowestZ(
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis,
			float& OutZ) const;

	protected:
		/** The tube Draw builds from these joints. */
		FSolidFittedCone Tube(const FVector& From, const FVector& To, const FVector& WideAxis) const;

		/** Lowest Z on the two end rings. False when the cone is not drawn. */
		bool ConeLowestZ(const FSolidFittedCone& Cone, float& OutZ) const;

		/** Widen the section, then size the bones. Depth scales the front-to-back. SideScale spreads the other bones. */
		void SizeSection(float Length, float SideScale, float Depth);

		/** Bones[0] takes Length. The other bones spread by SideScale. */
		void SizeBones(float Length, float SideScale);

		/** Left and right add " (L)" or " (R)". Core returns the label alone. */
		FString WithSide(const TCHAR* Label) const;

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
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
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
		/** The bottom of the head sphere. That pole sits on the parent joint. */
		virtual bool LowestZ(
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis,
			float& OutZ) const override;
	};

	/**
	 * The fingers on one hand. The first is a finger whose parent is the hand bone.
	 * Empty when no finger meets the hand. The original lengths stay. The mesh is an ellipsoid.
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
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
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
		/** The bottom of the hand ellipsoid. */
		virtual bool LowestZ(
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis,
			float& OutZ) const override;
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
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
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
		/** The bottom of the two shoulder cones. */
		virtual bool LowestZ(
			int32 WeightBone,
			int32 SpanBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis,
			float& OutZ) const override;
	};

	/** calf_l or calf_r. Leg length and bulk size it. Draw keeps the tube. */
	class FSolidThighPart : public FSolidBodyPart
	{
	public:
		FSolidThighPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** foot_l or foot_r. Leg length and bulk size it. Draw keeps the tube. */
	class FSolidCalfPart : public FSolidBodyPart
	{
	public:
		FSolidCalfPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** ball_l or ball_r. Leg length and bulk size it. Draw keeps the tube. */
	class FSolidFootPart : public FSolidBodyPart
	{
	public:
		FSolidFootPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** The toe bone on this side. Leg length and bulk size it. Draw keeps the tube. */
	class FSolidToePart : public FSolidBodyPart
	{
	public:
		FSolidToePart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** spine_01, then both thigh bones. Height, hip width, and torso girth size it. Draw keeps the tube. */
	class FSolidHipPart : public FSolidBodyPart
	{
	public:
		FSolidHipPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** spine_02. Height and torso girth size it. Draw keeps the tube. */
	class FSolidAbdomenPart : public FSolidBodyPart
	{
	public:
		FSolidAbdomenPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
	};

	/** spine_03. Height and torso girth size it. Draw keeps the tube. */
	class FSolidChestPart : public FSolidBodyPart
	{
	public:
		FSolidChestPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
	};

	/** lowerarm_l or lowerarm_r. Arm length and bulk size it. Draw keeps the tube. */
	class FSolidUpperArmPart : public FSolidBodyPart
	{
	public:
		FSolidUpperArmPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** hand_l or hand_r. Arm length and bulk size it. Draw keeps the tube. */
	class FSolidLowerArmPart : public FSolidBodyPart
	{
	public:
		FSolidLowerArmPart(
			ESolidPartType InType,
			ESolidPartSide InSide,
			ESolidPartColour InColour,
			const FSolidPartSection& InStart,
			const FSolidPartSection& InEnd,
			const TArray<FSolidDeformBone>& InOriginalBones);

		virtual TUniquePtr<FSolidBodyPart> Clone() const override;
		virtual FString Name() const override;
		virtual TArray<FName> PartBones() const override;
		virtual void Apply(const FSolidBodyParams& Params) override;
	};

	/** The part for this type. Each type is its own class. */
	TUniquePtr<FSolidBodyPart> MakeBodyPart(
		ESolidPartType InType,
		ESolidPartSide InSide,
		ESolidPartColour InColour,
		const FSolidPartSection& InStart,
		const FSolidPartSection& InEnd,
		const TArray<FSolidDeformBone>& InOriginalBones);
}
