#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidChestPart::FSolidChestPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidChestPart::Clone() const
{
	return MakeUnique<FSolidChestPart>(*this);
}

FString SolidBodyMesh::FSolidChestPart::Name() const
{
	return WithSide(TEXT("Chest"));
}

TArray<FName> SolidBodyMesh::FSolidChestPart::PartBones() const
{
	TArray<FName> Names;
	Names.Add(SpineChestBone);
	return Names;
}
