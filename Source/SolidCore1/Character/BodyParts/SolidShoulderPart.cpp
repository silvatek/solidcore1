#include "SolidBodyPart.h"
#include "SolidBodyMesh.h"

namespace
{
	bool ShoulderFrame(
		const FVector& Start,
		const FVector& End,
		const FVector& PreferredSide,
		FVector& OutAxis,
		FVector& OutWide,
		FVector& OutNarrow,
		float& OutLength)
	{
		const FVector Delta = End - Start;
		OutLength = Delta.Size();
		if (OutLength < 2.f)
		{
			return false;
		}
		OutAxis = Delta / OutLength;
		OutWide = PreferredSide - OutAxis * FVector::DotProduct(PreferredSide, OutAxis);
		if (OutWide.SizeSquared() < 0.01)
		{
			OutWide = FVector::CrossProduct(FVector::UpVector, OutAxis);
		}
		if (OutWide.SizeSquared() < 0.01)
		{
			OutWide = FVector::CrossProduct(FVector::ForwardVector, OutAxis);
		}
		if (!OutWide.Normalize())
		{
			return false;
		}
		OutNarrow = FVector::CrossProduct(OutAxis, OutWide).GetSafeNormal();
		return !OutNarrow.IsNearlyZero();
	}

	float ShoulderChord()
	{
		return FMath::Cos(PI / static_cast<float>(SolidBodyMesh::ShoulderSideCount));
	}
}

