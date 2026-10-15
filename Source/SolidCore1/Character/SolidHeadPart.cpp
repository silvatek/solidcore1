#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	TArray<FName> HeadBones(const TArray<FSolidDeformBone>& Skeleton)
	{
		using namespace SolidBodyMesh;

		TArray<FName> Bones;
		Bones.Add(HeadBone);
		for (const FSolidDeformBone& Bone : Skeleton)
		{
			const FString BoneName = Bone.Name.ToString();
			if (BoneName.Equals(HeadBone, ESearchCase::IgnoreCase))
			{
				Bones[0] = Bone.Name;
			}
			else if (IsEyeBoneName(BoneName))
			{
				Bones.AddUnique(Bone.Name);
			}
		}
		return Bones;
	}
}

SolidBodyMesh::FSolidHeadPart::FSolidHeadPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidHeadPart::Clone() const
{
	return MakeUnique<FSolidHeadPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidHeadPart::PartBones() const
{
	return HeadBones(Skeleton);
}

void SolidBodyMesh::FSolidHeadPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	const float Scale = Clamped.HeadSize;
	Start.X *= Scale;
	Start.Y *= Scale;
	End.X *= Scale;
	End.Y *= Scale;
	SizeBones(Scale, Scale);
}

void SolidBodyMesh::FSolidHeadPart::Draw(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 WeightBone,
	const int32 SpanBone,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	(void)WeightBone;
	(void)WideAxis;
	DrawHead(Verts, Tris, SpanBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
}
