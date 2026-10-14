#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

#include <initializer_list>

uint8 SolidBodyMesh::FSolidBodyPartMesh::MaterialSlot() const
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

namespace
{
	SolidBodyMesh::FSolidBodyPartMesh Part(
		const TCHAR* Name,
		std::initializer_list<const TCHAR*> Bones,
		const SolidBodyMesh::ESolidPartColour Colour,
		const SolidBodyMesh::FSolidPartSection& Start,
		const SolidBodyMesh::FSolidPartSection& End)
	{
		SolidBodyMesh::FSolidBodyPartMesh Out;
		Out.Name = Name;
		for (const TCHAR* Bone : Bones)
		{
			Out.Bones.Add(Bone);
		}
		Out.Colour = Colour;
		Out.Start = Start;
		Out.End = End;
		return Out;
	}

	FString SpinePartName(const int32 Step, const int32 Count)
	{
		if (Step == Count - 1)
		{
			return TEXT("Chest");
		}
		if (Step == 0)
		{
			return TEXT("Abdomen");
		}
		return FString::Printf(TEXT("Spine %d"), Step + 1);
	}
}

SolidBodyMesh::FSolidBodyMesh SolidBodyMesh::FSolidBodyMesh::Make(const TArray<FName>& RisingSpineBones)
{
	using SolidBodyMesh::ESolidPartColour;
	using SolidBodyMesh::FSolidPartSection;

	FSolidBodyMesh Body;
	const FSolidPartSection Round20 = FSolidPartSection::Round(LeftThighTopDiameterCm);
	Body.Parts.Add(Part(TEXT("Thigh (L)"), { LeftThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Thigh (R)"), { RightThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Calf (L)"), { LeftCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Calf (R)"), { RightCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)));
	Body.Parts.Add(Part(TEXT("Foot (L)"), { LeftBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)));
	Body.Parts.Add(Part(TEXT("Foot (R)"), { RightBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)));
	Body.Parts.Add(Part(TEXT("Toe (L)"), { LeftExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)));
	Body.Parts.Add(Part(TEXT("Toe (R)"), { RightExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)));

	// The hip mesh is the length of spine_01. The thighs are inside it.
	Body.Parts.Add(Part(
		TEXT("Hip"),
		{ HipSpineBone, HipLeftBone, HipRightBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(HipSideDiameterCm, HipDepthDiameterCm),
		FSolidPartSection::Oval(HipTopSideDiameterCm, HipDepthDiameterCm)));

	const int32 SpineCount = RisingSpineBones.Num();
	for (int32 Step = 0; Step < SpineCount; ++Step)
	{
		float BottomCm = 0.f;
		float TopCm = 0.f;
		TorsoSideDiameters(Step, SpineCount, BottomCm, TopCm);
		FSolidBodyPartMesh Spine = Part(
			TEXT("Spine"),
			{},
			ESolidPartColour::Shirt,
			FSolidPartSection::Oval(BottomCm, TorsoDepthDiameterCm),
			FSolidPartSection::Oval(TopCm, TorsoDepthDiameterCm));
		Spine.Name = SpinePartName(Step, SpineCount);
		Spine.Bones.Add(RisingSpineBones[Step]);
		Body.Parts.Add(Spine);
	}

	float ShoulderSideCm = ChestSideDiameterCm;
	for (const FSolidBodyPartMesh& Spine : Body.Parts)
	{
		if (Spine.Bones.Num() == 1 && Spine.Bones[0] == FName(SpineChestBone))
		{
			ShoulderSideCm = Spine.End.X;
		}
	}

	// Shoulders follow neck_01. Mid is fitted around the clavicles and the upper arms when the mesh is built.
	FSolidBodyPartMesh Shoulders = Part(
		TEXT("Shoulders"),
		{ ShoulderNeckBone, ShoulderLeftClavicleBone, ShoulderRightClavicleBone, ShoulderLeftUpperArmBone, ShoulderRightUpperArmBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(ShoulderSideCm, TorsoDepthDiameterCm),
		FSolidPartSection::Round(ShoulderTopRadiusCm * 2.f));
	Shoulders.bHasMid = true;
	Body.Parts.Add(Shoulders);

	// upperarm joint to elbow. The skeleton names that bone lowerarm.
	Body.Parts.Add(Part(
		TEXT("Upper arm (L)"),
		{ TEXT("lowerarm_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)));
	Body.Parts.Add(Part(
		TEXT("Upper arm (R)"),
		{ TEXT("lowerarm_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)));

	// elbow to wrist. The skeleton names that bone hand. Narrow at the elbow, wide at the wrist.
	Body.Parts.Add(Part(
		TEXT("Lower arm (L)"),
		{ TEXT("hand_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)));
	Body.Parts.Add(Part(
		TEXT("Lower arm (R)"),
		{ TEXT("hand_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)));

	return Body;
}

const SolidBodyMesh::FSolidBodyPartMesh* SolidBodyMesh::FSolidBodyMesh::Find(const FString& Name) const
{
	for (const FSolidBodyPartMesh& Part : Parts)
	{
		if (Part.Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return &Part;
		}
	}
	return nullptr;
}

namespace
{
	int32 FindNamedBone(const TArray<FSolidDeformBone>& Bones, const int32 Count, const TCHAR* BoneName)
	{
		const int32 Limit = FMath::Min(Count, Bones.Num());
		for (int32 Index = 0; Index < Limit; ++Index)
		{
			if (Bones[Index].Name.ToString().Equals(BoneName, ESearchCase::IgnoreCase))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	int32 ChildNamed(
		const TArray<FSolidDeformBone>& Bones,
		const int32 Parent,
		bool (*Test)(const FString&))
	{
		if (!Bones.IsValidIndex(Parent) || !Test)
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Bones[Index].ParentIndex != Parent)
			{
				continue;
			}
			if (Test(Bones[Index].Name.ToString()))
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}

SolidBodyMesh::FSolidPartDraw SolidBodyMesh::FSolidBodyPartMesh::Fit(
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
		Out.Shoulder = ShoulderPiece(From, To, SegmentStart, SegmentEnd, SegmentCount, Start.X, WideAxis);
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

void SolidBodyMesh::FSolidBodyPartMesh::AddTri(
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

void SolidBodyMesh::FSolidBodyPartMesh::AddTube(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FVector& TubeStart,
	const FVector& TubeEnd,
	const float RadiusStart,
	const float RadiusEnd,
	const int32 Sides,
	const int32 Segments,
	const uint8 Slot,
	const float FlattenStart,
	const float FlattenEnd,
	const FVector& PreferredSide) const
{
	const FVector Delta = TubeEnd - TubeStart;
	const float Length = Delta.Size();
	if (Length < 1.f || BoneIndex == INDEX_NONE || Sides < 3)
	{
		return;
	}

	const FVector Axis = Delta / Length;
	FVector Side = PreferredSide - Axis * FVector::DotProduct(PreferredSide, Axis);
	if (Side.SizeSquared() < 0.01)
	{
		Side = FVector::CrossProduct(FVector::UpVector, Axis);
	}
	if (Side.SizeSquared() < 0.01)
	{
		Side = FVector::CrossProduct(FVector::ForwardVector, Axis);
	}
	Side = Side.GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Axis, Side).GetSafeNormal();
	const int32 SideCount = FMath::Clamp(Sides, 3, 16);
	const int32 RingCount = FMath::Max(Segments, 1) + 1;
	const int32 Base = Verts.Num();

	for (int32 Ring = 0; Ring < RingCount; ++Ring)
	{
		const float T = (RingCount == 1)
			? 0.f
			: static_cast<float>(Ring) / static_cast<float>(RingCount - 1);
		const FVector Center = FMath::Lerp(TubeStart, TubeEnd, static_cast<double>(T));
		const float Radius = FMath::Lerp(RadiusStart, RadiusEnd, T);
		const float Squash = FMath::Lerp(
			FMath::Clamp(FlattenStart, 0.2f, 1.f),
			FMath::Clamp(FlattenEnd, 0.2f, 1.f),
			T);
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(SideCount);
			const FVector Offset = Side * (FMath::Cos(Angle) * Radius)
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

void SolidBodyMesh::FSolidBodyPartMesh::AddShoulder(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 BoneIndex,
	const FSolidShoulderPiece& Piece,
	const uint8 Slot) const
{
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
	FVector Side = Piece.Lower.WideAxis - Axis * FVector::DotProduct(Piece.Lower.WideAxis, Axis);
	if (Side.SizeSquared() < 0.01)
	{
		Side = FVector::CrossProduct(FVector::UpVector, Axis);
	}
	if (Side.SizeSquared() < 0.01)
	{
		Side = FVector::CrossProduct(FVector::ForwardVector, Axis);
	}
	Side = Side.GetSafeNormal();
	const FVector Up = FVector::CrossProduct(Axis, Side).GetSafeNormal();
	const int32 SideCount = ShoulderSideCount;
	const int32 Base = Verts.Num();
	for (int32 Ring = 0; Ring < 3; ++Ring)
	{
		const float Radius = Radii[Ring];
		const float Flat = FMath::Clamp(Squash[Ring], 0.2f, 1.f);
		for (int32 Step = 0; Step < SideCount; ++Step)
		{
			const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(SideCount);
			const FVector Offset = Side * (FMath::Cos(Angle) * Radius)
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

void SolidBodyMesh::FSolidBodyPartMesh::Draw(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const int32 WeightBone,
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	const FSolidPartDraw Fitted = Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	if (Fitted.bShoulder)
	{
		AddShoulder(Verts, Tris, WeightBone, Fitted.Shoulder, MaterialSlot());
		return;
	}

	const FSolidFittedCone& Cone = Fitted.Cone;
	if (!Cone.bValid)
	{
		return;
	}
	AddTube(
		Verts,
		Tris,
		WeightBone,
		Cone.Start,
		Cone.End,
		Cone.RadiusStart,
		Cone.RadiusEnd,
		10,
		1,
		MaterialSlot(),
		Cone.DepthScaleStart,
		Cone.DepthScaleEnd,
		Cone.WideAxis);
}

SolidBodyMesh::FSolidBodyMesh SolidBodyMesh::FSolidBodyMesh::FromBones(const TArray<FSolidDeformBone>& Bones)
{
	TArray<FName> RisingNames;
	const int32 HipSpine = FindNamedBone(Bones, Bones.Num(), HipSpineBone);
	if (HipSpine != INDEX_NONE)
	{
		int32 Cursor = HipSpine;
		for (int32 Guard = 0; Guard < Bones.Num(); ++Guard)
		{
			const int32 Next = ChildNamed(Bones, Cursor, IsRisingSpineBoneName);
			if (Next == INDEX_NONE)
			{
				break;
			}
			RisingNames.Add(Bones[Next].Name);
			Cursor = Next;
		}
	}
	return Make(RisingNames);
}

void SolidBodyMesh::FSolidBodyMesh::Draw(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris,
	const TArray<FSolidDeformBone>& Bones,
	const TArray<FVector>& Positions)
{
	// shoulder_jnt, fingers, head, and eye bones stay bare. Twists stay bare too.
	const int32 Count = FMath::Min(Bones.Num(), Positions.Num());
	for (FSolidBodyPartMesh& Part : Parts)
	{
		if (Part.Bones.Num() == 0)
		{
			continue;
		}
		const int32 Span = FindNamedBone(Bones, Count, *Part.Bones[0].ToString());
		if (Span == INDEX_NONE)
		{
			continue;
		}
		const int32 Parent = Bones[Span].ParentIndex;
		if (!Bones.IsValidIndex(Parent) || !Positions.IsValidIndex(Parent))
		{
			continue;
		}

		TArray<FVector> SegmentStart;
		TArray<FVector> SegmentEnd;
		bool bComplete = true;
		for (int32 BoneNumber = 1; BoneNumber < Part.Bones.Num(); ++BoneNumber)
		{
			const int32 Bone = FindNamedBone(Bones, Count, *Part.Bones[BoneNumber].ToString());
			if (Bone == INDEX_NONE)
			{
				bComplete = false;
				break;
			}
			const int32 BoneParent = Bones[Bone].ParentIndex;
			if (!Bones.IsValidIndex(BoneParent) || !Positions.IsValidIndex(BoneParent))
			{
				bComplete = false;
				break;
			}
			SegmentStart.Add(Positions[BoneParent]);
			SegmentEnd.Add(Positions[Bone]);
		}
		if (!bComplete)
		{
			continue;
		}

		Part.Draw(
			Verts,
			Tris,
			Parent,
			Positions[Parent],
			Positions[Span],
			SegmentStart.GetData(),
			SegmentEnd.GetData(),
			SegmentStart.Num(),
			FVector::ForwardVector);
	}
}
