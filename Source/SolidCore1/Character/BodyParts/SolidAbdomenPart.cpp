#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidAbdomenPart::FSolidAbdomenPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidAbdomenPart::Clone() const
{
	return MakeUnique<FSolidAbdomenPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidAbdomenPart::PartBones() const
{
	TArray<FName> Names;
	Names.Add(SpineAbdomenBone);
	return Names;
}
