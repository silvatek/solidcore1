#include "SolidBodyMesh.h"

#include "SolidContentLookup.h"
#include <initializer_list>
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "SolidSkeletonHover.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ReferenceSkeleton.h"

#if WITH_EDITORONLY_DATA
#include "BoneWeights.h"
#include "MeshDescription.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalMeshAttributes.h"
#endif

namespace
{
	bool NameHas(const FString& Lower, const TCHAR* Token)
	{
		return Lower.Contains(Token);
	}

	bool RoleScalesLength(const ESolidBodyBoneRole Role)
	{
		switch (Role)
		{
		case ESolidBodyBoneRole::Spine:
		case ESolidBodyBoneRole::Neck:
		case ESolidBodyBoneRole::Clavicle:
		case ESolidBodyBoneRole::Arm:
		case ESolidBodyBoneRole::Hand:
		case ESolidBodyBoneRole::Leg:
		case ESolidBodyBoneRole::Foot:
			return true;
		default:
			return false;
		}
	}
}

uint32 SolidBodyMesh::SeedForName(const FString& Name)
{
	const FString Lower = Name.ToLower();
	return FCrc::StrCrc32(*Lower);
}

ESolidBodyBoneRole SolidBodyMesh::RoleForBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	if (NameHas(Lower, TEXT("ik_")) || Lower.StartsWith(TEXT("ik")))
	{
		return ESolidBodyBoneRole::Root;
	}
	if (NameHas(Lower, TEXT("pelvis")) || NameHas(Lower, TEXT("hips")))
	{
		return ESolidBodyBoneRole::Pelvis;
	}
	if (NameHas(Lower, TEXT("neck")))
	{
		return ESolidBodyBoneRole::Neck;
	}
	if (NameHas(Lower, TEXT("head")))
	{
		return ESolidBodyBoneRole::Head;
	}
	if (NameHas(Lower, TEXT("clavicle")) || NameHas(Lower, TEXT("shoulder")))
	{
		return ESolidBodyBoneRole::Clavicle;
	}
	if (NameHas(Lower, TEXT("finger")) || NameHas(Lower, TEXT("thumb")) || NameHas(Lower, TEXT("hand")))
	{
		return ESolidBodyBoneRole::Hand;
	}
	if (NameHas(Lower, TEXT("arm")))
	{
		return ESolidBodyBoneRole::Arm;
	}
	if (NameHas(Lower, TEXT("toe")) || NameHas(Lower, TEXT("foot"))
		|| NameHas(Lower, TEXT("calf")) || NameHas(Lower, TEXT("shin")))
	{
		return ESolidBodyBoneRole::Foot;
	}
	if (NameHas(Lower, TEXT("thigh")) || NameHas(Lower, TEXT("upperleg")) || NameHas(Lower, TEXT("leg")))
	{
		return ESolidBodyBoneRole::Leg;
	}
	if (NameHas(Lower, TEXT("spine")) || NameHas(Lower, TEXT("chest")) || NameHas(Lower, TEXT("torso")))
	{
		return ESolidBodyBoneRole::Spine;
	}
	return ESolidBodyBoneRole::Root;
}

bool SolidBodyMesh::IsTwistBoneName(const FString& BoneName)
{
	return BoneName.ToLower().Contains(TEXT("twist"));
}

bool SolidBodyMesh::IsDigitBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.Contains(TEXT("finger"))
		|| Lower.Contains(TEXT("thumb"))
		|| Lower.Contains(TEXT("toe"))
		|| Lower.Contains(TEXT("ball"));
}

float SolidBodyMesh::HipBottomRadius(const float PelvisHalfWidth, const float HipLateral, const float ThighRadius)
{
	return FMath::Max(PelvisHalfWidth, FMath::Max(0.f, HipLateral) + FMath::Max(0.f, ThighRadius));
}

namespace
{
	bool IsLeftName(const FString& Lower)
	{
		return Lower.EndsWith(TEXT("_l")) || Lower.Contains(TEXT("_l_"));
	}

	bool IsRightName(const FString& Lower)
	{
		return Lower.EndsWith(TEXT("_r")) || Lower.Contains(TEXT("_r_"));
	}

	FString SidedKey(const FString& Lower, const TCHAR* Kind)
	{
		if (IsLeftName(Lower))
		{
			return FString(Kind) + TEXT("_l");
		}
		if (IsRightName(Lower))
		{
			return FString(Kind) + TEXT("_r");
		}
		return FString();
	}
}

FString SolidBodyMesh::SegmentKey(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	if (Lower == TEXT("head") || Lower.StartsWith(TEXT("eye_")) || Lower.StartsWith(TEXT("eyelid_")))
	{
		return TEXT("head");
	}
	if (Lower.StartsWith(TEXT("neck_")))
	{
		return TEXT("neck");
	}
	if (Lower.StartsWith(TEXT("spine_01")) || Lower.StartsWith(TEXT("spine_02")) || Lower.StartsWith(TEXT("spine_03")))
	{
		return Lower.Left(8);
	}
	if (Lower.StartsWith(TEXT("clavicle_"))
		|| Lower.StartsWith(TEXT("shoulder_jnt"))
		|| (Lower.StartsWith(TEXT("upperarm_")) && !Lower.Contains(TEXT("twist"))))
	{
		return SidedKey(Lower, TEXT("shoulder"));
	}
	if (Lower.StartsWith(TEXT("thigh_")))
	{
		return SidedKey(Lower, TEXT("thigh"));
	}
	if (Lower.StartsWith(TEXT("calf_")))
	{
		return SidedKey(Lower, TEXT("calf"));
	}
	if (Lower.StartsWith(TEXT("foot_")) || Lower.StartsWith(TEXT("ball_")) || Lower.StartsWith(TEXT("extra_finger_")))
	{
		return SidedKey(Lower, TEXT("foot"));
	}
	if (Lower.StartsWith(TEXT("lowerarm_")))
	{
		return SidedKey(Lower, TEXT("forearm"));
	}
	if (Lower.StartsWith(TEXT("hand_"))
		|| Lower.StartsWith(TEXT("index_"))
		|| Lower.StartsWith(TEXT("middle_"))
		|| Lower.StartsWith(TEXT("ring_"))
		|| Lower.StartsWith(TEXT("pinky_"))
		|| Lower.StartsWith(TEXT("thumb_")))
	{
		return SidedKey(Lower, TEXT("hand"));
	}
	return FString();
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::FitCone(const FVector* Points, const int32 Count)
{
	constexpr float Pad = 0.75f;
	constexpr float MinSpan = 1.01f;
	SolidBodyMesh::FSolidFittedCone Out;
	if (!Points || Count <= 0)
	{
		return Out;
	}

	FVector Centroid = FVector::ZeroVector;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Centroid += Points[Index];
	}
	Centroid /= static_cast<double>(Count);

	int32 FarA = 0;
	int32 FarB = 0;
	double Best = -1.0;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		for (int32 Other = Index + 1; Other < Count; ++Other)
		{
			const double DistSq = FVector::DistSquared(Points[Index], Points[Other]);
			if (DistSq > Best)
			{
				Best = DistSq;
				FarA = Index;
				FarB = Other;
			}
		}
	}

	FVector Axis = FVector::UpVector;
	if (Best > 0.01)
	{
		Axis = (Points[FarB] - Points[FarA]).GetSafeNormal();
	}

	double TMin = TNumericLimits<double>::Max();
	double TMax = TNumericLimits<double>::Lowest();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const double Along = FVector::DotProduct(Points[Index] - Centroid, Axis);
		TMin = FMath::Min(TMin, Along);
		TMax = FMath::Max(TMax, Along);
	}

	double Span = TMax - TMin;
	if (Span < static_cast<double>(MinSpan))
	{
		const double Grow = (static_cast<double>(MinSpan) - Span) * 0.5;
		TMin -= Grow;
		TMax += Grow;
		Span = static_cast<double>(MinSpan);
	}

	const FVector Start = Centroid + Axis * TMin;
	const FVector End = Centroid + Axis * TMax;

	double RadiusStart = Pad;
	double RadiusEnd = Pad;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Rel = Points[Index] - Start;
		const double Along = FVector::DotProduct(Rel, Axis);
		const double Radial = (Rel - Axis * Along).Size();
		const double U = Along / Span;
		if (U < 0.5)
		{
			RadiusStart = FMath::Max(RadiusStart, Radial + static_cast<double>(Pad));
		}
		else
		{
			RadiusEnd = FMath::Max(RadiusEnd, Radial + static_cast<double>(Pad));
		}
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Rel = Points[Index] - Start;
		const double Along = FVector::DotProduct(Rel, Axis);
		const double Radial = (Rel - Axis * Along).Size();
		const double U = FMath::Clamp(Along / Span, 0.0, 1.0);
		const double Have = FMath::Lerp(RadiusStart, RadiusEnd, U);
		if (Radial > Have)
		{
			const double Deficit = Radial - Have;
			RadiusStart += Deficit;
			RadiusEnd += Deficit;
		}
	}

	Out.Start = Start;
	Out.End = End;
	Out.RadiusStart = static_cast<float>(RadiusStart);
	Out.RadiusEnd = static_cast<float>(RadiusEnd);
	Out.bValid = true;
	return Out;
}

