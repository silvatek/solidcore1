#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidToePart::FSolidToePart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidToePart::Clone() const
{
	return MakeUnique<FSolidToePart>(*this);
}

FString SolidBodyMesh::FSolidToePart::Name() const
{
	return WithSide(TEXT("Toe"));
}

TArray<FName> SolidBodyMesh::FSolidToePart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(LeftToeBone);
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(RightToeBone);
	}
	return Names;
}

void SolidBodyMesh::FSolidToePart::Apply(const FSolidBodyParams& Params)
{
	ApplyLeg(Params);
}
