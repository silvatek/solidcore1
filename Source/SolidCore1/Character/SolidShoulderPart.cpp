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