SolidBodyMesh::FSolidShoulderPiece SolidBodyMesh::ShoulderPiece(
	const FVector& NeckStart,
	const FVector& NeckEnd,
	const FVector* SegmentStart,
	const FVector* SegmentEnd,
	const int32 SegmentCount,
	const float BaseSideDiameterCm,
	const FVector& WideAxis,
	const float BaseDepthDiameterCm,
	const float TopDiameterCm)
{
	FSolidShoulderPiece Out;
	FVector Axis = FVector::UpVector;
	FVector Wide = FVector::ForwardVector;
	FVector Narrow = FVector::RightVector;
	float Length = 0.f;
	if (!ShoulderFrame(NeckStart, NeckEnd, WideAxis, Axis, Wide, Narrow, Length)
		|| BaseSideDiameterCm <= KINDA_SMALL_NUMBER
		|| BaseDepthDiameterCm <= KINDA_SMALL_NUMBER
		|| TopDiameterCm <= KINDA_SMALL_NUMBER)
	{
		return Out;
	}

	const float SideBottom = BaseSideDiameterCm * 0.5f;
	const float DepthBottom = BaseDepthDiameterCm * 0.5f;
	const float SideTop = TopDiameterCm * 0.5f;
	const float DepthTop = TopDiameterCm * 0.5f;
	const float Chord = ShoulderChord();
	if (Chord <= KINDA_SMALL_NUMBER)
	{
		return Out;
	}

	struct FSample
	{
		float U = 0.f;
		float Need = 1.f;
	};
	TArray<FSample> Samples;
	const int32 SafeCount = FMath::Max(SegmentCount, 0);
	for (int32 Segment = 0; Segment < SafeCount; ++Segment)
	{
		if (!SegmentStart || !SegmentEnd)
		{
			break;
		}
		for (int32 Step = 0; Step < 8; ++Step)
		{
			const float T = static_cast<float>(Step) / 7.f;
			const FVector Point = FMath::Lerp(SegmentStart[Segment], SegmentEnd[Segment], T);
			const FVector Rel = Point - NeckStart;
			const float Along = FVector::DotProduct(Rel, Axis);
			const float U = FMath::Clamp(Along / Length, 0.f, 1.f);
			const float WideDist = FMath::Abs(FVector::DotProduct(Rel, Wide));
			const float NarrowDist = FMath::Abs(FVector::DotProduct(Rel, Narrow));
			const float Radial = FMath::Sqrt(FMath::Square(WideDist) + FMath::Square(NarrowDist));
			float WideUse = WideDist;
			float NarrowUse = NarrowDist;
			if (Radial > 1.f && ShoulderUpperArmClearanceCm > 0.f)
			{
				const float Grown = (Radial + ShoulderUpperArmClearanceCm) / Radial;
				WideUse *= Grown;
				NarrowUse *= Grown;
			}
			const float SideHere = FMath::Lerp(SideBottom, SideTop, U);
			const float DepthHere = FMath::Lerp(DepthBottom, DepthTop, U);
			if (SideHere <= KINDA_SMALL_NUMBER || DepthHere <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const float Ellipse = FMath::Sqrt(
				FMath::Square(WideUse / SideHere) + FMath::Square(NarrowUse / DepthHere));
			FSample Sample;
			Sample.U = U;
			Sample.Need = Ellipse / Chord;
			Samples.Add(Sample);
		}
	}

	float PeakU = 0.5f;
	float PeakNeed = 1.f;
	for (const FSample& Sample : Samples)
	{
		if (Sample.Need > PeakNeed)
		{
			PeakNeed = Sample.Need;
			PeakU = Sample.U;
		}
	}
	const float MinU = FMath::Clamp(1.f / Length, 0.05f, 0.45f);
	const float MidU = FMath::Clamp(PeakU, MinU, 1.f - MinU);

	float Multiplier = 1.f;
	for (const FSample& Sample : Samples)
	{
		if (Sample.Need <= 1.f)
		{
			continue;
		}
		if (Sample.U <= MidU)
		{
			const float Denom = FMath::Max(Sample.U, 0.02f);
			Multiplier = FMath::Max(Multiplier, 1.f + (Sample.Need - 1.f) * (MidU / Denom));
		}
		else
		{
			const float Frac = (Sample.U - MidU) / FMath::Max(1.f - MidU, 0.02f);
			const float Remain = 1.f - Frac;
			if (Remain > 0.02f)
			{
				Multiplier = FMath::Max(Multiplier, (Sample.Need - Frac) / Remain);
			}
		}
	}

	const FVector MidPoint = FMath::Lerp(NeckStart, NeckEnd, MidU);
	const float SideMid = Multiplier * FMath::Lerp(SideBottom, SideTop, MidU);
	const float DepthMid = Multiplier * FMath::Lerp(DepthBottom, DepthTop, MidU);
	auto ScaleOf = [](const float Side, const float Depth)
	{
		return Side > KINDA_SMALL_NUMBER ? Depth / Side : 1.f;
	};

	Out.Lower.Start = NeckStart;
	Out.Lower.End = MidPoint;
	Out.Lower.RadiusStart = SideBottom;
	Out.Lower.RadiusEnd = SideMid;
	Out.Lower.DepthScaleStart = ScaleOf(SideBottom, DepthBottom);
	Out.Lower.DepthScaleEnd = ScaleOf(SideMid, DepthMid);
	Out.Lower.WideAxis = WideAxis;
	Out.Lower.bValid = FVector::Dist(NeckStart, MidPoint) >= 1.0;

	Out.Upper.Start = MidPoint;
	Out.Upper.End = NeckEnd;
	Out.Upper.RadiusStart = SideMid;
	Out.Upper.RadiusEnd = SideTop;
	Out.Upper.DepthScaleStart = ScaleOf(SideMid, DepthMid);
	Out.Upper.DepthScaleEnd = ScaleOf(SideTop, DepthTop);
	Out.Upper.WideAxis = WideAxis;
	Out.Upper.bValid = FVector::Dist(MidPoint, NeckEnd) >= 1.0;
	Out.bValid = Out.Lower.bValid && Out.Upper.bValid;
	return Out;
}

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

FString SolidBodyMesh::FSolidShoulderPart::Name() const
{
	return WithSide(TEXT("Shoulders"));
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
	const FSolidPartDraw Fitted = Fit(From, To, SegmentStart, SegmentEnd, SegmentCount, WideAxis);
	const FSolidShoulderPiece& Piece = Fitted.Shoulder;
	const uint8 Slot = MaterialSlot();
	if (!Piece.bValid || WeightBone == INDEX_NONE)
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
			Vert.Bone = WeightBone;
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

bool SolidBodyMesh::FSolidShoulderPart::LowestZ(
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
	const FSolidShoulderPiece Piece = ShoulderPiece(
		From,
		To,
		SegmentStart,
		SegmentEnd,
		SegmentCount,
		Start.X,
		WideAxis,
		Start.Y,
		End.X);
	if (!Piece.bValid)
	{
		return false;
	}
	float LowerZ = 0.f;
	float UpperZ = 0.f;
	if (!ConeLowestZ(Piece.Lower, LowerZ) || !ConeLowestZ(Piece.Upper, UpperZ))
	{
		return false;
	}
	OutZ = FMath::Min(LowerZ, UpperZ);
	return true;
}
