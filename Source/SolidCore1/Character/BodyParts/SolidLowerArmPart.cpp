#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidLowerArmPart::FSolidLowerArmPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidLowerArmPart::Clone() const
{
	return MakeUnique<FSolidLowerArmPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidLowerArmPart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(LeftHandBone);
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(RightHandBone);
	}
	return Names;
}

void SolidBodyMesh::FSolidLowerArmPart::Apply(const FSolidBodyParams& Params)
{
	ApplyArm(Params);
}
