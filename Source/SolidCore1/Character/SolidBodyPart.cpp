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

FString SolidBodyMesh::PartName(const ESolidPartType Type, const ESolidPartSide Side)
{
	const TCHAR* Label = TEXT("Part");
	switch (Type)
	{
	case ESolidPartType::Thigh:
		Label = TEXT("Thigh");
		break;
	case ESolidPartType::Calf:
		Label = TEXT("Calf");
		break;
	case ESolidPartType::Foot:
		Label = TEXT("Foot");
		break;
	case ESolidPartType::Toe:
		Label = TEXT("Toe");
		break;
	case ESolidPartType::Hip:
		Label = TEXT("Hip");
		break;
	case ESolidPartType::Abdomen:
		Label = TEXT("Abdomen");
		break;
	case ESolidPartType::Chest:
		Label = TEXT("Chest");
		break;
	case ESolidPartType::Shoulders:
		Label = TEXT("Shoulders");
		break;
	case ESolidPartType::UpperArm:
		Label = TEXT("Upper arm");
		break;
	case ESolidPartType::LowerArm:
		Label = TEXT("Lower arm");
		break;
	case ESolidPartType::Hand:
		Label = TEXT("Hand");
		break;
	case ESolidPartType::Head:
		Label = TEXT("Head");
		break;
	}

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

SolidBodyMesh::FSolidBodyPart::FSolidBodyPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const std::initializer_list<const TCHAR*> InBones,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd)
	: Type(InType)
	, Side(InSide)
	, Name(PartName(InType, InSide))
	, Colour(InColour)
	, Start(InStart)
	, End(InEnd)
	, Measure(PartMeasure(InType))
{
	for (const TCHAR* Bone : InBones)
	{
		Bones.Add(Bone);
	}
}

SolidBodyMesh::FSolidBodyPart::FSolidBodyPart(
	const ESolidPartType InType,
	const ESolidPartSide InSide,
	const TArray<FName>& InBones,
	const ESolidPartColour InColour,
	const FSolidPartSection& InStart,
	const FSolidPartSection& InEnd)
	: Type(InType)
	, Side(InSide)
	, Name(PartName(InType, InSide))
	, Bones(InBones)
	, Colour(InColour)
	, Start(InStart)
	, End(InEnd)
	, Measure(PartMeasure(InType))
{
}

