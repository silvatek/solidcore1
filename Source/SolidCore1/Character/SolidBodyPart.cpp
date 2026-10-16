#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	int32 FindPartBone(const TArray<FSolidDeformBone>& BoneLocals, const FName& Name)
	{
		const FString Wanted = Name.ToString();
		for (int32 Index = 0; Index < BoneLocals.Num(); ++Index)
		{
			if (BoneLocals[Index].Name.ToString().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	/** Bind-pose rotation of one bone. A cycle has no rotation. */
	FQuat WorldRotationAt(const TArray<FSolidDeformBone>& BoneLocals, const int32 Index)
	{
		TArray<int32, TInlineAllocator<32>> Up;
		int32 Cursor = Index;
		for (int32 Guard = 0; Guard < BoneLocals.Num(); ++Guard)
		{
			if (!BoneLocals.IsValidIndex(Cursor))
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
			const int32 Parent = BoneLocals[Cursor].ParentIndex;
			if (!BoneLocals.IsValidIndex(Parent) || Parent == Cursor)
			{
				FQuat Rotation = FQuat::Identity;
				for (int32 Step = Up.Num() - 1; Step >= 0; --Step)
				{
					Rotation = Rotation * BoneLocals[Up[Step]].LocalRotation;
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
	const TArray<FSolidDeformBone>& InOriginalBones)
	: Type(InType)
	, Side(InSide)
	, Colour(InColour)
	, Start(InStart)
	, End(InEnd)
	, BoneLocals(InOriginalBones)
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

void SolidBodyMesh::FSolidBodyPart::ApplyCharacterAttributes(const FSolidBodyParams& Params)
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

	const int32 Span = FindPartBone(BoneLocals, Bones[0]);
	if (BoneLocals.IsValidIndex(Span))
	{
		BoneLocals[Span].LocalTranslation *= Length;
	}

	for (int32 BoneNumber = 1; BoneNumber < Bones.Num(); ++BoneNumber)
	{
		const int32 Index = FindPartBone(BoneLocals, Bones[BoneNumber]);
		if (!BoneLocals.IsValidIndex(Index))
		{
			continue;
		}
		const int32 Parent = BoneLocals[Index].ParentIndex;
		const FQuat ParentRotation = BoneLocals.IsValidIndex(Parent)
			? WorldRotationAt(BoneLocals, Parent)
			: FQuat::Identity;
		FVector Offset = ParentRotation.RotateVector(BoneLocals[Index].LocalTranslation);
		Offset.X *= SideScale;
		Offset.Y *= SideScale;
		BoneLocals[Index].LocalTranslation = ParentRotation.UnrotateVector(Offset);
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

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::FSolidBodyPart::Tube(
	const FVector& From,
	const FVector& To,
	const FVector& WideAxis) const
{
	FSolidFittedCone Cone;
	Cone.Start = From;
	Cone.End = To;
	Cone.RadiusStart = Start.X * 0.5f;
	Cone.RadiusEnd = End.X * 0.5f;
	Cone.DepthScaleStart = Start.X > KINDA_SMALL_NUMBER ? Start.Y / Start.X : 1.f;
	Cone.DepthScaleEnd = End.X > KINDA_SMALL_NUMBER ? End.Y / End.X : 1.f;
	Cone.WideAxis = WideAxis;
	Cone.bValid = FVector::Dist(From, To) >= 1.0
		&& Start.X > KINDA_SMALL_NUMBER
		&& End.X > KINDA_SMALL_NUMBER;
	return Cone;
}

SolidBodyMesh::FSolidPartDraw SolidBodyMesh::FSolidBodyPart::Fit(
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	(void)SegmentStart;
	(void)SegmentEnd;
	(void)SegmentCount;
	FSolidPartDraw Out;
	Out.Cone = Tube(From, To, WideAxis);
	return Out;
}

bool SolidBodyMesh::FSolidBodyPart::ConeLowestZ(const FSolidFittedCone& Cone, float& OutZ) const
{
	if (!Cone.bValid)
	{
		return false;
	}

	const FVector Delta = Cone.End - Cone.Start;
	const float Length = Delta.Size();
	if (Length < 1.f)
	{
		return false;
	}

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
	if (!Wide.Normalize())
	{
		return false;
	}
	const FVector Narrow = FVector::CrossProduct(Axis, Wide).GetSafeNormal();
	if (Narrow.IsNearlyZero())
	{
		return false;
	}

	auto RingZ = [&](const FVector& Center, const float Radius, const float DepthScale)
	{
		const float Depth = Radius * FMath::Clamp(DepthScale, 0.2f, 1.f);
		const float Drop = FMath::Sqrt(
			FMath::Square(Radius * static_cast<float>(Wide.Z))
			+ FMath::Square(Depth * static_cast<float>(Narrow.Z)));
		return static_cast<float>(Center.Z) - Drop;
	};

	OutZ = FMath::Min(
		RingZ(Cone.Start, Cone.RadiusStart, Cone.DepthScaleStart),
		RingZ(Cone.End, Cone.RadiusEnd, Cone.DepthScaleEnd));
	return true;
}

bool SolidBodyMesh::FSolidBodyPart::LowestZ(
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
	(void)SpanBone;
	(void)SegmentStart;
	(void)SegmentEnd;
	(void)SegmentCount;
	return ConeLowestZ(Tube(From, To, WideAxis), OutZ);
}

bool SolidBodyMesh::FSolidBodyPart::Base(
	const TArray<FSolidDeformBone>& CharacterBones,
	const TArray<FVector>& JointPositions,
	float& OutZ) const
{
	if (Bones.Num() == 0)
	{
		return false;
	}

	const int32 Count = FMath::Min(CharacterBones.Num(), JointPositions.Num());
	const int32 Span = FindPartBone(CharacterBones, Bones[0]);
	if (Span == INDEX_NONE || Span >= Count)
	{
		return false;
	}
	const int32 Parent = CharacterBones[Span].ParentIndex;
	if (!CharacterBones.IsValidIndex(Parent) || !JointPositions.IsValidIndex(Parent) || Parent >= Count)
	{
		return false;
	}

	TArray<FVector> SegmentStart;
	TArray<FVector> SegmentEnd;
	for (int32 BoneNumber = 1; BoneNumber < Bones.Num(); ++BoneNumber)
	{
		const int32 Bone = FindPartBone(CharacterBones, Bones[BoneNumber]);
		if (Bone == INDEX_NONE || Bone >= Count)
		{
			return false;
		}
		const int32 BoneParent = CharacterBones[Bone].ParentIndex;
		if (!CharacterBones.IsValidIndex(BoneParent) || !JointPositions.IsValidIndex(BoneParent) || BoneParent >= Count)
		{
			return false;
		}
		SegmentStart.Add(JointPositions[BoneParent]);
		SegmentEnd.Add(JointPositions[Bone]);
	}

	return LowestZ(
		Parent,
		Span,
		JointPositions[Parent],
		JointPositions[Span],
		SegmentStart.GetData(),
		SegmentEnd.GetData(),
		SegmentStart.Num(),
		FVector::ForwardVector,
		OutZ);
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
		const TArray<FSolidDeformBone>& InOriginalBones)
	{
		return MakeUnique<T>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	}
}

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::MakeBodyPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd,
	const TArray<FSolidDeformBone>& InOriginalBones)
{
	switch (InType)
	{
	case ESolidPartType::Thigh:
		return MakePart<FSolidThighPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Calf:
		return MakePart<FSolidCalfPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Foot:
		return MakePart<FSolidFootPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Toe:
		return MakePart<FSolidToePart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Hip:
		return MakePart<FSolidHipPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Abdomen:
		return MakePart<FSolidAbdomenPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Chest:
		return MakePart<FSolidChestPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Shoulders:
		return MakePart<FSolidShoulderPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::UpperArm:
		return MakePart<FSolidUpperArmPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::LowerArm:
		return MakePart<FSolidLowerArmPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Hand:
		return MakePart<FSolidHandPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	case ESolidPartType::Head:
		return MakePart<FSolidHeadPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	default:
		return MakePart<FSolidBodyPart>(InType, InSide, InColour, InStart, InEnd, InOriginalBones);
	}
}
