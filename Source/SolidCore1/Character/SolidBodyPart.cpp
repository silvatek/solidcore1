#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	int32 FindPartBone(const TArray<FSolidDeformBone>& Skeleton, const FName& Name)
	{
		const FString Wanted = Name.ToString();
		for (int32 Index = 0; Index < Skeleton.Num(); ++Index)
		{
			if (Skeleton[Index].Name.ToString().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** Bind-pose rotation of one bone. A cycle has no rotation. */
	FQuat WorldRotationAt(const TArray<FSolidDeformBone>& Skeleton, const int32 Index)
	{
		TArray<int32, TInlineAllocator<32>> Up;
		int32 Cursor = Index;
		for (int32 Guard = 0; Guard < Skeleton.Num(); ++Guard)
		{
			if (!Skeleton.IsValidIndex(Cursor))
			{
				return FQuat::Identity;
			}
			for (const int32 Prior : Up)
			{
				if (Prior == Cursor)
				{
					return FQuat::Identity;
				}
			}
			Up.Add(Cursor);
			const int32 Parent = Skeleton[Cursor].ParentIndex;
			if (!Skeleton.IsValidIndex(Parent) || Parent == Cursor)
			{
				FQuat Rotation = FQuat::Identity;
				for (int32 Step = Up.Num() - 1; Step >= 0; --Step)
				{
					Rotation = Rotation * Skeleton[Up[Step]].LocalRotation;
				}
				return Rotation;
			}
			Cursor = Parent;
		}
		return FQuat::Identity;
	}
}

TArray<FName> SolidBodyMesh::FSolidBodyPart::PartBones() const
{
	return {};
}

SolidBodyMesh::FSolidBodyPart::FSolidBodyPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InSkeleton)
	: Type(InType)
	, Side(InSide)
	, Colour(InColour)
	, Start(InStart)
	, End(InEnd)
	, Skeleton(InSkeleton)
{
	// Each part type replaces this list in its own constructor.
	Bones = FSolidBodyPart::PartBones();
	Shape = PartShape(Type);
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidBodyPart::Clone() const
{
	return MakeUnique<FSolidBodyPart>(*this);
}

FString SolidBodyMesh::FSolidBodyPart::WithSide(const TCHAR* Label) const
{
	switch (Side)
	{
	case ESolidPartSide::Left:
		return FString::Printf(TEXT("%s (L)"), Label);
	case ESolidPartSide::Right:
		return FString::Printf(TEXT("%s (R)"), Label);
	default:
		return FString(Label);
	}
}

FString SolidBodyMesh::FSolidBodyPart::Name() const
{
	return WithSide(TEXT("Part"));
}

void SolidBodyMesh::FSolidBodyPart::SizeSection(const float Length, const float SideScale, const float Depth)
{
	Start.X *= SideScale;
	Start.Y *= Depth;
	End.X *= SideScale;
	End.Y *= Depth;
	SizeBones(Length, SideScale);
}

void SolidBodyMesh::FSolidBodyPart::ApplyLeg(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.LegLength * Clamped.Height, Clamped.Bulk, Clamped.Bulk);
}

void SolidBodyMesh::FSolidBodyPart::ApplyArm(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.ArmLength, Clamped.Bulk, Clamped.Bulk);
}

void SolidBodyMesh::FSolidBodyPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	SizeSection(Clamped.Height, Clamped.TorsoGirth, Clamped.TorsoGirth);
}

void SolidBodyMesh::FSolidBodyPart::SizeBones(const float Length, const float SideScale)
{
	if (Bones.Num() == 0)
	{
		return;
	}

	const int32 Span = FindPartBone(Skeleton, Bones[0]);
	if (Skeleton.IsValidIndex(Span))
	{
		Skeleton[Span].LocalTranslation *= Length;
	}

	for (int32 BoneNumber = 1; BoneNumber < Bones.Num(); ++BoneNumber)
	{
		const int32 Index = FindPartBone(Skeleton, Bones[BoneNumber]);
		if (!Skeleton.IsValidIndex(Index))
		{
			continue;
		}
		const int32 Parent = Skeleton[Index].ParentIndex;
		const FQuat ParentRotation = Skeleton.IsValidIndex(Parent)
			? WorldRotationAt(Skeleton, Parent)
			: FQuat::Identity;
		FVector Offset = ParentRotation.RotateVector(Skeleton[Index].LocalTranslation);
		Offset.X *= SideScale;
		Offset.Y *= SideScale;
		Skeleton[Index].LocalTranslation = ParentRotation.UnrotateVector(Offset);
	}
}

uint8 SolidBodyMesh::FSolidBodyPart::MaterialSlot() const
{
	switch (Colour)
	{
	case ESolidPartColour::Trousers:
		return TrouserMaterialSlot;
	case ESolidPartColour::Skin:
		return SkinMaterialSlot;
	default:
		return ShirtMaterialSlot;
	}
}

SolidBodyMesh::FSolidPartDraw SolidBodyMesh::FSolidBodyPart::Fit(
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	FSolidPartDraw Out;
	Out.Cone.Start = From;
	Out.Cone.End = To;
	Out.Cone.RadiusStart = Start.X * 0.5f;
	Out.Cone.RadiusEnd = End.X * 0.5f;
	Out.Cone.DepthScaleStart = Start.X > KINDA_SMALL_NUMBER ? Start.Y / Start.X : 1.f;
	Out.Cone.DepthScaleEnd = End.X > KINDA_SMALL_NUMBER ? End.Y / End.X : 1.f;
	Out.Cone.WideAxis = WideAxis;
	Out.Cone.bValid = FVector::Dist(From, To) >= 1.0
		&& Start.X > KINDA_SMALL_NUMBER
		&& End.X > KINDA_SMALL_NUMBER;
	return Out;
}

