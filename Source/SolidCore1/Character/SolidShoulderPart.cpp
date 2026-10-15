#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

SolidBodyMesh::FSolidShoulderPart::FSolidShoulderPart(
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

TUniquePtr<SolidBodyMesh::FSolidBodyPart> SolidBodyMesh::FSolidShoulderPart::Clone() const
{
	return MakeUnique<FSolidShoulderPart>(*this);
}

TArray<FName> SolidBodyMesh::FSolidShoulderPart::PartBones() const
{
	TArray<FName> Names;
	Names.Add(ShoulderNeckBone);
	Names.Add(ShoulderLeftClavicleBone);
	Names.Add(ShoulderRightClavicleBone);
	Names.Add(ShoulderLeftUpperArmBone);
	Names.Add(ShoulderRightUpperArmBone);
	return Names;
}

void SolidBodyMesh::FSolidShoulderPart::Apply(const FSolidBodyParams& Params)
{
	const FSolidBodyParams Clamped = Params.Clamped();
	const float SideScale = Clamped.ShoulderWidth;
	const float Depth = Clamped.TorsoGirth;
	Start.X *= SideScale;
	Start.Y *= Depth;
	End.X *= SideScale;
	End.Y *= SideScale;
	Mid.X *= SideScale;
	Mid.Y *= Depth;
	SizeBones(Clamped.NeckLength, SideScale);
}

SolidBodyMesh::FSolidPartDraw SolidBodyMesh::FSolidShoulderPart::Fit(
	const FVector& From,
	const FVector& To,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const FVector& WideAxis)
{
	FSolidPartDraw Out;
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

void SolidBodyMesh::FSolidShoulderPart::DrawShoulder(
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

void SolidBodyMesh::FSolidShoulderPart::Draw(
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
	DrawShoulder(Verts, Tris, WeightBone, From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
}
