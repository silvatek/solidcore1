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

void SolidBodyMesh::FSolidHeadPart::DrawHead(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FVector& Neck,
	const FVector& Base,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount) const
{
	if (BoneIndex == INDEX_NONE || !SegmentStart || !SegmentEnd || Bones.Num() < 2)
	{
		return;
	}

	const float HeadLength = FVector::Dist(Neck, Base);
	if (HeadLength < 1.f)
	{
		return;
	}

	float RadiusSum = 0.f;
	int32 Count = 0;
	const int32 Limit = FMath::Min(SegmentCount, Bones.Num() - 1);
	for (int32 Index = 0; Index < Limit; ++Index)
	{
		if (!IsEyeJointBoneName(Bones[Index + 1].ToString()))
		{
			continue;
		}
		const float EyeLength = FVector::Dist(SegmentStart[Index], SegmentEnd[Index]);
		if (EyeLength < 1.f)
		{
			continue;
		}
		RadiusSum += EyeLength;
		++Count;
	}
	if (Count == 0)
	{
		return;
	}

	const float HorizontalRadius = RadiusSum / static_cast<float>(Count);
	const float Height = HeadLength * HeadHeightInBoneLengths;
	const float VerticalRadius = Height * 0.5f;
	const FVector Center = Neck + FVector::UpVector * VerticalRadius;
	const FVector Up = FVector::UpVector;
	const FVector Right = FVector::RightVector;
	const FVector Forward = FVector::ForwardVector;
	const int32 Sides = FMath::Clamp(HeadSideCount, 3, BodyPartSideLimit);
	const int32 Rings = FMath::Max(HeadRingCount, 3);
	const uint8 Slot = MaterialSlot();
	const int32 BaseVert = Verts.Num();

	auto AddVert = [&](const FVector& Position)
	{
		FSolidBodyVert Vert;
		Vert.Position = Position;
		Vert.Bone = BoneIndex;
		Vert.Slot = Slot;
		Verts.Add(Vert);
	};

	AddVert(Neck);
	for (int32 Ring = 1; Ring < Rings - 1; ++Ring)
	{
		const float T = static_cast<float>(Ring) / static_cast<float>(Rings - 1);
		const float Cos = -FMath::Cos(PI * T);
		const float Sin = FMath::Sin(PI * T);
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const float Phi = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(Sides);
			const FVector Offset = Up * (Cos * VerticalRadius)
				+ Right * (FMath::Cos(Phi) * Sin * HorizontalRadius)
				+ Forward * (FMath::Sin(Phi) * Sin * HorizontalRadius);
			AddVert(Center + Offset);
		}
	}
	AddVert(Center + Up * VerticalRadius);

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
