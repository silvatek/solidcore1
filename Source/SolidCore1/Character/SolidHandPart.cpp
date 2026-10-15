#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	bool NameMarksSide(const FString& Lower, const TCHAR* Suffix)
	{
		const FString Infix = FString(Suffix) + TEXT("_");
		return Lower.EndsWith(Suffix) || Lower.Contains(Infix);
	}

	bool FingerBelongsToHand(
		const TArray<FSolidDeformBone>& Skeleton,
		const int32 Index,
		const bool bLeft)
	{
		if (!Skeleton.IsValidIndex(Index))
		{
			return false;
		}
		const FString Lower = Skeleton[Index].Name.ToString().ToLower();
		const bool bNameLeft = NameMarksSide(Lower, TEXT("_l"));
		const bool bNameRight = NameMarksSide(Lower, TEXT("_r"));
		if (bNameLeft || bNameRight)
		{
			return bLeft ? bNameLeft : bNameRight;
		}

		int32 Cursor = Index;
		for (int32 Guard = 0; Guard < Skeleton.Num(); ++Guard)
		{
			if (!Skeleton.IsValidIndex(Cursor))
			{
				return false;
			}
			const FString Current = Skeleton[Cursor].Name.ToString();
			if (Current.Equals(SolidBodyMesh::LeftHandBone, ESearchCase::IgnoreCase))
			{
				return bLeft;
			}
			if (Current.Equals(SolidBodyMesh::RightHandBone, ESearchCase::IgnoreCase))
			{
				return !bLeft;
			}
			const int32 Parent = Skeleton[Cursor].ParentIndex;
			if (!Skeleton.IsValidIndex(Parent) || Parent == Cursor)
			{
				return false;
			}
			Cursor = Parent;
		}
		return false;
	}

	TArray<FName> HandBones(const SolidBodyMesh::ESolidPartSide Side, const TArray<FSolidDeformBone>& Skeleton)
	{
		using namespace SolidBodyMesh;

		const bool bLeft = Side == ESolidPartSide::Left;
		const TCHAR* HandName = bLeft ? LeftHandBone : RightHandBone;
		int32 HandIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Skeleton.Num(); ++Index)
		{
			if (Skeleton[Index].Name.ToString().Equals(HandName, ESearchCase::IgnoreCase))
			{
				HandIndex = Index;
				break;
			}
		}

		FName SpanFinger = NAME_None;
		TArray<FName> Fingers;
		for (int32 Index = 0; Index < Skeleton.Num(); ++Index)
		{
			const FString BoneName = Skeleton[Index].Name.ToString();
			if (!IsFingerBoneName(BoneName)
				|| !FingerBelongsToHand(Skeleton, Index, bLeft)
				|| !Skeleton.IsValidIndex(Skeleton[Index].ParentIndex))
			{
				continue;
			}
			Fingers.Add(Skeleton[Index].Name);
			if (SpanFinger.IsNone() && Skeleton[Index].ParentIndex == HandIndex)
			{
				SpanFinger = Skeleton[Index].Name;
			}
		}

		TArray<FName> Bones;
		if (!SpanFinger.IsNone())
		{
			Bones.Add(SpanFinger);
			for (const FName& Finger : Fingers)
			{
				Bones.AddUnique(Finger);
			}
		}
		return Bones;
	}
}

SolidBodyMesh::FSolidHandPart::FSolidHandPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidHandPart::Clone() const
{
	return MakeUnique<FSolidHandPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidHandPart::PartBones() const
{
	return HandBones(Side, Skeleton);
}

void SolidBodyMesh::FSolidHandPart::Apply(const FSolidBodyParams& Params)
{
	(void)Params;
	SizeBones(1.f, 1.f);
}

void SolidBodyMesh::FSolidHandPart::Draw(
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
	(void)SpanBone;
	(void)WideAxis;
	DrawHand(Verts, Tris, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
}
