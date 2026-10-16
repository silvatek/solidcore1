#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidAbdomenPart::FSolidAbdomenPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InVikingBones)
	: FSolidBodyPart(InType, InSide, InColour, InStart, InEnd, InVikingBones)
{
	Bones = PartBones();
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidAbdomenPart::Clone() const
{
	return MakeUnique<FSolidAbdomenPart>(*this);
}

FString SolidBodyMesh::FSolidAbdomenPart::Name() const
{
	return WithSide(TEXT("Abdomen"));
}

TArray<FName> SolidBodyMesh::FSolidAbdomenPart::PartBones() const
{
	TArray<FName> Names;
	Names.Add(SpineAbdomenBone);
	return Names;
}
