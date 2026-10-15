#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidFootPart::FSolidFootPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidFootPart::Clone() const
{
	return MakeUnique<FSolidFootPart>(*this);
}

FString SolidBodyMesh::FSolidFootPart::Name() const
{
	return WithSide(TEXT("Foot"));
}

TArray<FName> SolidBodyMesh::FSolidFootPart::PartBones() const
{
	TArray<FName> Names;
	if (Side == ESolidPartSide::Left)
	{
		Names.Add(LeftBallBone);
	}
	else if (Side == ESolidPartSide::Right)
	{
		Names.Add(RightBallBone);
	}
	return Names;
}

void SolidBodyMesh::FSolidFootPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.LegLength * Clamped.Height, Clamped.Bulk, Clamped.Bulk);
}
