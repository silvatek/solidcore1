#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	TArray<FName> HeadBones(const TArray<FSolidDeformBone>& BoneLocals)
	{
		using namespace SolidBodyMesh;

		TArray<FName> Bones;
		Bones.Add(HeadBone);
		for (const FSolidDeformBone& Bone : BoneLocals)
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

	/** The sphere Draw builds. The bottom pole is the parent joint. */
	bool MeasureHead(
		const TArray<FName>& BoneNames,
		const int32 SpanBone,
		const FVector& From,
		const FVector& To,
		const FVector* SegmentStart,
		const FVector* SegmentEnd,
		const int32 SegmentCount,
		float& OutHorizontal,
		float& OutVertical,
		FVector& OutCenter)
	{
		using namespace SolidBodyMesh;

		if (SpanBone == INDEX_NONE || !SegmentStart || !SegmentEnd || BoneNames.Num() < 2)
		{
			return false;
		}

		const float HeadLength = FVector::Dist(From, To);
		if (HeadLength < 1.f)
		{
			return false;
		}

		float RadiusSum = 0.f;
		int32 Count = 0;
		const int32 Limit = FMath::Min(SegmentCount, BoneNames.Num() - 1);
		for (int32 Index = 0; Index < Limit; ++Index)
		{
			if (!IsEyeJointBoneName(BoneNames[Index + 1].ToString()))
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
			return false;
		}

		OutHorizontal = RadiusSum / static_cast<float>(Count);
		OutVertical = HeadLength * HeadHeightInBoneLengths * 0.5f;
		OutCenter = From + FVector::UpVector * OutVertical;
		return true;
	}
}

SolidBodyMesh::FSolidHeadPart::FSolidHeadPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidHeadPart::Clone() const
{
	return MakeUnique<FSolidHeadPart>(*this);
}

FString SolidBodyMesh::FSolidHeadPart::Name() const
{
	return WithSide(TEXT("Head"));
}

TArray<FName> SolidBodyMesh::FSolidHeadPart::PartBones() const
{
	return HeadBones(BoneLocals);
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
	float HorizontalRadius = 0.f;
	float VerticalRadius = 0.f;
	FVector Center = FVector::ZeroVector;
	if (!MeasureHead(
		Bones,
		SpanBone,
		From,
		To,
		SegmentStart,
		SegmentEnd,
		SegmentCount,
		HorizontalRadius,
		VerticalRadius,
		Center))
	{
		return;
	}
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
		Vert.Bone = SpanBone;
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

bool SolidBodyMesh::FSolidHeadPart::LowestZ(
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
	(void)WeightBone;
	(void)WideAxis;
	float Horizontal = 0.f;
	float Vertical = 0.f;
	FVector Center = FVector::ZeroVector;
	if (!MeasureHead(
		Bones,
		SpanBone,
		From,
		To,
		SegmentStart,
		SegmentEnd,
		SegmentCount,
		Horizontal,
		Vertical,
		Center))
	{
		return false;
	}
	(void)Horizontal;
	OutZ = static_cast<float>(Center.Z - Vertical);
	return true;
}
