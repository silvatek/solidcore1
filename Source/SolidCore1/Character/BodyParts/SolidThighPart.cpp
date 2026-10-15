#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidThighPart::FSolidThighPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidThighPart::Clone() const
{
	return MakeUnique<FSolidThighPart>(*this);
}

FString SolidBodyMesh::FSolidThighPart::Name() const
{
	return WithSide(TEXT("Thigh"));
}

TArray<FName> SolidBodyMesh::FSolidThighPart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(LeftThighBone);
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(RightThighBone);
	}
	return Names;
}

void SolidBodyMesh::FSolidThighPart::Apply(const FSolidBodyParams& Params)
{
	ApplyLeg(Params);
}