void SolidBodyMesh::FSolidBodyPart::AddTri(
	TArray<FSolidBodyTri>& Tris,
	const int32 A,
	const int32 B,
	const int32 C) const
{
	FSolidBodyTri Tri;
	Tri.V[0] = A;
	Tri.V[1] = B;
	Tri.V[2] = C;
	Tris.Add(Tri);
}

void SolidBodyMesh::FSolidBodyPart::Draw(
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
	const FSolidPartDraw Fitted = Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	const FSolidFittedCone& Cone = Fitted.Cone;
	if (!Cone.bValid || WeightBone == INDEX_NONE)
	{
		return;
	}

	const FVector Delta = Cone.End - Cone.Start;
	const float Length = Delta.Size();
	if (Length < 1.f)
	{
		return;
	}

	const uint8 Slot = MaterialSlot();
	const FVector Axis = Delta / Length;
	FVector Wide = Cone.WideAxis - Axis * FVector::DotProduct(Cone.WideAxis, Axis);
	if (Wide.SizeSquared() < 0.01)
	{
		Wide = FVector::CrossProduct(FVector::UpVector, Axis);
	}
	if (Wide.SizeSquared() < 0.01)
	{
		Wide = FVector::CrossProduct(FVector::ForwardVector, Axis);
	}
	Wide = Wide.GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Axis, Wide).GetSafeNormal();
	const int32 SideCount = FMath::Clamp(BodyPartSides, 3, BodyPartSideLimit);
	const int32 RingCount = 2;
	const int32 Base = Verts.Num();

	for (int32 Ring = 0; Ring < RingCount; ++Ring)
	{
		const float T = static_cast<float>(Ring);
		const FVector Center = FMath::Lerp(Cone.Start, Cone.End, static_cast<double>(T));
		const float Radius = FMath::Lerp(Cone.RadiusStart, Cone.RadiusEnd, T);
		const float Squash = FMath::Lerp(
			FMath::Clamp(Cone.DepthScaleStart, 0.2f, 1.f),
			FMath::Clamp(Cone.DepthScaleEnd, 0.2f, 1.f),
			T);
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(SideCount);
			const FVector Offset = Wide * (FMath::Cos(Angle) * Radius)
				+ Up * (FMath::Sin(Angle) * Radius * Squash);
			FSolidBodyVert Vert;
			Vert.Position = Center + Offset;
			Vert.Bone = WeightBone;
			Vert.Slot = Slot;
			Verts.Add(Vert);
		}
	}

	auto Id = [Base, SideCount](const int32 Ring, const int32 Step)
	{
		return Base + Ring * SideCount + (Step % SideCount);
	};
	for (int32 Ring = 0; Ring < RingCount - 1; ++Ring)
	{
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const int32 Next = Step + 1;
			AddTri(Tris, Id(Ring, Step), Id(Ring, Next), Id(Ring + 1, Step));
			AddTri(Tris, Id(Ring, Next), Id(Ring + 1, Next), Id(Ring + 1, Step));
		}
	}
	const int32 Last = RingCount - 1;
	for (int32 Step = 1; Step < SideCount - 1; ++Step)
	{
		AddTri(Tris, Id(0, 0), Id(0, Step + 1), Id(0, Step));
		AddTri(Tris, Id(Last, 0), Id(Last, Step), Id(Last, Step + 1));
	}
}

namespace
{
	template<typename T>
	TUniquePtr<SolidBodyMesh::FSolidBodyPart> MakePart(
		const SolidBodyMesh::ESolidPartType InType,
		const SolidBodyMesh::ESolidPartSide InSide,
		const SolidBodyMesh::ESolidPartColour InColour,
		const SolidBodyMesh::FSolidPartSection& InStart,
		const SolidBodyMesh::FSolidPartSection& InEnd,
		const TArray<FSolidDeformBone>& InSkeleton)
	{
		return MakeUnique<T>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	}
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::MakeBodyPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InSkeleton)
{
	switch (PartShape(InType))
	{
	case ESolidPartShape::Head:
		return MakePart<FSolidHeadPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartShape::Hand:
		return MakePart<FSolidHandPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartShape::Shoulder:
		return MakePart<FSolidShoulderPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartShape::Tube:
		break;
	}

	switch (InType)
	{
	case ESolidPartType::Thigh:
		return MakePart<FSolidThighPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Calf:
		return MakePart<FSolidCalfPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Foot:
		return MakePart<FSolidFootPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Toe:
		return MakePart<FSolidToePart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Hip:
		return MakePart<FSolidHipPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Abdomen:
		return MakePart<FSolidAbdomenPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::Chest:
		return MakePart<FSolidChestPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::UpperArm:
		return MakePart<FSolidUpperArmPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	case ESolidPartType::LowerArm:
		return MakePart<FSolidLowerArmPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
	// The mesh switch above already built the head, a hand, and the shoulders.
	case ESolidPartType::Shoulders:
	case ESolidPartType::Hand:
	case ESolidPartType::Head:
		break;
	}
	return MakePart<FSolidBodyPart>(InType, InSide, InColour, InStart, InEnd, InSkeleton);
}