void SolidBodyMesh::FSolidBodyPart::Apply(
	TArray<FSolidDeformBone>& Skeleton,
	const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	float Length = 1.f;
	float SideScale = 1.f;
	float Depth = 1.f;
	switch (Measure)
	{
	case ESolidPartMeasure::Leg:
		Length = Clamped.LegLength * Clamped.Height;
		SideScale = Clamped.Bulk;
		Depth = Clamped.Bulk;
		break;
	case ESolidPartMeasure::Hip:
		Length = Clamped.Height;
		SideScale = Clamped.HipWidth;
		Depth = Clamped.TorsoGirth;
		break;
	case ESolidPartMeasure::Shoulder:
		Length = Clamped.NeckLength;
		SideScale = Clamped.ShoulderWidth;
		Depth = Clamped.TorsoGirth;
		break;
	case ESolidPartMeasure::Arm:
		Length = Clamped.ArmLength;
		SideScale = Clamped.Bulk;
		Depth = Clamped.Bulk;
		break;
	case ESolidPartMeasure::Head:
		Length = Clamped.HeadSize;
		SideScale = Clamped.HeadSize;
		Depth = Clamped.HeadSize;
		break;
	case ESolidPartMeasure::Hand:
		Length = 1.f;
		SideScale = 1.f;
		Depth = 1.f;
		break;
	default:
		Length = Clamped.Height;
		SideScale = Clamped.TorsoGirth;
		Depth = Clamped.TorsoGirth;
		break;
	}

	Start.X *= SideScale;
	Start.Y *= Depth;
	if (Measure == ESolidPartMeasure::Shoulder)
	{
		End.X *= SideScale;
		End.Y *= SideScale;
	}
	else
	{
		End.X *= SideScale;
		End.Y *= Depth;
	}
	if (bHasMid)
	{
		Mid.X *= SideScale;
		Mid.Y *= Depth;
	}

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
	if (bHasMid)
	{
		Out.bShoulder = true;
		Out.Shoulder = ShoulderPiece(From, To, SegmentStart, SegmentEnd, SegmentCount, Start.X, WideAxis, Start.Y, End.X);
		if (Out.Shoulder.bValid)
		{
			const float SideRadius = Out.Shoulder.Lower.RadiusEnd;
			const float DepthRadius = SideRadius * Out.Shoulder.Lower.DepthScaleEnd;
			Mid.X = SideRadius * 2.f;
			Mid.Y = DepthRadius * 2.f;
		}
		return Out;
	}

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

void SolidBodyMesh::FSolidBodyPart::AddTube(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	const FSolidPartDraw Fitted = Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	const FSolidFittedCone& Cone = Fitted.Cone;
	if (!Cone.bValid || BoneIndex == INDEX_NONE)
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
			Vert.Bone = BoneIndex;
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

void SolidBodyMesh::FSolidBodyPart::AddShoulder(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	const FSolidPartDraw Fitted = Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	const FSolidShoulderPiece& Piece = Fitted.Shoulder;
	const uint8 Slot = MaterialSlot();
	if (!Piece.bValid || BoneIndex == INDEX_NONE)
	{
		return;
	}

	const FVector Centers[] = { Piece.Lower.Start, Piece.Lower.End, Piece.Upper.End };
	const float Radii[] = { Piece.Lower.RadiusStart, Piece.Lower.RadiusEnd, Piece.Upper.RadiusEnd };
	const float Squash[] = {
		Piece.Lower.DepthScaleStart,
		Piece.Lower.DepthScaleEnd,
		Piece.Upper.DepthScaleEnd,
	};
	const FVector Delta = Centers[2] - Centers[0];
	const float Length = Delta.Size();
	if (Length < 2.f)
	{
		return;
	}
	const FVector Axis = Delta / Length;
	FVector Wide = Piece.Lower.WideAxis - Axis * FVector::DotProduct(Piece.Lower.WideAxis, Axis);
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
	const int32 SideCount = ShoulderSideCount;
	const int32 Base = Verts.Num();
	for (int32 Ring = 0; Ring < 3; ++Ring)
	{
		const float Radius = Radii[Ring];
		const float Flat = FMath::Clamp(Squash[Ring], 0.2f, 1.f);
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(SideCount);
			const FVector Offset = Wide * (FMath::Cos(Angle) * Radius)
				+ Up * (FMath::Sin(Angle) * Radius * Flat);
			FSolidBodyVert Vert;
			Vert.Position = Centers[Ring] + Offset;
			Vert.Bone = BoneIndex;
			Vert.Slot = Slot;
			Verts.Add(Vert);
		}
	}

	auto Id = [Base, SideCount](const int32 Ring, const int32 Step)
	{
		return Base + Ring * SideCount + (Step % SideCount);
	};
	for (int32 Ring = 0; Ring < 2; ++Ring)
	{
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const int32 Next = Step + 1;
			AddTri(Tris, Id(Ring, Step), Id(Ring, Next), Id(Ring + 1, Step));
			AddTri(Tris, Id(Ring, Next), Id(Ring + 1, Next), Id(Ring + 1, Step));
		}
	}
	for (int32 Step = 1; Step < SideCount - 1; ++Step)
	{
		AddTri(Tris, Id(0, 0), Id(0, Step + 1), Id(0, Step));
		AddTri(Tris, Id(2, 0), Id(2, Step), Id(2, Step + 1));
	}
}

void SolidBodyMesh::FSolidBodyPart::AddHead(
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

void SolidBodyMesh::FSolidBodyPart::AddHand(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount) const
{
	if (BoneIndex == INDEX_NONE)
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
		Vert.Bone = BoneIndex;
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
	switch (Measure)
	{
	case ESolidPartMeasure::Head:
		AddHead(Verts, Tris, SpanBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
		break;
	case ESolidPartMeasure::Hand:
		AddHand(Verts, Tris, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount);
		break;
	case ESolidPartMeasure::Shoulder:
		AddShoulder(Verts, Tris, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
		break;
	default:
		AddTube(Verts, Tris, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
		break;
	}
}