namespace
{
	SolidBodyMesh::FSolidFittedCone ConeFromNamedPart(
		const TCHAR* PartName,
		const FVector& From,
		const FVector& To)
	{
		const SolidBodyMesh::FSolidBodyMesh Body = SolidBodyMesh::FSolidBodyMesh::Make({});
		const SolidBodyMesh::FSolidBodyPartMesh* Found = Body.Find(PartName);
		SolidBodyMesh::FSolidFittedCone Out;
		if (!Found)
		{
			return Out;
		}
		Out.Start = From;
		Out.End = To;
		Out.RadiusStart = Found->Start.X * 0.5f;
		Out.RadiusEnd = Found->End.X * 0.5f;
		Out.DepthScaleStart = Found->Start.X > KINDA_SMALL_NUMBER ? Found->Start.Y / Found->Start.X : 1.f;
		Out.DepthScaleEnd = Found->End.X > KINDA_SMALL_NUMBER ? Found->End.Y / Found->End.X : 1.f;
		Out.WideAxis = FVector::ForwardVector;
		Out.bValid = FVector::Dist(From, To) >= 1.0
			&& Found->Start.X > KINDA_SMALL_NUMBER
			&& Found->End.X > KINDA_SMALL_NUMBER;
		return Out;
	}
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::LeftThighCone(const FVector& Hip, const FVector& Knee)
{
	return ConeFromNamedPart(TEXT("Thigh (L)"), Hip, Knee);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::CalfCone(const FVector& Knee, const FVector& Ankle)
{
	return ConeFromNamedPart(TEXT("Calf (L)"), Knee, Ankle);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::BallCylinder(const FVector& Ankle, const FVector& Toe)
{
	return ConeFromNamedPart(TEXT("Foot (L)"), Ankle, Toe);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::ExtraFingerCone(const FVector& AnkleEnd, const FVector& ToeEnd)
{
	return ConeFromNamedPart(TEXT("Toe (L)"), AnkleEnd, ToeEnd);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::HipCone(const FVector& SpineStart, const FVector& SpineEnd)
{
	return ConeFromNamedPart(TEXT("Hip"), SpineStart, SpineEnd);
}

bool SolidBodyMesh::IsRisingSpineBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("spine_"))
		&& !Lower.StartsWith(TEXT("spine_01"))
		&& !IsTwistBoneName(Lower);
}

bool SolidBodyMesh::IsClavicleBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("clavicle_")) && !IsTwistBoneName(Lower);
}

bool SolidBodyMesh::IsUpperArmBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("upperarm_")) && !IsTwistBoneName(Lower);
}

bool SolidBodyMesh::IsForearmBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("lowerarm_")) && !IsTwistBoneName(Lower);
}

bool SolidBodyMesh::IsHandBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("hand_")) && !IsTwistBoneName(Lower);
}

