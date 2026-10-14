#include "SolidBodyMesh.h"

#include "SolidContentLookup.h"
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
	FString SpinePartName(int32 Step, int32 Count);
}

SolidBodyMesh::FSolidBodyMesh SolidBodyMesh::FSolidBodyMesh::Make(const TArray<FName>& RisingSpineBones)
{
	using SolidBodyMesh::ESolidPartColour;
	using SolidBodyMesh::ESolidPartMeasure;
	using SolidBodyMesh::FSolidPartSection;

	FSolidBodyMesh Body;
	auto AddPart = [&Body](FSolidBodyPart Part, const ESolidPartMeasure Measure)
	{
		Part.Measure = Measure;
		Body.Parts.Add(MoveTemp(Part));
	};

	const FSolidPartSection Round20 = FSolidPartSection::Round(LeftThighTopDiameterCm);
	AddPart(FSolidBodyPart(TEXT("Thigh (L)"), { LeftThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Thigh (R)"), { RightThighBone }, ESolidPartColour::Trousers, Round20, FSolidPartSection::Round(LeftThighBottomDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Calf (L)"), { LeftCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Calf (R)"), { RightCalfBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(CalfTopDiameterCm), FSolidPartSection::Round(CalfBottomDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Foot (L)"), { LeftBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Foot (R)"), { RightBallBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(BallDiameterCm), FSolidPartSection::Round(BallDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Toe (L)"), { LeftExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)), ESolidPartMeasure::Leg);
	AddPart(FSolidBodyPart(TEXT("Toe (R)"), { RightExtraFingerBone }, ESolidPartColour::Trousers, FSolidPartSection::Round(ExtraFingerAnkleDiameterCm), FSolidPartSection::Round(ExtraFingerToeDiameterCm)), ESolidPartMeasure::Leg);

	// The hip mesh is the length of spine_01. The thighs are inside it.
	AddPart(FSolidBodyPart(
		TEXT("Hip"),
		{ HipSpineBone, HipLeftBone, HipRightBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(HipSideDiameterCm, HipDepthDiameterCm),
		FSolidPartSection::Oval(HipTopSideDiameterCm, HipDepthDiameterCm)), ESolidPartMeasure::Hip);

	const int32 SpineCount = RisingSpineBones.Num();
	for (int32 Step = 0; Step < SpineCount; ++Step)
	{
		float BottomCm = 0.f;
		float TopCm = 0.f;
		TorsoSideDiameters(Step, SpineCount, BottomCm, TopCm);
		FSolidBodyPart Spine = FSolidBodyPart(
			TEXT("Spine"),
			{},
			ESolidPartColour::Shirt,
			FSolidPartSection::Oval(BottomCm, TorsoDepthDiameterCm),
			FSolidPartSection::Oval(TopCm, TorsoDepthDiameterCm));
		Spine.Name = SpinePartName(Step, SpineCount);
		Spine.Bones.Add(RisingSpineBones[Step]);
		AddPart(Spine, ESolidPartMeasure::Torso);
	}

	float ShoulderSideCm = ChestSideDiameterCm;
	for (const FSolidBodyPart& Spine : Body.Parts)
	{
		if (Spine.Bones.Num() == 1 && Spine.Bones[0] == FName(SpineChestBone))
		{
			ShoulderSideCm = Spine.End.X;
		}
	}

	// Shoulders follow neck_01. Mid is fitted around the clavicles and the upper arms when the mesh is built.
	FSolidBodyPart Shoulders = FSolidBodyPart(
		TEXT("Shoulders"),
		{ ShoulderNeckBone, ShoulderLeftClavicleBone, ShoulderRightClavicleBone, ShoulderLeftUpperArmBone, ShoulderRightUpperArmBone },
		ESolidPartColour::Shirt,
		FSolidPartSection::Oval(ShoulderSideCm, TorsoDepthDiameterCm),
		FSolidPartSection::Round(ShoulderTopRadiusCm * 2.f));
	Shoulders.bHasMid = true;
	AddPart(Shoulders, ESolidPartMeasure::Shoulder);

	// upperarm joint to elbow. The skeleton names that bone lowerarm.
	AddPart(FSolidBodyPart(
		TEXT("Upper arm (L)"),
		{ TEXT("lowerarm_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)), ESolidPartMeasure::Arm);
	AddPart(FSolidBodyPart(
		TEXT("Upper arm (R)"),
		{ TEXT("lowerarm_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(UpperArmElbowDiameterCm),
		FSolidPartSection::Round(ForearmWristDiameterCm)), ESolidPartMeasure::Arm);

	// elbow to wrist. The skeleton names that bone hand. Narrow at the elbow, wide at the wrist.
	AddPart(FSolidBodyPart(
		TEXT("Lower arm (L)"),
		{ TEXT("hand_l") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)), ESolidPartMeasure::Arm);
	AddPart(FSolidBodyPart(
		TEXT("Lower arm (R)"),
		{ TEXT("hand_r") },
		ESolidPartColour::Shirt,
		FSolidPartSection::Round(LowerArmStartDiameterCm),
		FSolidPartSection::Round(LowerArmEndDiameterCm)), ESolidPartMeasure::Arm);

	return Body;
}

namespace
{
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

uint32 SolidBodyMesh::SeedForName(const FString& Name)
{
	const FString Lower = Name.ToLower();
	return FCrc::StrCrc32(*Lower);
}

bool SolidBodyMesh::IsTwistBoneName(const FString& BoneName)
{
	return BoneName.ToLower().Contains(TEXT("twist"));
}

bool SolidBodyMesh::IsRisingSpineBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	return Lower.StartsWith(TEXT("spine_"))
		&& !Lower.StartsWith(TEXT("spine_01"))
		&& !IsTwistBoneName(Lower);
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
			Bone.ParentIndex = Infos[Index].ParentIndex;
			Bone.Name = Infos[Index].Name;
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
		const TArray<FMeshBoneInfo>& Infos = Ref.GetRawRefBoneInfo();
		TArray<FSolidDeformBone> Named;
		Named.Reserve(Infos.Num());
		for (int32 Index = 0; Index < Infos.Num(); ++Index)
		{
			FSolidDeformBone Bone;
			Bone.ParentIndex = Infos[Index].ParentIndex;
			Bone.Name = Infos[Index].Name;
			Named.Add(Bone);
		}
		const SolidBodyMesh::FSolidBodyMesh Catalogue = SolidBodyMesh::FSolidBodyMesh::FromBones(Named);
		for (int32 Index = 0; Index < Infos.Num(); ++Index)
		{
			const FName BoneName = Infos[Index].Name;
			bool bWrapped = false;
			for (const SolidBodyMesh::FSolidBodyPart& Part : Catalogue.Parts)
			{
				if (Part.Bones.Contains(BoneName))
				{
					bWrapped = true;
					break;
				}
			}
			if (!bWrapped)
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
	// Lengths start as the Viking reference pose. Each part then sizes its own bones.
	const FSolidBodyParams Clamped = Params.Clamped();
	FSolidDeformMesh Deformed = Bind;
	FSolidBodyMesh Body = FSolidBodyMesh::FromBones(Deformed.Bones);
	Body.Apply(Deformed.Bones, Clamped);

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Deformed.Bones, Positions);

	// The part list draws the triangles. The rest of this function hands them to Unreal.
	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
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

const SolidBodyMesh::FSolidBodyPart* SolidBodyMesh::FSolidBodyMesh::Find(const FString& Name) const
{
	for (const FSolidBodyPart& Part : Parts)
	{
		if (Part.Name.Equals(Name, ESearchCase::IgnoreCase))
		{
			return &Part;
		}
	}
	return nullptr;
}

void SolidBodyMesh::FSolidBodyMesh::Apply(
	TArray<FSolidDeformBone>& Bones,
	const FSolidBodyParams& Params)
{
	for (FSolidBodyPart& Part : Parts)
	{
		Part.Apply(Bones, Params);
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
	for (FSolidBodyPart& Part : Parts)
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
