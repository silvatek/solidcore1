#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidCalfPart::FSolidCalfPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidCalfPart::Clone() const
{
	return MakeUnique<FSolidCalfPart>(*this);
}

FString SolidBodyMesh::FSolidCalfPart::Name() const
{
	return WithSide(TEXT("Calf"));
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
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.LegLength * Clamped.Height, Clamped.Bulk, Clamped.Bulk);
}