void SolidBodyMesh::TorsoSideDiameters(
	const int32 Step,
	const int32 Count,
	float& OutBottomCm,
	float& OutTopCm)
{
	if (Count < 1)
	{
		OutBottomCm = HipTopSideDiameterCm;
		OutTopCm = HipTopSideDiameterCm;
		return;
	}

	const int32 SafeStep = FMath::Clamp(Step, 0, Count - 1);
	const float Denom = static_cast<float>(Count);
	OutBottomCm = FMath::Lerp(HipTopSideDiameterCm, ChestSideDiameterCm, static_cast<float>(SafeStep) / Denom);
	OutTopCm = FMath::Lerp(HipTopSideDiameterCm, ChestSideDiameterCm, static_cast<float>(SafeStep + 1) / Denom);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::TorsoCone(
	const FVector& Bottom,
	const FVector& Top,
	const float SideBottomCm,
	const float SideTopCm)
{
	FSolidFittedCone Out;
	Out.Start = Bottom;
	Out.End = Top;
	Out.RadiusStart = SideBottomCm * 0.5f;
	Out.RadiusEnd = SideTopCm * 0.5f;
	Out.DepthScaleStart = SideBottomCm > KINDA_SMALL_NUMBER ? TorsoDepthDiameterCm / SideBottomCm : 1.f;
	Out.DepthScaleEnd = SideTopCm > KINDA_SMALL_NUMBER ? TorsoDepthDiameterCm / SideTopCm : 1.f;
	Out.WideAxis = FVector::ForwardVector;
	Out.bValid = FVector::Dist(Bottom, Top) >= 1.0
		&& SideBottomCm > KINDA_SMALL_NUMBER
		&& SideTopCm > KINDA_SMALL_NUMBER;
	return Out;
}

namespace
{
	SolidBodyMesh::FSolidFittedCone RoundBoneCone(
		const FVector& Parent,
		const FVector& Bone,
		const float DiameterParent,
		const float DiameterBone)
	{
		SolidBodyMesh::FSolidFittedCone Out;
		Out.Start = Parent;
		Out.End = Bone;
		Out.RadiusStart = DiameterParent * 0.5f;
		Out.RadiusEnd = DiameterBone * 0.5f;
		Out.bValid = FVector::Dist(Parent, Bone) >= 1.0;
		return Out;
	}
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::ClavicleCone(const FVector& Parent, const FVector& Bone)
{
	return RoundBoneCone(Parent, Bone, ClavicleRootDiameterCm, UpperArmShoulderDiameterCm);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::UpperArmCone(const FVector& Parent, const FVector& Bone)
{
	return RoundBoneCone(Parent, Bone, UpperArmShoulderDiameterCm, UpperArmElbowDiameterCm);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::ForearmCone(const FVector& Parent, const FVector& Bone)
{
	return ConeFromNamedPart(TEXT("Upper arm (L)"), Parent, Bone);
}

SolidBodyMesh::FSolidFittedCone SolidBodyMesh::HandCone(const FVector& Wrist, const FVector& Hand)
{
	return ConeFromNamedPart(TEXT("Lower arm (L)"), Wrist, Hand);
}

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
	const FVector& WideAxis)
{
	FSolidShoulderPiece Out;
	FVector Axis = FVector::UpVector;
	FVector Wide = FVector::ForwardVector;
	FVector Narrow = FVector::RightVector;
	float Length = 0.f;
	if (!ShoulderFrame(NeckStart, NeckEnd, WideAxis, Axis, Wide, Narrow, Length)
		|| BaseSideDiameterCm <= KINDA_SMALL_NUMBER
		|| ShoulderTopRadiusCm <= KINDA_SMALL_NUMBER)
	{
		return Out;
	}

	const float SideBottom = BaseSideDiameterCm * 0.5f;
	const float DepthBottom = TorsoDepthDiameterCm * 0.5f;
	const float SideTop = ShoulderTopRadiusCm;
	const float DepthTop = ShoulderTopRadiusCm;
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

bool SolidBodyMesh::ShoulderContains(const FSolidShoulderPiece& Piece, const FVector& Point)
{
	if (!Piece.bValid)
	{
		return false;
	}

	const FVector Start = Piece.Lower.Start;
	const FVector End = Piece.Upper.End;
	FVector Axis = End - Start;
	const float Length = Axis.Size();
	if (Length < 2.f || !Axis.Normalize())
	{
		return false;
	}
	const float Along = FVector::DotProduct(Point - Start, Axis);
	if (Along < -0.15f || Along > Length + 0.15f)
	{
		return false;
	}
	const float U = FMath::Clamp(Along / Length, 0.f, 1.f);
	const float MidU = FMath::Clamp(FVector::DotProduct(Piece.Lower.End - Start, Axis) / Length, 0.f, 1.f);

	const FSolidFittedCone& Cone = (U <= MidU) ? Piece.Lower : Piece.Upper;
	const FVector ConeDelta = Cone.End - Cone.Start;
	const float ConeLength = ConeDelta.Size();
	const float ConeU = ConeLength > KINDA_SMALL_NUMBER
		? FMath::Clamp(FVector::DotProduct(Point - Cone.Start, ConeDelta) / FMath::Square(ConeLength), 0.f, 1.f)
		: 0.f;
	const float SideRadius = FMath::Lerp(Cone.RadiusStart, Cone.RadiusEnd, ConeU);
	const float DepthScale = FMath::Lerp(Cone.DepthScaleStart, Cone.DepthScaleEnd, ConeU);
	const float DepthRadius = SideRadius * FMath::Clamp(DepthScale, 0.2f, 1.f);
	if (SideRadius <= KINDA_SMALL_NUMBER || DepthRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FVector Wide = Cone.WideAxis - Axis * FVector::DotProduct(Cone.WideAxis, Axis);
	if (!Wide.Normalize())
	{
		return false;
	}
	const FVector Narrow = FVector::CrossProduct(Axis, Wide).GetSafeNormal();
	const FVector Offset = Point - (Start + Axis * Along);
	const float WideDist = FVector::DotProduct(Offset, Wide);
	const float NarrowDist = FVector::DotProduct(Offset, Narrow);
	const float Ellipse = FMath::Square(WideDist / SideRadius) + FMath::Square(NarrowDist / DepthRadius);
	return Ellipse <= 1.f;
}

bool SolidBodyMesh::RayHitsCone(
	const FSolidFittedCone& Cone,
	const FVector& RayOrigin,
	const FVector& RayDir,
	float& InOutRayT)
{
	if (!Cone.bValid)
	{
		return false;
	}

	float RayT = 0.f;
	float SegU = 0.f;
	SolidSkeletonHover::RaySegmentDistance(RayOrigin, RayDir, Cone.Start, Cone.End, RayT, SegU);
	if (RayT >= InOutRayT)
	{
		return false;
	}

	const FVector Delta = Cone.End - Cone.Start;
	FVector Axis = Delta;
	if (!Axis.Normalize())
	{
		return false;
	}
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
	const float SideRadius = FMath::Lerp(Cone.RadiusStart, Cone.RadiusEnd, SegU) + 0.15f;
	const float DepthScale = FMath::Lerp(Cone.DepthScaleStart, Cone.DepthScaleEnd, SegU);
	const float DepthRadius = FMath::Lerp(Cone.RadiusStart, Cone.RadiusEnd, SegU) * FMath::Clamp(DepthScale, 0.2f, 1.f) + 0.15f;
	if (SideRadius <= KINDA_SMALL_NUMBER || DepthRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector Offset = (RayOrigin + RayDir * RayT) - (Cone.Start + Delta * SegU);
	const float WideDist = FVector::DotProduct(Offset, Wide);
	const float NarrowDist = FVector::DotProduct(Offset, Narrow);
	const float Ellipse = FMath::Square(WideDist / SideRadius) + FMath::Square(NarrowDist / DepthRadius);
	if (Ellipse > 1.f)
	{
		return false;
	}

	InOutRayT = RayT;
	return true;
}

namespace
{
	bool ConeIsStructural(const FSolidDeformBone& Bone)
	{
		return !Bone.bTwist && !Bone.bDigit;
	}

	int32 ConeStructuralParent(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		int32 Parent = Bones.IsValidIndex(Index) ? Bones[Index].ParentIndex : INDEX_NONE;
		int32 Guard = 0;
		while (Bones.IsValidIndex(Parent) && Bones[Parent].bTwist && Guard < Bones.Num())
		{
			Parent = Bones[Parent].ParentIndex;
			++Guard;
		}
		return Parent;
	}

	int32 ConeStructuralChild(
		const TArray<FSolidDeformBone>& Bones,
		const int32 Parent,
		const ESolidBodyBoneRole Role)
	{
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Bones[Index].ParentIndex != Parent)
			{
				continue;
			}
			if (Bones[Index].bTwist)
			{
				const int32 Deeper = ConeStructuralChild(Bones, Index, Role);
				if (Deeper != INDEX_NONE)
				{
					return Deeper;
				}
				continue;
			}
			if (Bones[Index].Role == Role && !Bones[Index].bDigit)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	float HorizontalReach(const FVector& From, const FVector& To)
	{
		FVector Delta = To - From;
		Delta.Z = 0.0;
		return static_cast<float>(Delta.Size());
	}

	/** Longest structural spine chain whose root hangs off a pelvis. */
	void CollectConeSpine(const TArray<FSolidDeformBone>& Bones, TArray<int32>& OutChain)
	{
		OutChain.Reset();
		int32 BestRoot = INDEX_NONE;
		int32 BestLength = 0;
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (!ConeIsStructural(Bones[Index]) || Bones[Index].Role != ESolidBodyBoneRole::Spine)
			{
				continue;
			}
			const int32 Parent = ConeStructuralParent(Bones, Index);
			if (!Bones.IsValidIndex(Parent) || Bones[Parent].Role != ESolidBodyBoneRole::Pelvis)
			{
				continue;
			}
			int32 Length = 0;
			int32 Cursor = Index;
			int32 Guard = 0;
			while (Cursor != INDEX_NONE && Guard < Bones.Num())
			{
				++Length;
				Cursor = ConeStructuralChild(Bones, Cursor, ESolidBodyBoneRole::Spine);
				++Guard;
			}
			if (Length > BestLength)
			{
				BestLength = Length;
				BestRoot = Index;
			}
		}

		int32 Cursor = BestRoot;
		int32 Guard = 0;
		while (Cursor != INDEX_NONE && Guard < Bones.Num())
		{
			OutChain.Add(Cursor);
			Cursor = ConeStructuralChild(Bones, Cursor, ESolidBodyBoneRole::Spine);
			++Guard;
		}
	}

	void CollectConeNeck(const TArray<FSolidDeformBone>& Bones, TArray<int32>& OutChain)
	{
		OutChain.Reset();
		int32 BestRoot = INDEX_NONE;
		int32 BestLength = 0;
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (!ConeIsStructural(Bones[Index]) || Bones[Index].Role != ESolidBodyBoneRole::Neck)
			{
				continue;
			}
			const int32 Parent = ConeStructuralParent(Bones, Index);
			if (Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Neck)
			{
				continue;
			}
			int32 Length = 0;
			int32 Cursor = Index;
			int32 Guard = 0;
			while (Cursor != INDEX_NONE && Guard < Bones.Num())
			{
				++Length;
				Cursor = ConeStructuralChild(Bones, Cursor, ESolidBodyBoneRole::Neck);
				++Guard;
			}
			if (Length > BestLength)
			{
				BestLength = Length;
				BestRoot = Index;
			}
		}

		int32 Cursor = BestRoot;
		int32 Guard = 0;
		while (Cursor != INDEX_NONE && Guard < Bones.Num())
		{
			OutChain.Add(Cursor);
			Cursor = ConeStructuralChild(Bones, Cursor, ESolidBodyBoneRole::Neck);
			++Guard;
		}
	}

	float ScaledJoint(const float Base, const float Channel)
	{
		return FMath::Clamp(Base * FMath::Max(Channel, 0.f), 2.5f, 28.f);
	}
}

float SolidBodyMesh::HeadHeightCm(const FSolidBodyParams& Params)
{
	return FMath::Clamp(SolidBody::HeadLengthCm * Params.HeadSize, 17.f, 30.f);
}

float SolidBodyMesh::HeadChinRadius(const FSolidBodyParams& Params)
{
	return HeadHeightCm(Params) * HeadRadiusOverHeight * HeadChinSlice;
}

SolidBodyMesh::FSolidConeProfile SolidBodyMesh::ConeProfile(const FSolidBodyParams& Params)
{
	FSolidConeProfile Profile;
	Profile.Waist = ScaledJoint(JointWaist, Params.TorsoGirth);
	Profile.Chest = ScaledJoint(JointChest, Params.TorsoGirth);
	Profile.Neck = ScaledJoint(JointNeck, Params.TorsoGirth);
	Profile.Jaw = HeadChinRadius(Params);
	Profile.Thigh = ScaledJoint(JointThigh, Params.Bulk);
	Profile.Knee = ScaledJoint(JointKnee, Params.Bulk);
	Profile.Ankle = ScaledJoint(JointAnkle, Params.Bulk);
	Profile.Shoulder = ScaledJoint(JointShoulder, Params.Bulk);
	Profile.Elbow = ScaledJoint(JointElbow, Params.Bulk);
	Profile.Wrist = ScaledJoint(JointWrist, Params.Bulk);
	return Profile;
}

float SolidBodyMesh::SplitHalfWidth(const float ChildLateral, const float ChildRadius)
{
	return FMath::Max(0.f, ChildLateral) + FMath::Max(0.f, ChildRadius);
}

float SolidBodyMesh::SpineJointRadius(const float Waist, const float Chest, const int32 Index, const int32 LastIndex)
{
	if (LastIndex <= 0)
	{
		return Chest;
	}
	const float T = FMath::Clamp(
		static_cast<float>(Index) / static_cast<float>(LastIndex),
		0.f,
		1.f);
	return FMath::Lerp(Waist, Chest, T);
}

void SolidBodyMesh::SegmentRadii(
	const TArray<float>& Radii,
	const int32 Parent,
	const int32 Child,
	float& OutStart,
	float& OutEnd)
{
	OutEnd = Radii.IsValidIndex(Child) ? Radii[Child] : 0.f;
	OutStart = Radii.IsValidIndex(Parent) ? Radii[Parent] : OutEnd;
}

void SolidBodyMesh::JointRadii(
	const TArray<FSolidDeformBone>& Bones,
	const TArray<FVector>& Positions,
	const FSolidBodyParams& Params,
	TArray<float>& OutRadii)
{
	const int32 Count = FMath::Min(Bones.Num(), Positions.Num());
	const FSolidConeProfile Profile = ConeProfile(Params);
	OutRadii.Init(Profile.Chest, Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		if (!ConeIsStructural(Bones[Index]))
		{
			continue;
		}
		const int32 Parent = ConeStructuralParent(Bones, Index);
		const ESolidBodyBoneRole ParentRole = Bones.IsValidIndex(Parent)
			? Bones[Parent].Role
			: ESolidBodyBoneRole::Root;
		switch (Bones[Index].Role)
		{
		case ESolidBodyBoneRole::Leg:
			OutRadii[Index] = (ParentRole == ESolidBodyBoneRole::Pelvis || ParentRole == ESolidBodyBoneRole::Root)
				? Profile.Thigh
				: Profile.Knee;
			break;
		case ESolidBodyBoneRole::Foot:
			OutRadii[Index] = (ParentRole == ESolidBodyBoneRole::Leg) ? Profile.Knee : Profile.Ankle;
			break;
		case ESolidBodyBoneRole::Arm:
			OutRadii[Index] = (ParentRole == ESolidBodyBoneRole::Arm) ? Profile.Elbow : Profile.Shoulder;
			break;
		case ESolidBodyBoneRole::Hand:
			OutRadii[Index] = Profile.Wrist;
			break;
		case ESolidBodyBoneRole::Head:
			OutRadii[Index] = Profile.Jaw;
			break;
		case ESolidBodyBoneRole::Clavicle:
			OutRadii[Index] = Profile.Shoulder;
			break;
		default:
			break;
		}
	}

	TArray<int32> Spines;
	CollectConeSpine(Bones, Spines);
	const int32 SpineLast = Spines.Num() - 1;
	for (int32 Step = 0; Step < Spines.Num(); ++Step)
	{
		if (Spines[Step] < Count)
		{
			OutRadii[Spines[Step]] = SpineJointRadius(Profile.Waist, Profile.Chest, Step, SpineLast);
		}
	}

	for (int32 Pelvis = 0; Pelvis < Count; ++Pelvis)
	{
		if (!ConeIsStructural(Bones[Pelvis]) || Bones[Pelvis].Role != ESolidBodyBoneRole::Pelvis)
		{
			continue;
		}
		float Reach = 0.f;
		int32 Legs = 0;
		for (int32 Leg = 0; Leg < Count; ++Leg)
		{
			if (!ConeIsStructural(Bones[Leg]) || Bones[Leg].Role != ESolidBodyBoneRole::Leg)
			{
				continue;
			}
			if (ConeStructuralParent(Bones, Leg) != Pelvis)
			{
				continue;
			}
			++Legs;
			Reach = FMath::Max(Reach, SplitHalfWidth(HorizontalReach(Positions[Pelvis], Positions[Leg]), OutRadii[Leg]));
		}
		OutRadii[Pelvis] = (Legs > 0) ? Reach : Profile.Waist;
	}

	// The chest is the cone that meets both arms. The neck stays a neck and ends on the chin.
	float ShoulderReach = 0.f;
	int32 Arms = 0;
	for (int32 Clavicle = 0; Clavicle < Count; ++Clavicle)
	{
		if (!ConeIsStructural(Bones[Clavicle]) || Bones[Clavicle].Role != ESolidBodyBoneRole::Clavicle)
		{
			continue;
		}
		const int32 Arm = ConeStructuralChild(Bones, Clavicle, ESolidBodyBoneRole::Arm);
		if (!Positions.IsValidIndex(Arm) || Arm >= Count)
		{
			continue;
		}
		const int32 Origin = ConeStructuralParent(Bones, Clavicle);
		if (!Positions.IsValidIndex(Origin))
		{
			continue;
		}
		++Arms;
		ShoulderReach = FMath::Max(
			ShoulderReach,
			SplitHalfWidth(HorizontalReach(Positions[Origin], Positions[Arm]), OutRadii[Arm]));
	}
	if (Arms > 0 && Spines.Num() > 0 && Spines.Last() < Count)
	{
		OutRadii[Spines.Last()] = ShoulderReach;
	}

	TArray<int32> Necks;
	CollectConeNeck(Bones, Necks);
	for (int32 Step = 0; Step < Necks.Num(); ++Step)
	{
		if (Necks[Step] >= Count)
		{
			continue;
		}
		float Radius = Profile.Neck;
		if (Necks.Num() > 1)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Necks.Num() - 1);
			Radius = FMath::Lerp(Profile.Neck, Profile.Jaw, T);
		}
		OutRadii[Necks[Step]] = Radius;
	}
}

#if WITH_EDITORONLY_DATA
namespace
{
	void FillBones(const FReferenceSkeleton& Ref, FSolidDeformMesh& Out)
	{
		const TArray<FMeshBoneInfo>& Infos = Ref.GetRawRefBoneInfo();
		const TArray<FTransform>& Pose = Ref.GetRawRefBonePose();
		const int32 Count = FMath::Min(Infos.Num(), Pose.Num());
		Out.Bones.Reserve(Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FSolidDeformBone Bone;
			const FString BoneName = Infos[Index].Name.ToString();
			Bone.ParentIndex = Infos[Index].ParentIndex;
			Bone.Name = Infos[Index].Name;
			Bone.Role = SolidBodyMesh::RoleForBoneName(BoneName);
			Bone.bTwist = SolidBodyMesh::IsTwistBoneName(BoneName);
			Bone.bDigit = SolidBodyMesh::IsDigitBoneName(BoneName);
			Bone.LocalTranslation = Pose[Index].GetTranslation();
			Bone.LocalRotation = Pose[Index].GetRotation();
			Out.Bones.Add(Bone);
		}
	}

	void PaintSlot(
		USkeletalMeshComponent* MeshComp,
		const int32 Slot,
		const FLinearColor& Color,
		const TCHAR* DebugName)
	{
		if (!MeshComp)
		{
			return;
		}

		// Overrides live on the component. The shared mesh keeps its slot names and is not recolored
		// while a companion is already drawing it.
		UMaterialInterface* Current = MeshComp->GetMaterial(Slot);
		UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Current);
		if (Existing && Existing->GetOuter() == MeshComp)
		{
			Existing->SetVectorParameterValue(TEXT("Base Color"), Color);
			Existing->SetVectorParameterValue(TEXT("BaseColor"), Color);
			Existing->SetScalarParameterValue(TEXT("Roughness"), 1.f);
			Existing->RecacheUniformExpressions(true);
			return;
		}

		if (UMaterialInterface* Created = SolidMaterials::CreateSolidColor(MeshComp, Color, DebugName, 1.f))
		{
			MeshComp->SetMaterial(Slot, Created);
		}
	}

	void ApplyComponentColors(
		USkeletalMeshComponent* MeshComp,
		const FLinearColor& Torso,
		const FLinearColor& Legs,
		const FLinearColor& Skin)
	{
		PaintSlot(MeshComp, 0, Torso, TEXT("BodyTorso"));
		PaintSlot(MeshComp, 1, Legs, TEXT("BodyLegs"));
		PaintSlot(MeshComp, 2, Skin, TEXT("BodySkin"));
	}

	void ApplyWearColors(
		USkeletalMesh* Mesh,
		const FLinearColor& Torso,
		const FLinearColor& Legs,
		const FLinearColor& Skin)
	{
		UMaterialInterface* TorsoMat = SolidMaterials::CreateSolidColor(
			Mesh, Torso, TEXT("BodyTorso"), 1.f);
		UMaterialInterface* LegMat = SolidMaterials::CreateSolidColor(
			Mesh, Legs, TEXT("BodyLegs"), 1.f);
		UMaterialInterface* SkinMat = SolidMaterials::CreateSolidColor(
			Mesh, Skin, TEXT("BodySkin"), 1.f);
		if (!TorsoMat && !LegMat && !SkinMat)
		{
			return;
		}

		TArray<FSkeletalMaterial> Slots;
		FSkeletalMaterial TorsoSlot;
		TorsoSlot.MaterialInterface = TorsoMat ? TorsoMat : LegMat;
		TorsoSlot.MaterialSlotName = TEXT("Torso");
		TorsoSlot.ImportedMaterialSlotName = TEXT("Torso");
		FSkeletalMaterial LegSlot;
		LegSlot.MaterialInterface = LegMat ? LegMat : TorsoMat;
		LegSlot.MaterialSlotName = TEXT("Legs");
		LegSlot.ImportedMaterialSlotName = TEXT("Legs");
		FSkeletalMaterial SkinSlot;
		SkinSlot.MaterialInterface = SkinMat ? SkinMat : (TorsoMat ? TorsoMat : LegMat);
		SkinSlot.MaterialSlotName = TEXT("Skin");
		SkinSlot.ImportedMaterialSlotName = TEXT("Skin");
		Slots.Add(TorsoSlot);
		Slots.Add(LegSlot);
		Slots.Add(SkinSlot);
		Mesh->SetMaterials(Slots);
	}

	bool WriteDescription(
		FMeshDescription& Description,
		const FReferenceSkeleton& Ref,
		const FSolidDeformMesh& Deformed,
		const TArray<SolidBodyMesh::FSolidBodyVert>& Verts,
		const TArray<SolidBodyMesh::FSolidBodyTri>& Tris)
	{
		FSkeletalMeshAttributes Attributes(Description);
		Attributes.Register(false);
		if (!Attributes.GetVertexSkinWeights(NAME_None).IsValid())
		{
			Attributes.RegisterSkinWeightAttribute(NAME_None);
		}

		FSkinWeightsVertexAttributesRef Skin = Attributes.GetVertexSkinWeights(NAME_None);
		auto BoneNames = Attributes.GetBoneNames();
		auto BoneParents = Attributes.GetBoneParentIndices();
		auto BonePoses = Attributes.GetBonePoses();
		if (!Skin.IsValid() || !BoneNames.IsValid() || !BoneParents.IsValid() || !BonePoses.IsValid())
		{
			UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly mesh could not register bone attributes."));
			return false;
		}

		const TArray<FTransform>& SourcePose = Ref.GetRawRefBonePose();
		const int32 BoneCount = FMath::Min3(Deformed.Bones.Num(), Ref.GetRawBoneNum(), SourcePose.Num());
		for (int32 Index = 0; Index < BoneCount; ++Index)
		{
			const FBoneID BoneID(Index);
			Attributes.CreateBone(BoneID);
			BoneNames.Set(BoneID, Ref.GetBoneName(Index));

			using FParentValue = typename TDecay<decltype(BoneParents.Get(BoneID))>::Type;
			BoneParents.Set(BoneID, FParentValue(Deformed.Bones[Index].ParentIndex));

			FTransform Pose = SourcePose[Index];
			Pose.SetTranslation(Deformed.Bones[Index].LocalTranslation);
			Pose.SetRotation(Deformed.Bones[Index].LocalRotation);
			using FPoseValue = typename TDecay<decltype(BonePoses.Get(BoneID))>::Type;
			BonePoses.Set(BoneID, FPoseValue(Pose));
		}

		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		for (int32 Index = 0; Index < Verts.Num(); ++Index)
		{
			const FVertexID VertexID = Description.CreateVertex();
			if (VertexID.GetValue() != Index || Verts[Index].Bone == INDEX_NONE)
			{
				UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly vertex order does not match the skeleton."));
				return false;
			}
			Positions.Set(VertexID, FVector3f(Verts[Index].Position));

			const FBoneIndexType InfluenceBone = static_cast<FBoneIndexType>(Verts[Index].Bone);
			const float InfluenceWeight = 1.f;
			Skin.Set(
				VertexID,
				UE::AnimationCore::FBoneWeights::Create(
					&InfluenceBone,
					&InfluenceWeight,
					1,
					UE::AnimationCore::FBoneWeightsSettings()));
		}

		const FPolygonGroupID TorsoGroup = Description.CreatePolygonGroup();
		const FPolygonGroupID LegGroup = Description.CreatePolygonGroup();
		const FPolygonGroupID SkinGroup = Description.CreatePolygonGroup();
		auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
		if (SlotNames.IsValid())
		{
			SlotNames.Set(TorsoGroup, FName(TEXT("Torso")));
			SlotNames.Set(LegGroup, FName(TEXT("Legs")));
			SlotNames.Set(SkinGroup, FName(TEXT("Skin")));
		}

		auto Normals = Attributes.GetVertexInstanceNormals();
		auto Tangents = Attributes.GetVertexInstanceTangents();
		auto Signs = Attributes.GetVertexInstanceBinormalSigns();
		auto UVs = Attributes.GetVertexInstanceUVs();
		if (UVs.IsValid() && UVs.GetNumChannels() < 1)
		{
			UVs.SetNumChannels(1);
		}

		int32 Written = 0;
		for (const SolidBodyMesh::FSolidBodyTri& Tri : Tris)
		{
			if (!Verts.IsValidIndex(Tri.V[0]) || !Verts.IsValidIndex(Tri.V[1]) || !Verts.IsValidIndex(Tri.V[2]))
			{
				continue;
			}
			const FVector A = Verts[Tri.V[0]].Position;
			const FVector B = Verts[Tri.V[1]].Position;
			const FVector C = Verts[Tri.V[2]].Position;
			const FVector Cross = FVector::CrossProduct(B - A, C - A);
			if (Cross.SizeSquared() < 0.001)
			{
				continue;
			}
			const FVector Normal = Cross.GetSafeNormal();
			FVector Tangent = FVector::CrossProduct(Normal, FVector::UpVector);
			if (Tangent.SizeSquared() < 0.01)
			{
				Tangent = FVector::CrossProduct(Normal, FVector::ForwardVector);
			}
			Tangent = Tangent.GetSafeNormal();

			FVertexInstanceID Corners[3];
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVertexID VertexID(Tri.V[Corner]);
				const FVertexInstanceID InstanceID = Description.CreateVertexInstance(VertexID);
				if (Normals.IsValid())
				{
					Normals.Set(InstanceID, FVector3f(Normal));
				}
				if (Tangents.IsValid())
				{
					Tangents.Set(InstanceID, FVector3f(Tangent));
				}
				if (Signs.IsValid())
				{
					Signs.Set(InstanceID, 1.f);
				}
				if (UVs.IsValid())
				{
					UVs.Set(InstanceID, 0, FVector2f(static_cast<float>(Corner) * 0.5f, (Written & 1) ? 1.f : 0.f));
				}
				Corners[Corner] = InstanceID;
			}
			FPolygonGroupID Group = TorsoGroup;
			if (Verts[Tri.V[0]].Slot == SolidBodyMesh::TrouserMaterialSlot)
			{
				Group = LegGroup;
			}
			else if (Verts[Tri.V[0]].Slot == SolidBodyMesh::SkinMaterialSlot)
			{
				Group = SkinGroup;
			}
			Description.CreateTriangle(Group, MakeArrayView(Corners));
			++Written;
		}

		if (Written == 0)
		{
			UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly mesh produced no triangles."));
			return false;
		}
		return true;
	}

	USkeleton* DuplicateBodySkeleton(const USkeleton* SourceSkeleton, UObject* Outer)
	{
		if (!SourceSkeleton || !Outer)
		{
			return nullptr;
		}

		USkeleton* BodySkeleton = DuplicateObject<USkeleton>(SourceSkeleton, Outer);
		if (!BodySkeleton)
		{
			return nullptr;
		}

		BodySkeleton->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
		BodySkeleton->SetFlags(RF_Transient);

		const FReferenceSkeleton& Ref = BodySkeleton->GetReferenceSkeleton();
		for (int32 Index = 0; Index < Ref.GetRawBoneNum(); ++Index)
		{
			if (!RoleScalesLength(SolidBodyMesh::RoleForBoneName(Ref.GetBoneName(Index).ToString())))
			{
				continue;
			}
			BodySkeleton->SetBoneTranslationRetargetingMode(
				Index,
				EBoneTranslationRetargetingMode::AnimationScaled,
				false);
		}
		return BodySkeleton;
	}

	USkeleton* SharedBodySkeleton(USkeleton* SourceSkeleton)
	{
		static TWeakObjectPtr<USkeleton> Cached;
		static TWeakObjectPtr<USkeleton> CachedSource;
		if (USkeleton* Existing = Cached.Get())
		{
			if (CachedSource.Get() == SourceSkeleton)
			{
				return Existing;
			}
		}

		USkeleton* Copy = DuplicateBodySkeleton(SourceSkeleton, GetTransientPackage());
		if (!Copy)
		{
			return nullptr;
		}
		Cached = Copy;
		CachedSource = SourceSkeleton;
		return Copy;
	}

	bool HasRenderableBody(const USkeletalMesh* Mesh)
	{
		if (!Mesh || Mesh->GetRefSkeleton().GetRawBoneNum() <= 0)
		{
			return false;
		}
		if (Mesh->GetRefBasesInvMatrix().Num() <= 0)
		{
			return false;
		}
		const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
		return RenderData
			&& RenderData->LODRenderData.Num() > 0
			&& RenderData->LODRenderData[0].GetNumVertices() > 0;
	}

	int32 RenderVertexCount(const USkeletalMesh* Mesh)
	{
		const FSkeletalMeshRenderData* RenderData = Mesh ? Mesh->GetResourceForRendering() : nullptr;
		if (!RenderData || RenderData->LODRenderData.Num() == 0)
		{
			return 0;
		}
		return static_cast<int32>(RenderData->LODRenderData[0].GetNumVertices());
	}

	void EnsureImportedLod(USkeletalMesh* Mesh)
	{
#if WITH_EDITOR
		FSkeletalMeshModel* Imported = Mesh ? Mesh->GetImportedModel() : nullptr;
		if (Imported && Imported->LODModels.Num() < 1)
		{
			Imported->LODModels.Add(new FSkeletalMeshLODModel());
		}
#else
		(void)Mesh;
#endif
	}

	void ApplyDeformedRefPose(
		USkeletalMesh* Mesh,
		const USkeleton* VirtualBoneSource,
		const FReferenceSkeleton& SourceRef,
		const FSolidDeformMesh& Deformed)
	{
		if (!Mesh || !VirtualBoneSource)
		{
			return;
		}

		Mesh->SetRefSkeleton(SourceRef);
		const TArray<FTransform>& SourcePose = SourceRef.GetRawRefBonePose();
		FReferenceSkeleton& MeshRef = Mesh->GetRefSkeleton();
		{
			FReferenceSkeletonModifier Modifier(MeshRef, VirtualBoneSource);
			const int32 Count = FMath::Min3(Deformed.Bones.Num(), MeshRef.GetRawBoneNum(), SourcePose.Num());
			for (int32 Index = 0; Index < Count; ++Index)
			{
				FTransform Pose = SourcePose[Index];
				Pose.SetTranslation(Deformed.Bones[Index].LocalTranslation);
				Pose.SetRotation(Deformed.Bones[Index].LocalRotation);
				Modifier.UpdateRefPoseTransform(Index, Pose);
			}
		}
		Mesh->CalculateInvRefMatrices();
	}

	TMap<uint32, TWeakObjectPtr<USkeletalMesh>>& BodyCache()
	{
		static TMap<uint32, TWeakObjectPtr<USkeletalMesh>> Cache;
		return Cache;
	}

	/** FNV-1a over the clamped channel bits, so two equal bodies share one mesh. */
	uint32 ParamsCacheKey(const FSolidBodyParams& Params)
	{
		const FSolidBodyParams Clamped = Params.Clamped();
		uint32 Hash = 2166136261u;
		const uint8* Bytes = reinterpret_cast<const uint8*>(&Clamped);
		for (int32 Index = 0; Index < static_cast<int32>(sizeof(FSolidBodyParams)); ++Index)
		{
			Hash ^= Bytes[Index];
			Hash *= 16777619u;
		}
		return Hash;
	}

	void WearBody(
		USkeletalMeshComponent* MeshComp,
		USkeletalMesh* Body,
		const uint32 Seed,
		const FSolidBodyParams& Params,
		const TCHAR* DebugLabel,
		const bool bReused)
	{
		MeshComp->SetSkeletalMeshAsset(Body);
		MeshComp->SetAnimInstanceClass(nullptr);
		MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		MeshComp->InitAnim(true);
		UE_LOG(LogSolid, Warning,
			TEXT("%s body seed %u height %.2f bulk %.2f verts %d%s"),
			DebugLabel ? DebugLabel : TEXT("Body"),
			Seed,
			Params.Height,
			Params.Bulk,
			RenderVertexCount(Body),
			bReused ? TEXT(" (reused)") : TEXT(""));
	}
}
#endif

