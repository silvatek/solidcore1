#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidHipPart::FSolidHipPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InOriginalBones)
	: FSolidBodyPart(InType, InSide, InColour, InStart, InEnd, InOriginalBones)
{
	Bones = PartBones();
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidHipPart::Clone() const
{
	return MakeUnique<FSolidHipPart>(*this);
}

FString SolidBodyMesh::FSolidHipPart::Name() const
{
	return WithSide(TEXT("Hip"));
}

TArray<FName> SolidBodyMesh::FSolidHipPart::PartBones() const
{
	TArray<FName> Names;
	Names.Add(HipSpineBone);
	Names.Add(HipLeftBone);
	Names.Add(HipRightBone);
	return Names;
}

void SolidBodyMesh::FSolidHipPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.Height, Clamped.HipWidth, Clamped.TorsoGirth);
}
