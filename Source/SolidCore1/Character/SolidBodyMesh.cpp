#include "SolidBodyMesh.h"

#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
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
			Bone.Role = SolidBodyMesh::RoleForBoneName(Infos[Index].Name.ToString());
			Bone.LocalTranslation = Pose[Index].GetTranslation();
			Bone.LocalRotation = Pose[Index].GetRotation();
			Out.Bones.Add(Bone);
		}
	}

	int32 SkeletonIndexForDescriptionBone(
		const FReferenceSkeleton& Ref,
		const FName BoneName,
		const int32 DescriptionIndex)
	{
		const int32 ByName = Ref.FindBoneIndex(BoneName);
		if (ByName != INDEX_NONE)
		{
			return ByName;
		}
		if (DescriptionIndex >= 0 && DescriptionIndex < Ref.GetRawBoneNum())
		{
			return DescriptionIndex;
		}
		return INDEX_NONE;
	}

	void FillVertices(FMeshDescription& Description, const FReferenceSkeleton& Ref, FSolidDeformMesh& Out)
	{
		FSkeletalMeshAttributes Attributes(Description);
		const TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const FSkinWeightsVertexAttributesRef Skin = Attributes.GetVertexSkinWeights(NAME_None);
		const bool bHaveWeights = Skin.IsValid();

		TArray<int32> ToSkeleton;
		if (bHaveWeights)
		{
			int32 MaxId = 0;
			for (const FBoneID BoneID : Attributes.Bones().GetElementIDs())
			{
				MaxId = FMath::Max(MaxId, BoneID.GetValue());
			}
			ToSkeleton.Init(INDEX_NONE, MaxId + 1);
			const auto BoneNames = Attributes.GetBoneNames();
			for (const FBoneID BoneID : Attributes.Bones().GetElementIDs())
			{
				const int32 DescriptionIndex = BoneID.GetValue();
				const FName BoneName = BoneNames.IsValid() ? BoneNames.Get(BoneID) : NAME_None;
				ToSkeleton[DescriptionIndex] = SkeletonIndexForDescriptionBone(Ref, BoneName, DescriptionIndex);
			}
		}

		Out.Vertices.Reserve(Description.Vertices().Num());
		for (const FVertexID VertexID : Description.Vertices().GetElementIDs())
		{
			FSolidDeformVertex Vertex;
			Vertex.Position = FVector(Positions.Get(VertexID));
			if (bHaveWeights)
			{
				const FVertexBoneWeights Weights = Skin.Get(VertexID);
				int32 Slot = 0;
				for (const UE::AnimationCore::FBoneWeight& Weight : Weights)
				{
					if (Slot >= FSolidDeformVertex::MaxInfluences)
					{
						break;
					}
					const int32 RawIndex = static_cast<int32>(Weight.GetBoneIndex());
					const int32 BoneIndex = ToSkeleton.IsValidIndex(RawIndex) ? ToSkeleton[RawIndex] : INDEX_NONE;
					if (BoneIndex == INDEX_NONE || Weight.GetWeight() <= 0.f)
					{
						continue;
					}
					Vertex.BoneIndex[Slot] = BoneIndex;
					Vertex.Weight[Slot] = Weight.GetWeight();
					++Slot;
				}
			}
			Out.Vertices.Add(Vertex);
		}
	}

	void WritePose(FMeshDescription& Description, const FReferenceSkeleton& Ref, const FSolidDeformMesh& Deformed)
	{
		FSkeletalMeshAttributes Attributes(Description);
		const auto BoneNames = Attributes.GetBoneNames();
		auto BonePoses = Attributes.GetBonePoses();
		for (const FBoneID BoneID : Attributes.Bones().GetElementIDs())
		{
			const FName BoneName = BoneNames.IsValid() ? BoneNames.Get(BoneID) : NAME_None;
			const int32 Index = SkeletonIndexForDescriptionBone(Ref, BoneName, BoneID.GetValue());
			if (!Deformed.Bones.IsValidIndex(Index))
			{
				continue;
			}
			FTransform Pose(BonePoses.Get(BoneID));
			Pose.SetTranslation(Deformed.Bones[Index].LocalTranslation);
			Pose.SetRotation(Deformed.Bones[Index].LocalRotation);
			using FPoseValue = typename TDecay<decltype(BonePoses.Get(BoneID))>::Type;
			BonePoses.Set(BoneID, FPoseValue(Pose));
		}
	}

	void WriteVertices(FMeshDescription& Description, const FSolidDeformMesh& Deformed)
	{
		FSkeletalMeshAttributes Attributes(Description);
		TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
		int32 Index = 0;
		for (const FVertexID VertexID : Description.Vertices().GetElementIDs())
		{
			if (!Deformed.Vertices.IsValidIndex(Index))
			{
				break;
			}
			Positions.Set(VertexID, FVector3f(Deformed.Vertices[Index].Position));
			++Index;
		}
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

	// Build() may replace the bind pose. Put the Viking bone order back, then write the deform.
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

	FMeshDescription* SourceDescription = Source->GetMeshDescription(0);
	if (!SourceDescription || SourceDescription->Vertices().Num() == 0)
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
	FMeshDescription Description(*SourceDescription);
	FillVertices(Description, Ref, Bind);

	const FSolidDeformMesh Deformed = SolidBodyDeform::Apply(Bind, Params);
	WritePose(Description, Ref, Deformed);
	WriteVertices(Description, Deformed);

	USkeleton* BodySkeleton = DuplicateBodySkeleton(SourceSkeleton, Outer);
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
	Mesh->SetPhysicsAsset(const_cast<USkeletalMesh*>(Source)->GetPhysicsAsset());
	Mesh->NeverStream = true;
	Mesh->SetMaterials(Source->GetMaterials());
	Mesh->SetRefSkeleton(Ref);

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
		return nullptr;
	}
	Mesh->Build();
	ApplyDeformedRefPose(Mesh, BodySkeleton, Ref, Deformed);
	// The bind-pose edit drops the derived render data. Build it again before the mesh is worn.
	Mesh->SetSkeleton(BodySkeleton);
	Mesh->Build();
	Mesh->CalculateInvRefMatrices();
	if (!HasRenderableBody(Mesh))
	{
		const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
		UE_LOG(LogSolid, Warning,
			TEXT("SolidBody build has no wearable LOD (bones %d, lods %d), so it is discarded."),
			Mesh->GetRefSkeleton().GetRawBoneNum(),
			RenderData ? RenderData->LODRenderData.Num() : 0);
		return nullptr;
	}
	return Mesh;
#else
	(void)Source;
	(void)Params;
	(void)Outer;
	return nullptr;
#endif
}

void SolidBodyMesh::ApplySeed(
	USkeletalMeshComponent* MeshComp,
	const uint32 Seed,
	UObject* Outer,
	const TCHAR* DebugLabel)
{
	if (!MeshComp || !Outer)
	{
		return;
	}

	USkeletalMesh* Source = SolidContentLookup::FindVikingMesh();
	if (!Source)
	{
		return;
	}

	const FSolidBodyParams Params = FSolidBodyParams::FromSeed(Seed);
	USkeletalMesh* Built = Build(Source, Params, Outer);
	if (!Built)
	{
		UE_LOG(LogSolid, Warning,
			TEXT("%s body: could not build from SK_Viking, so the source mesh stays."),
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
#endif

	MeshComp->SetSkeletalMeshAsset(Built);
	MeshComp->SetAnimInstanceClass(nullptr);
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->InitAnim(true);
	UE_LOG(LogSolid, Warning,
		TEXT("%s body seed %u height %.2f bulk %.2f"),
		DebugLabel ? DebugLabel : TEXT("Body"),
		Seed,
		Params.Height,
		Params.Bulk);
}
