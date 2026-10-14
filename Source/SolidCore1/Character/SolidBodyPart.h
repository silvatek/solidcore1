#pragma once

#include "CoreMinimal.h"

/**
 * One component of a body mesh.
 * A part's name is the body region it shows. The skeleton bone inside it often has another name.
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
	 * Fit measures the cones. Draw adds the triangles.
	 */
	struct FSolidBodyPartMesh
	{
		FString Name;
		TArray<FName> Bones;
		ESolidPartColour Colour = ESolidPartColour::Shirt;
		FSolidPartSection Start;
		FSolidPartSection End;
		FSolidPartSection Mid;
		bool bHasMid = false;

		/** Shirt, trousers, or skin. The companion's colours fill those slots. */
		uint8 MaterialSlot() const;

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
		 * WeightBone is the parent joint. The child joint only rotates, so the tube is skinned there.
		 */
		void Draw(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 WeightBone,
			const FVector& From,
			const FVector& To,
			const FVector* SegmentStart,
			const FVector* SegmentEnd,
			int32 SegmentCount,
			const FVector& WideAxis);

	private:
		void AddTri(TArray<FSolidBodyTri>& Tris, int32 A, int32 B, int32 C) const;

		void AddTube(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FVector& TubeStart,
			const FVector& TubeEnd,
			float RadiusStart,
			float RadiusEnd,
			int32 Sides,
			int32 Segments,
			uint8 Slot,
			float FlattenStart,
			float FlattenEnd,
			const FVector& PreferredSide) const;

		void AddShoulder(
			TArray<FSolidBodyVert>& Verts,
			TArray<FSolidBodyTri>& Tris,
			int32 BoneIndex,
			const FSolidShoulderPiece& Piece,
			uint8 Slot) const;
	};
}
