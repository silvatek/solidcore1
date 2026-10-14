#pragma once

#include "CoreMinimal.h"

/**
 * The drawn body is a list of parts.
 * A part's name is the body region it shows. The skeleton bone inside it often has another name.
 * "Lower arm (L)" wraps hand_l. The cone runs from that bone's parent joint to the bone.
 * X is side to side, in centimetres. Y is front to back.
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

	/**
	 * One mesh piece.
	 * Bones[0] is the length of the piece. The other bones are wrapped inside it.
	 * Start is the parent joint. End is Bones[0]. Mid is used by the shoulders.
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
	};

	/** Every part that is drawn. */
	class FSolidBodyMesh
	{
	public:
		TArray<FSolidBodyPartMesh> Parts;

		/**
		 * The body.
		 * RisingSpineBones are the spine bones above the hips, parent to child.
		 * An empty list leaves out the abdomen and the chest. The shoulder base then stays the chest width.
		 */
		static FSolidBodyMesh Make(const TArray<FName>& RisingSpineBones);

		const FSolidBodyPartMesh* Find(const FString& Name) const;
	};
}
