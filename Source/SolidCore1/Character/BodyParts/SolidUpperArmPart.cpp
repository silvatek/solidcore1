#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidUpperArmPart::FSolidUpperArmPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InSkeleton)
	: FSolidBodyPart(InType, InSide, InColour, InStart, InEnd, InSkeleton)
{
	Bones = PartBones();
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidUpperArmPart::Clone() const
{
	return MakeUnique<FSolidUpperArmPart>(*this);
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
	ApplyArm(Params);
}
