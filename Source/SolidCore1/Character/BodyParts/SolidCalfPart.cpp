#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidCalfPart::FSolidCalfPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidCalfPart::Clone() const
{
	return MakeUnique<FSolidCalfPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidCalfPart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(LeftCalfBone);
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(RightCalfBone);
	}
	return Names;
}

void SolidBodyMesh::FSolidCalfPart::Apply(const FSolidBodyParams& Params)
{
	ApplyLeg(Params);
}
