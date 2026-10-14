#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

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

void SolidBodyMesh::FSolidBodyPart::AddShoulder(
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

void SolidBodyMesh::FSolidBodyPart::Draw(
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
