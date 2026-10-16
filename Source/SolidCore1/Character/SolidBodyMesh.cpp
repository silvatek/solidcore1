#include "SolidBodyMesh.h"

#include "Party/SolidCompany.h"
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
#include "Components/CapsuleComponent.h"
#include "RenderingThread.h"
#include "SkeletalMeshAttributes.h"
#include "UObject/StrongObjectPtr.h"
#endif

SolidBodyMesh::FSolidBodyMesh::FSolidBodyMesh(const FSolidBodyMesh& Other)
	: OriginalBones(Other.OriginalBones)
	, CharacterBones(Other.CharacterBones)
{
	Parts.Reserve(Other.Parts.Num());
	for (const TUniquePtr<FSolidBodyPart>& Part : Other.Parts)
	{
		Parts.Add(Part ? Part->Clone() : nullptr);
	}
}

SolidBodyMesh::FSolidBodyMesh& SolidBodyMesh::FSolidBodyMesh::operator=(const FSolidBodyMesh& Other)
{
	if (this != &Other)
	{
		OriginalBones = Other.OriginalBones;
		CharacterBones = Other.CharacterBones;
		Parts.Reset();
		Parts.Reserve(Other.Parts.Num());
		for (const TUniquePtr<FSolidBodyPart>& Part : Other.Parts)
		{
			Parts.Add(Part ? Part->Clone() : nullptr);
		}
	}
	return *this;
}

