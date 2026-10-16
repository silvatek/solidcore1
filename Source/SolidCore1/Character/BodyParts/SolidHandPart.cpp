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
		const TArray<FSolidDeformBone>& BoneLocals,
		const int32 Index,
		const bool bLeft)
	{
		if (!BoneLocals.IsValidIndex(Index))
		{
			return false;
		}
		const FString Lower = BoneLocals[Index].Name.ToString().ToLower();
		const bool bNameLeft = NameMarksSide(Lower, TEXT("_l"));
		const bool bNameRight = NameMarksSide(Lower, TEXT("_r"));
		if (bNameLeft || bNameRight)
		{
			return bLeft ? bNameLeft : bNameRight;
		}

		int32 Cursor = Index;
		for (int32 Guard = 0; Guard < BoneLocals.Num(); ++Guard)
		{
			if (!BoneLocals.IsValidIndex(Cursor))
			{
				return false;
			}
			const FString Current = BoneLocals[Cursor].Name.ToString();
			if (Current.Equals(SolidBodyMesh::LeftHandBone, ESearchCase::IgnoreCase))
			{
				return bLeft;
			}
			if (Current.Equals(SolidBodyMesh::RightHandBone, ESearchCase::IgnoreCase))
			{
				return !bLeft;
			}
			const int32 Parent = BoneLocals[Cursor].ParentIndex;
			if (!BoneLocals.IsValidIndex(Parent) || Parent == Cursor)
			{
				return false;
			}
			Cursor = Parent;
		}
		return false;
	}

	TArray<FName> HandBones(const SolidBodyMesh::ESolidPartSide Side, const TArray<FSolidDeformBone>& BoneLocals)
	{
		using namespace SolidBodyMesh;

		const bool bLeft = Side == ESolidPartSide::Left;
		const TCHAR* HandName = bLeft ? LeftHandBone : RightHandBone;
		int32 HandIndex = INDEX_NONE;
		for (int32 Index = 0; Index < BoneLocals.Num(); ++Index)
		{
			if (BoneLocals[Index].Name.ToString().Equals(HandName, ESearchCase::IgnoreCase))
			{
				HandIndex = Index;
				break;
			}
		}

		FName SpanFinger = NAME_None;
		TArray<FName> Fingers;
		for (int32 Index = 0; Index < BoneLocals.Num(); ++Index)
		{
			const FString BoneName = BoneLocals[Index].Name.ToString();
			if (!IsFingerBoneName(BoneName)
				|| !FingerBelongsToHand(BoneLocals, Index, bLeft)
				|| !BoneLocals.IsValidIndex(BoneLocals[Index].ParentIndex))
			{
				continue;
			}
			Fingers.Add(BoneLocals[Index].Name);
			if (SpanFinger.IsNone() && BoneLocals[Index].ParentIndex == HandIndex)
			{
				SpanFinger = BoneLocals[Index].Name;
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

	struct FHandFrame
	{
		bool bValid = false;
		FVector Center = FVector::ZeroVector;
		FVector Long = FVector::UpVector;
		FVector Wide = FVector::RightVector;
		FVector Thin = FVector::ForwardVector;
		float LongRadius = 0.f;
		float WideRadius = 0.f;
		float ThinRadius = 0.f;
	};

	/** The ellipsoid Draw builds. Invalid when the finger is too short to draw. */
	FHandFrame HandFrame(
		const TArray<FName>& BoneNames,
		const int32 WeightBone,
		const FVector& From,
		const FVector& To,
		const FVector* SegmentStart,
		const FVector* SegmentEnd,
		const int32 SegmentCount)
	{
		using namespace SolidBodyMesh;

		FHandFrame Frame;
		if (WeightBone == INDEX_NONE)
		{
			return Frame;
		}

		const float Length = static_cast<float>(FVector::Dist(From, To));
		if (Length < 1.f)
		{
			return Frame;
		}
		const FVector Long = (To - From) / Length;

		FVector Wide = FVector::ZeroVector;
		if (SegmentStart && SegmentEnd)
		{
			const int32 Limit = FMath::Min(SegmentCount, FMath::Max(BoneNames.Num() - 1, 0));
			for (int32 Index = 0; Index < Limit; ++Index)
			{
				if (!IsFingerBoneName(BoneNames[Index + 1].ToString()))
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
			return Frame;
		}
		Wide = FVector::CrossProduct(Thin, Long).GetSafeNormal();
		if (Wide.IsNearlyZero())
		{
			return Frame;
		}

		Frame.bValid = true;
		Frame.Center = From + Long * (Length * 0.5f);
		Frame.Long = Long;
		Frame.Wide = Wide;
		Frame.Thin = Thin;
		Frame.LongRadius = Length * 0.5f;
		Frame.WideRadius = HandWidthDiameterCm * 0.5f;
		Frame.ThinRadius = HandThicknessDiameterCm * 0.5f;
		return Frame;
	}
}

SolidBodyMesh::FSolidHandPart::FSolidHandPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidHandPart::Clone() const
{
	return MakeUnique<FSolidHandPart>(*this);
}

FString SolidBodyMesh::FSolidHandPart::Name() const
{
	return WithSide(TEXT("Hand"));
}

TArray<FName> SolidBodyMesh::FSolidHandPart::PartBones() const
{
	return HandBones(Side, BoneLocals);
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
	const FHandFrame Frame = HandFrame(Bones, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
	if (!Frame.bValid)
	{
		return;
	}
	const FVector& Long = Frame.Long;
	const FVector& Wide = Frame.Wide;
	const FVector& Thin = Frame.Thin;
	const float LongRadius = Frame.LongRadius;
	const float WideRadius = Frame.WideRadius;
	const float ThinRadius = Frame.ThinRadius;
	const FVector& Center = Frame.Center;
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

bool SolidBodyMesh::FSolidHandPart::LowestZ(
	const int32 WeightBone,
	const int32 SpanBone,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis,
	float& OutZ) const
{
	(void)SpanBone;
	(void)WideAxis;
	const FHandFrame Frame = HandFrame(Bones, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
	if (!Frame.bValid)
	{
		return false;
	}
	const float Extent = FMath::Sqrt(
		FMath::Square(static_cast<float>(Frame.Long.Z) * Frame.LongRadius)
		+ FMath::Square(static_cast<float>(Frame.Wide.Z) * Frame.WideRadius)
		+ FMath::Square(static_cast<float>(Frame.Thin.Z) * Frame.ThinRadius));
	OutZ = static_cast<float>(Frame.Center.Z) - Extent;
	return true;
}
