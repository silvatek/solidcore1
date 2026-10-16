#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidLowerArmPart::FSolidLowerArmPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidLowerArmPart::Clone() const
{
	return MakeUnique<FSolidLowerArmPart>(*this);
}

FString SolidBodyMesh::FSolidLowerArmPart::Name() const
{
	return WithSide(TEXT("Lower arm"));
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
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.ArmLength, Clamped.Bulk, Clamped.Bulk);
}