SolidBodyMesh::FSolidBodyMesh SolidBodyMesh::FSolidBodyMesh::Make(const TArray<FSolidDeformBone>& OriginalBones)
{
	constexpr ESolidPartSide Left = ESolidPartSide::Left;
	constexpr ESolidPartSide Right = ESolidPartSide::Right;
	constexpr ESolidPartSide Core = ESolidPartSide::Core;
	constexpr ESolidPartColour Shirt = ESolidPartColour::Shirt;
	constexpr ESolidPartColour Trousers = ESolidPartColour::Trousers;
	constexpr ESolidPartColour Skin = ESolidPartColour::Skin;

	FSolidBodyMesh Body;
	Body.OriginalBones = OriginalBones;
	Body.CharacterBones = OriginalBones;

	// The top of a part is the bottom of the part above it, where those ends already meet.
	const FSolidPartSection ToeTip = RoundSection(ToeTipDiameterCm);
	const FSolidPartSection ToeTop = RoundSection(ToeAnkleDiameterCm);
	const FSolidPartSection FootTop = RoundSection(BallDiameterCm);
	const FSolidPartSection CalfBottom = RoundSection(CalfBottomDiameterCm);
	const FSolidPartSection CalfTop = RoundSection(CalfTopDiameterCm);
	const FSolidPartSection ThighTop = RoundSection(LeftThighTopDiameterCm);
	const FSolidPartSection HipTop = OvalSection(HipTopSideDiameterCm, HipDepthDiameterCm);
	// The bottoms come back as the previous part's top, and the parts use that section.
	float JoinedBottomCm = 0.f;
	float AbdomenTopCm = 0.f;
	float ChestTopCm = 0.f;
	TorsoSideDiameters(0, 2, JoinedBottomCm, AbdomenTopCm);
	TorsoSideDiameters(1, 2, JoinedBottomCm, ChestTopCm);
	(void)JoinedBottomCm;
	const FSolidPartSection AbdomenTop = OvalSection(AbdomenTopCm, TorsoDepthDiameterCm);
	const FSolidPartSection ChestTop = OvalSection(ChestTopCm, TorsoDepthDiameterCm);
	const FSolidPartSection ShoulderTop = RoundSection(ShoulderTopRadiusCm * 2.f);
	const FSolidPartSection UpperArmTop = RoundSection(UpperArmElbowDiameterCm);
	const FSolidPartSection LowerArmTop = RoundSection(LowerArmStartDiameterCm);
	const FSolidPartSection NullSection = RoundSection(0.f);
	const FSolidPartSection HandTop = NullSection;

	Body.AddBodyPart(ESolidPartType::Thigh, Left, Trousers, ThighTop, CalfTop);
	Body.AddBodyPart(ESolidPartType::Thigh, Right, Trousers, ThighTop, CalfTop);
	// The calf ends narrower than the foot, so the ankle keeps its own step.
	Body.AddBodyPart(ESolidPartType::Calf, Left, Trousers, CalfTop, CalfBottom);
	Body.AddBodyPart(ESolidPartType::Calf, Right, Trousers, CalfTop, CalfBottom);
	Body.AddBodyPart(ESolidPartType::Foot, Left, Trousers, FootTop, ToeTop);
	Body.AddBodyPart(ESolidPartType::Foot, Right, Trousers, FootTop, ToeTop);
	Body.AddBodyPart(ESolidPartType::Toe, Left, Trousers, ToeTop, ToeTip);
	Body.AddBodyPart(ESolidPartType::Toe, Right, Trousers, ToeTop, ToeTip);

	// The hip mesh is the length of spine_01. The thighs sit inside it, so its base is wider than the thigh top.
	Body.AddBodyPart(ESolidPartType::Hip, Core, Shirt, OvalSection(HipSideDiameterCm, HipDepthDiameterCm), HipTop);

	// The abdomen is spine_02. The chest is spine_03.
	Body.AddBodyPart(ESolidPartType::Abdomen, Core, Shirt, HipTop, AbdomenTop);
	Body.AddBodyPart(ESolidPartType::Chest, Core, Shirt, AbdomenTop, ChestTop);

	// Shoulders follow neck_01. Mid is fitted around the clavicles and the upper arms when the mesh is built.
	Body.AddBodyPart(ESolidPartType::Shoulders, Core, Shirt, ChestTop, ShoulderTop);

	// upperarm joint to elbow. The skeleton names that bone lowerarm. The elbow then steps down to the lower arm.
	Body.AddBodyPart(ESolidPartType::UpperArm, Left, Shirt, UpperArmTop, RoundSection(ForearmWristDiameterCm));
	Body.AddBodyPart(ESolidPartType::UpperArm, Right, Shirt, UpperArmTop, RoundSection(ForearmWristDiameterCm));

	// elbow to wrist. The skeleton names that bone hand. Narrow at the elbow, wide at the wrist. The hand sizes itself.
	Body.AddBodyPart(ESolidPartType::LowerArm, Left, Shirt, LowerArmTop, RoundSection(LowerArmEndDiameterCm));
	Body.AddBodyPart(ESolidPartType::LowerArm, Right, Shirt, LowerArmTop, RoundSection(LowerArmEndDiameterCm));

	// The part lists the fingers on OriginalBones.
	Body.AddBodyPart(ESolidPartType::Hand, Left, Skin, HandTop, HandTop);
	Body.AddBodyPart(ESolidPartType::Hand, Right, Skin, HandTop, HandTop);

	// The part lists the head bone and the eye bones on OriginalBones. These diameters are unused.
	Body.AddBodyPart(ESolidPartType::Head, Core, Skin, NullSection, NullSection);

	return Body;
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

bool SolidBodyMesh::IsEyeBoneName(const FString& BoneName)
{
	return BoneName.ToLower().Contains(TEXT("eye"));
}

bool SolidBodyMesh::IsEyeJointBoneName(const FString& BoneName)
{
	return BoneName.ToLower().Contains(TEXT("eye_jnt"));
}

bool SolidBodyMesh::IsFingerBoneName(const FString& BoneName)
{
	const FString Lower = BoneName.ToLower();
	if (IsTwistBoneName(Lower))
	{
		return false;
	}
	return Lower.StartsWith(TEXT("thumb_"))
		|| Lower.StartsWith(TEXT("index_"))
		|| Lower.StartsWith(TEXT("middle_"))
		|| Lower.StartsWith(TEXT("ring_"))
		|| Lower.StartsWith(TEXT("pinky_"))
		|| Lower.StartsWith(TEXT("little_"));
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
		const SolidBodyMesh::FSolidBodyMesh Catalogue = SolidBodyMesh::FSolidBodyMesh::Make(Named);
		for (int32 Index = 0; Index < Infos.Num(); ++Index)
		{
			const FName BoneName = Infos[Index].Name;
			bool bWrapped = false;
			for (const TUniquePtr<SolidBodyMesh::FSolidBodyPart>& Part : Catalogue.Parts)
			{
				if (Part && Part->Bones.Contains(BoneName))
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

	TMap<uint32, TStrongObjectPtr<USkeletalMesh>>& BodyCache()
	{
		static TMap<uint32, TStrongObjectPtr<USkeletalMesh>> Cache;
		return Cache;
	}

	TMap<uint32, float>& CachedBaseZ()
	{
		static TMap<uint32, float> Bases;
		return Bases;
	}

	void PlantBase(USkeletalMeshComponent* MeshComp, const float BaseZ)
	{
		float HalfHeight = 96.f;
		if (const UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(MeshComp->GetAttachParent()))
		{
			HalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		}
		FVector Rel = MeshComp->GetRelativeLocation();
		Rel.Z = SolidBodyMesh::BaseMeshOffset(HalfHeight, BaseZ);
		MeshComp->SetRelativeLocation(Rel);
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
		const bool bReused,
		const float BaseZ)
	{
		if (MeshComp->GetSkeletalMeshAsset() != Body)
		{
			if (MeshComp->GetSkeletalMeshAsset())
			{
				// The last frame may still be drawing the previous body. Wait, then swap.
				FlushRenderingCommands();
			}
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
		PlantBase(MeshComp, BaseZ);
	}
}
#endif

USkeletalMesh* SolidBodyMesh::Build(
	const USkeletalMesh* Source,
	const FSolidBodyParams& Params,
	UObject* Outer,
	float& OutBaseZ)
{
	OutBaseZ = 0.f;
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
	// Lengths start as OriginalBones. ApplyCharacterAttributes stores CharacterBones on the body.
	const FSolidBodyParams Clamped = Params.Clamped();
	FSolidBodyMesh Body = FSolidBodyMesh::Make(Bind.Bones);
	Body.ApplyCharacterAttributes(Clamped);
	OutBaseZ = BodyBaseZ(Body);

	// The part list draws the triangles. The rest of this function hands them to Unreal.
	TArray<FSolidBodyVert> Verts;
	TArray<FSolidBodyTri> Tris;
	Body.Draw(Verts, Tris);
	FSolidDeformMesh CharacterPose;
	CharacterPose.Bones = Body.CharacterBones;
	if (Tris.Num() == 0 || Verts.Num() == 0)
	{
		UE_LOG(LogSolid, Warning, TEXT("SolidBody low-poly mesh has no segments, so it is discarded."));
		return nullptr;
	}

	FMeshDescription Description;
	if (!WriteDescription(Description, Ref, CharacterPose, Verts, Tris))
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
	ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, CharacterPose);
	ApplyWearColors(
		Mesh,
		FLinearColor(0.29f, 0.38f, 0.55f),
		FLinearColor(0.12f, 0.11f, 0.10f),
		SolidCompanionColors::SkinPink());

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
	ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, CharacterPose);
	Mesh->SetSkeleton(BodySkeleton);
	if (!HasRenderableBody(Mesh))
	{
		Mesh->Build();
		ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, CharacterPose);
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
		SolidCompanionColors::SkinPink());
	return Mesh;
#else
	(void)Source;
	(void)Params;
	(void)Outer;
	(void)OutBaseZ;
	return nullptr;
#endif
}

float SolidBodyMesh::BodyBaseZ(const FSolidBodyMesh& Body)
{
	return Body.Base();
}

float SolidBodyMesh::BodyBaseZ(const TArray<FSolidDeformBone>& Bones)
{
	const FSolidBodyMesh Body = FSolidBodyMesh::Make(Bones);
	return Body.Base();
}

float SolidBodyMesh::BaseMeshOffset(const float CapsuleHalfHeight, const float BaseZ)
{
	return -CapsuleHalfHeight - BaseZ;
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
	if (TStrongObjectPtr<USkeletalMesh>* Found = BodyCache().Find(Key))
	{
		if (USkeletalMesh* Cached = Found->Get())
		{
			if (HasRenderableBody(Cached))
			{
				WearBody(MeshComp, Cached, Seed, Params, DebugLabel, true, CachedBaseZ().FindRef(Key));
				ApplyComponentColors(MeshComp, TorsoColor, LegColor, SkinColor);
				return;
			}
		}
		BodyCache().Remove(Key);
		CachedBaseZ().Remove(Key);
	}
#endif

	USkeletalMesh* Source = SolidContentLookup::FindVikingMesh();
	if (!Source)
	{
		return;
	}

	float BaseZ = 0.f;
	USkeletalMesh* Built = Build(Source, Params, GetTransientPackage(), BaseZ);
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
	TStrongObjectPtr<USkeletalMesh> Kept(Built);
	BodyCache().Add(Key, MoveTemp(Kept));
	CachedBaseZ().Add(Key, BaseZ);
	WearBody(MeshComp, Built, Seed, Params, DebugLabel, false, BaseZ);
	ApplyComponentColors(MeshComp, TorsoColor, LegColor, SkinColor);
#else
	(void)Seed;
	(void)Params;
	(void)Built;
	(void)BaseZ;
	(void)TorsoColor;
	(void)LegColor;
	(void)SkinColor;
#endif
}

const SolidBodyMesh::FSolidBodyPart* SolidBodyMesh::FSolidBodyMesh::Find(const FString& Name) const
{
	for (const TUniquePtr<FSolidBodyPart>& Part : Parts)
	{
		if (Part && Part->Name().Equals(Name, ESearchCase::IgnoreCase))
		{
			return Part.Get();
		}
	}
	return nullptr;
}

void SolidBodyMesh::FSolidBodyMesh::ApplyCharacterAttributes(const FSolidBodyParams& Params)
{
	CharacterBones = OriginalBones;
	for (const TUniquePtr<FSolidBodyPart>& Part : Parts)
	{
		if (!Part)
		{
			continue;
		}
		Part->BoneLocals = CharacterBones;
		Part->ApplyCharacterAttributes(Params);
		CharacterBones = Part->BoneLocals;
	}
	for (const TUniquePtr<FSolidBodyPart>& Part : Parts)
	{
		if (Part)
		{
			Part->BoneLocals = CharacterBones;
		}
	}
}

void SolidBodyMesh::FSolidBodyMesh::Draw(
	TArray<FSolidBodyVert>& Verts,
	TArray<FSolidBodyTri>& Tris)
{
	TArray<FVector> JointPositions;
	SolidBodyDeform::ComponentPositions(CharacterBones, JointPositions);
	// shoulder_jnt stays bare. Twists stay bare. Hands draw their own fingers.
	const int32 Count = FMath::Min(CharacterBones.Num(), JointPositions.Num());
	for (const TUniquePtr<FSolidBodyPart>& Part : Parts)
	{
		if (!Part || Part->Bones.Num() == 0)
		{
			continue;
		}
		const int32 Span = FindNamedBone(CharacterBones, Count, *Part->Bones[0].ToString());
		if (Span == INDEX_NONE)
		{
			continue;
		}
		const int32 Parent = CharacterBones[Span].ParentIndex;
		if (!CharacterBones.IsValidIndex(Parent) || !JointPositions.IsValidIndex(Parent))
		{
			continue;
		}

		TArray<FVector> SegmentStart;
		TArray<FVector> SegmentEnd;
		bool bComplete = true;
		for (int32 BoneNumber = 1; BoneNumber < Part->Bones.Num(); ++BoneNumber)
		{
			const int32 Bone = FindNamedBone(CharacterBones, Count, *Part->Bones[BoneNumber].ToString());
			if (Bone == INDEX_NONE)
			{
				bComplete = false;
				break;
			}
			const int32 BoneParent = CharacterBones[Bone].ParentIndex;
			if (!CharacterBones.IsValidIndex(BoneParent) || !JointPositions.IsValidIndex(BoneParent))
			{
				bComplete = false;
				break;
			}
			SegmentStart.Add(JointPositions[BoneParent]);
			SegmentEnd.Add(JointPositions[Bone]);
		}
		if (!bComplete)
		{
			continue;
		}

		Part->Draw(
			Verts,
			Tris,
			Parent,
			Span,
			JointPositions[Parent],
			JointPositions[Span],
			SegmentStart.GetData(),
			SegmentEnd.GetData(),
			SegmentStart.Num(),
			FVector::ForwardVector);
	}
}

float SolidBodyMesh::FSolidBodyMesh::Base() const
{
	TArray<FVector> JointPositions;
	SolidBodyDeform::ComponentPositions(CharacterBones, JointPositions);
	float BaseZ = TNumericLimits<float>::Max();
	bool bFound = false;
	for (const TUniquePtr<FSolidBodyPart>& Part : Parts)
	{
		float PartZ = 0.f;
		if (!Part || !Part->Base(CharacterBones, JointPositions, PartZ))
		{
			continue;
		}
		BaseZ = FMath::Min(BaseZ, PartZ);
		bFound = true;
	}
	return bFound ? BaseZ : 0.f;
}