USkeletalMesh* SolidBodyMesh::Build(
	const USkeletalMesh* Source,
	const FSolidBodyParams& Params,
	UObject* Outer)
{
#if WITH_EDITORONLY_DATA
	if (!Source || !Outer)
	{
		return nullptr;
	}

	const FReferenceSkeleton& Ref = Source->GetRefSkeleton();
	if (Ref.GetRawBoneNum() == 0)
	{
		return nullptr;
	}

	USkeleton* SourceSkeleton = const_cast<USkeletalMesh*>(Source)->GetSkeleton();
	if (!SourceSkeleton)
	{
		return nullptr;
	}

	FSolidDeformMesh Bind;
	FillBones(Ref, Bind);
	// Lengths stay the Viking reference pose. The sliders scale from there.
	const FSolidBodyParams Clamped = Params.Clamped();
	const FSolidDeformMesh Deformed = SolidBodyDeform::Apply(Bind, Clamped);

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Deformed.Bones, Positions);

	// The part list draws the triangles. The rest of this function hands them to Unreal.
	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
	FSolidBodyMesh Body = FSolidBodyMesh::FromBones(Deformed.Bones);
	Body.Draw(Verts, Tris, Deformed.Bones, Positions);
	if (Tris.Num() == 0 || Verts.Num() == 0)
	{
		UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly mesh has no segments, so it is discarded."));
		return nullptr;
	}

	FMeshDescription Description;
	if (!WriteDescription(Description, Ref, Deformed, Verts, Tris))
	{
		return nullptr;
	}

	USkeleton* BodySkeleton = SharedBodySkeleton(SourceSkeleton);
	if (!BodySkeleton)
	{
		return nullptr;
	}

	USkeletalMesh* Mesh = NewObject<USkeletalMesh>(
		Outer,
		MakeUniqueObjectName(Outer, USkeletalMesh::StaticClass(), TEXT("SolidBody")),
		RF_Transient);
	if (!Mesh)
	{
		return nullptr;
	}

	Mesh->SetSkeleton(BodySkeleton);
	Mesh->NeverStream = true;
	Mesh->SetRefSkeleton(Ref);
	ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, Deformed);
	ApplyWearColors(
		Mesh,
		FLinearColor(0.29f, 0.38f, 0.55f),
		FLinearColor(0.12f, 0.11f, 0.10f),
		FLinearColor(0.96f, 0.76f, 0.72f));

	if (Mesh->GetNumSourceModels() < 1)
	{
		Mesh->AddSourceModel(false);
	}
	if (!Mesh->GetLODInfo(0))
	{
		Mesh->AddLODInfo();
	}
	EnsureImportedLod(Mesh);

	FMeshDescription* Dest = Mesh->CreateMeshDescription(0, MoveTemp(Description));
	if (!Dest)
	{
		return nullptr;
	}

	USkeletalMesh::FCommitMeshDescriptionParams CommitParams;
	CommitParams.bMarkPackageDirty = false;
	CommitParams.bForceUpdate = true;
	if (!Mesh->CommitMeshDescription(0, CommitParams))
	{
		UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly mesh could not commit its description."));
		return nullptr;
	}

	Mesh->Build();
	ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, Deformed);
	Mesh->SetSkeleton(BodySkeleton);
	if (!HasRenderableBody(Mesh))
	{
		Mesh->Build();
		ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, Deformed);
		Mesh->CalculateInvRefMatrices();
	}
	if (!HasRenderableBody(Mesh))
	{
		const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
		UE_LOG(LogSolid, Warning,
			TEXT("SolidBody low-poly build has no wearable LOD (bones %d, lods %d), so it is discarded."),
			Mesh->GetRefSkeleton().GetRawBoneNum(),
			RenderData ? RenderData->LODRenderData.Num() : 0);
		return nullptr;
	}
	ApplyWearColors(
		Mesh,
		FLinearColor(0.29f, 0.38f, 0.55f),
		FLinearColor(0.12f, 0.11f, 0.10f),
		FLinearColor(0.96f, 0.76f, 0.72f));
	return Mesh;
