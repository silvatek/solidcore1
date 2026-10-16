#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidUpperArmPart::FSolidUpperArmPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InOriginalBones)
	: FSolidBodyPart(InType, InSide, InColour, InStart, InEnd, InOriginalBones)
{
	Bones = PartBones();
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidUpperArmPart::Clone() const
{
	return MakeUnique<FSolidUpperArmPart>(*this);
}

FString SolidBodyMesh::FSolidUpperArmPart::Name() const
{
	return WithSide(TEXT("Upper arm"));
}

TArray<FName> SolidBodyMesh::FSolidUpperArmPart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(TEXT("lowerarm_l"));
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(TEXT("lowerarm_r"));
	}
	return Names;
}

void SolidBodyMesh::FSolidUpperArmPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.ArmLength, Clamped.Bulk, Clamped.Bulk);
}
