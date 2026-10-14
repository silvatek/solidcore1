#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

#include <initializer_list>

uint8 SolidBodyMesh::FSolidBodyPartMesh::MaterialSlot() const
{
	switch (Colour)
	{
	case ESolidPartColour::Trousers:
		return TrouserMaterialSlot;
	case ESolidPartColour::Skin:
		return SkinMaterialSlot;
	default:
		return ShirtMaterialSlot;
	}
}

namespace
{
	SolidBodyMesh::FSolidBodyPartMesh Part(
		const TCHAR* Name,
		std::initializer_list<const TCHAR*> Bones,
		const SolidBodyMesh::ESolidPartColour Colour,
		const SolidBodyMesh::FSolidPartSection& Start,
		const SolidBodyMesh::FSolidPartSection& End)
	{
		SolidBodyMesh::FSolidBodyPartMesh Out;
		Out.Name = Name;
		for (const TCHAR* Bone : Bones)
		{
			Out.Bones.Add(Bone);
		}
		Out.Colour = Colour;
		Out.Start = Start;
		Out.End = End;
		return Out;
	}

	FString SpinePartName(const int32 Step, const int32 Count)
	{
		if (Step == Count - 1)
		{
			return TEXT("Chest");
		}
		if (Step == 0)
		{
			return TEXT("Abdomen");
		}
		return FString::Printf(TEXT("Spine %d"), Step + 1);
	}
}

SolidBodyMesh::FSolidBodyMesh SolidBodyMesh::FSolidBodyMesh::Make(const TArray<FName>& RisingSpineBones)
{
	using SolidBodyMesh::ESolidPartColour;
	using SolidBodyMesh::FSolidPartSection;

	FSolidBodyMesh Body;
	const FSolidPartSection Round20 = FSolidPartSection::Round(LeftThighTopDiameterCm);
	Body.Parts.Add(Part(TEXT("Thigh (L)"), { LeftThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Thigh (R)"), { RightThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Calf (L)"), { LeftCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Calf (R)"), { RightCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Foot (L)"), { LeftBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)));
	Body.Parts.Add(Part(TEXT("Foot (R)"), { RightBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)));
	Body.Parts.Add(Part(TEXT("Toe (L)"), { LeftExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)));
	Body.Parts.Add(Part(TEXT("Toe (R)"), { RightExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)));

	// The hip mesh is the length of spine_01. The thighs are inside it.
	Body.Parts.Add(Part(
		TEXT("Hip"),
		{ HipSpineBone, HipLeftBone, HipRightBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(HipSideDiameterCm, HipDepthDiameterCm),
		FSolidPartSection::Oval(HipTopSideDiameterCm, HipDepthDiameterCm)));

	const int32 SpineCount = RisingSpineBones.Num();
	for (int32 Step = 0; Step < SpineCount; ++Step)
	{
		float BottomCm = 0.f;
		float TopCm = 0.f;
		TorsoSideDiameters(Step, SpineCount, BottomCm, TopCm);
		FSolidBodyPartMesh Spine = Part(
			TEXT("Spine"),
			{},
			ESolidPartColour::Shirt,
			FSolidPartSection::Oval(BottomCm, TorsoDepthDiameterCm),
			FSolidPartSection::Oval(TopCm, TorsoDepthDiameterCm));
		Spine.Name = SpinePartName(Step, SpineCount);
		Spine.Bones.Add(RisingSpineBones[Step]);
		Body.Parts.Add(Spine);
	}

	float ShoulderSideCm = ChestSideDiameterCm;
	for (const FSolidBodyPartMesh& Spine : Body.Parts)
	{
		if (Spine.Bones.Num() == 1 && Spine.Bones[0] == FName(SpineChestBone))
		{
			ShoulderSideCm = Spine.End.X;
		}
	}

	// Shoulders follow neck_01. Mid is fitted around the clavicles and the upper arms when the mesh is built.
	FSolidBodyPartMesh Shoulders = Part(
		TEXT("Shoulders"),
		{ ShoulderNeckBone, ShoulderLeftClavicleBone, ShoulderRightClavicleBone, ShoulderLeftUpperArmBone, ShoulderRightUpperArmBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(ShoulderSideCm, TorsoDepthDiameterCm),
		FSolidPartSection::Round(ShoulderTopRadiusCm * 2.f));
	Shoulders.bHasMid = true;
	Body.Parts.Add(Shoulders);

	// upperarm joint to elbow. The skeleton names that bone lowerarm.
	Body.Parts.Add(Part(
		TEXT("Upper arm (L)"),
		{ TEXT("lowerarm_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)));
	Body.Parts.Add(Part(
		TEXT("Upper arm (R)"),
		{ TEXT("lowerarm_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)));

	// elbow to wrist. The skeleton names that bone hand. Narrow at the elbow, wide at the wrist.
	Body.Parts.Add(Part(
		TEXT("Lower arm (L)"),
		{ TEXT("hand_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)));
	Body.Parts.Add(Part(
		TEXT("Lower arm (R)"),
		{ TEXT("hand_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)));

	return Body;
}

const SolidBodyMesh::FSolidBodyPartMesh* SolidBodyMesh::FSolidBodyMesh::Find(const FString& Name) const
{
	for (const FSolidBodyPartMesh& Part : Parts)
	{
		if (Part.Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return &Part;
		}
	}
	return nullptr;
}