#else
	(void)Source;
	(void)Params;
	(void)Outer;
	return nullptr;
#endif
}

uint32 SolidBodyMesh::FaceCodepointForName(const FString& Name)
{
	static const uint32 Faces[] = {
		0x1F610u,
		0x1F611u,
		0x1F642u,
		0x1F641u,
		0x1F612u,
		0x1F644u,
	};
	return Faces[SeedForName(Name) % UE_ARRAY_COUNT(Faces)];
}

FString SolidBodyMesh::FaceStringForCodepoint(const uint32 Codepoint)
{
	if (Codepoint <= 0xFFFFu)
	{
		return FString::Chr(static_cast<TCHAR>(Codepoint));
	}

	const uint32 Value = Codepoint - 0x10000u;
	const TCHAR Pair[3] = {
		static_cast<TCHAR>(0xD800u + (Value >> 10)),
		static_cast<TCHAR>(0xDC00u + (Value & 0x3FFu)),
		0,
	};
	return FString(Pair);
}

void SolidBodyMesh::ApplySeed(
	USkeletalMeshComponent* MeshComp,
	const uint32 Seed,
	const FSolidBodyParams& Params,
	UObject* Outer,
	const TCHAR* DebugLabel,
	const FLinearColor& TorsoColor,
	const FLinearColor& LegColor,
	const FLinearColor& SkinColor)
{
	if (!MeshComp || !Outer)
	{
		return;
	}

#if WITH_EDITORONLY_DATA
	const uint32 Key = ParamsCacheKey(Params);
	if (TWeakObjectPtr<USkeletalMesh>* Found = BodyCache().Find(Key))
	{
		if (USkeletalMesh* Cached = Found->Get())
		{
			if (HasRenderableBody(Cached))
			{
				WearBody(MeshComp, Cached, Seed, Params, DebugLabel, true);
				ApplyComponentColors(MeshComp, TorsoColor, LegColor, SkinColor);
				return;
			}
		}
		BodyCache().Remove(Key);
	}
#endif

	USkeletalMesh* Source = SolidContentLookup::FindVikingMesh();
	if (!Source)
	{
		return;
	}

	USkeletalMesh* Built = Build(Source, Params, GetTransientPackage());
	if (!Built)
	{
		UE_LOG(LogSolid, Warning,
			TEXT("%s body: could not build a low-poly body, so the source mesh stays."),
			DebugLabel ? DebugLabel : TEXT("Body"));
		return;
	}
#if WITH_EDITORONLY_DATA
	if (!HasRenderableBody(Built))
	{
		UE_LOG(LogSolid, Warning,
			TEXT("%s body: generated mesh has no wearable LOD, so the source mesh stays."),
			DebugLabel ? DebugLabel : TEXT("Body"));
		return;
	}
	BodyCache().Add(Key, Built);
	WearBody(MeshComp, Built, Seed, Params, DebugLabel, false);
	ApplyComponentColors(MeshComp, TorsoColor, LegColor, SkinColor);
#else
	(void)Seed;
	(void)Params;
	(void)Built;
	(void)TorsoColor;
	(void)LegColor;
	(void)SkinColor;
#endif
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
