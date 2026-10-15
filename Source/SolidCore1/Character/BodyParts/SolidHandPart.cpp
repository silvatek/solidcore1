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
	if (WeightBone == INDEX_NONE)
	{
		return;
	}

	const float Length = static_cast<float>(FVector::Dist(From, To));
	if (Length < 1.f)
	{
		return;
	}
	const FVector Long = (To - From) / Length;

	FVector Wide = FVector::ZeroVector;
	if (SegmentStart && SegmentEnd)
	{
		const int32 Limit = FMath::Min(SegmentCount, FMath::Max(Bones.Num() - 1, 0));
		for (int32 Index = 0; Index < Limit; ++Index)
		{
			if (!IsFingerBoneName(Bones[Index + 1].ToString()))
			{
				continue;
			}
			FVector Perp = SegmentEnd[Index] - From;
			Perp -= Long * FVector::DotProduct(Perp, Long);
			if (Perp.SizeSquared() < 0.01)
			{
				continue;
			}
			if (FVector::DotProduct(Perp, Wide) < 0.0)
			{
				Perp = -Perp;
			}
			Wide += Perp;
		}
	}
	if (!Wide.Normalize())
	{
		Wide = FVector::RightVector - Long * FVector::DotProduct(FVector::RightVector, Long);
		if (!Wide.Normalize())
		{
			Wide = FVector::ForwardVector - Long * FVector::DotProduct(FVector::ForwardVector, Long);
		}
	}
	FVector Thin = FVector::CrossProduct(Long, Wide);
	if (!Thin.Normalize())
	{
		return;
	}
	Wide = FVector::CrossProduct(Thin, Long).GetSafeNormal();
	if (Wide.IsNearlyZero())
	{
		return;
	}

	const float LongRadius = Length * 0.5f;
	const float WideRadius = HandWidthDiameterCm * 0.5f;
	const float ThinRadius = HandThicknessDiameterCm * 0.5f;
	const FVector Center = From + Long * LongRadius;
	const int32 Sides = FMath::Clamp(HeadSideCount, 3, BodyPartSideLimit);
	const int32 Rings = FMath::Max(HeadRingCount, 3);
	const uint8 Slot = MaterialSlot();
	const int32 BaseVert = Verts.Num();

	auto AddVert = [&](const FVector& Position)
	{
		FSolidBodyVert Vert;
		Vert.Position = Position;
		Vert.Bone = WeightBone;
		Vert.Slot = Slot;
		Verts.Add(Vert);
	};

	AddVert(From);
	for (int32 Ring = 1; Ring < Rings - 1; ++Ring)
	{
		const float T = static_cast<float>(Ring) / static_cast<float>(Rings - 1);
		const float Cos = -FMath::Cos(PI * T);
		const float Sin = FMath::Sin(PI * T);
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const float Phi = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(Sides);
			const FVector Offset = Long * (Cos * LongRadius)
				+ Wide * (FMath::Cos(Phi) * Sin * WideRadius)
				+ Thin * (FMath::Sin(Phi) * Sin * ThinRadius);
			AddVert(Center + Offset);
		}
	}
	AddVert(To);

	auto RingVert = [BaseVert, Sides, Rings](const int32 Ring, const int32 Step)
	{
		if (Ring <= 0)
		{
			return BaseVert;
		}
		if (Ring >= Rings - 1)
		{
			return BaseVert + 1 + (Rings - 2) * Sides;
		}
		return BaseVert + 1 + (Ring - 1) * Sides + (Step % Sides);
	};

	for (int32 Step = 0; Step < Sides; ++Step)
	{
		AddTri(Tris, RingVert(0, 0), RingVert(1, Step + 1), RingVert(1, Step));
	}
	for (int32 Ring = 1; Ring < Rings - 2; ++Ring)
	{
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			AddTri(Tris, RingVert(Ring, Step), RingVert(Ring, Step + 1), RingVert(Ring + 1, Step));
			AddTri(Tris, RingVert(Ring, Step + 1), RingVert(Ring + 1, Step + 1), RingVert(Ring + 1, Step));
		}
	}
	const int32 LastRing = Rings - 2;
	for (int32 Step = 0; Step < Sides; ++Step)
	{
		AddTri(Tris, RingVert(Rings - 1, 0), RingVert(LastRing, Step), RingVert(LastRing, Step + 1));
	}
}
