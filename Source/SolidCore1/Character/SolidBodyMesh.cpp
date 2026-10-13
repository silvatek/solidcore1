#include "SolidBodyMesh.h"

#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
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
	struct FLoFiVert
	{
		FVector Position = FVector::ZeroVector;
		int32 Bone = INDEX_NONE;
	};

	struct FLoFiTri
	{
		int32 V[3] = { INDEX_NONE, INDEX_NONE, INDEX_NONE };
	};

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

	void AddTri(TArray<FLoFiTri>& Tris, const int32 A, const int32 B, const int32 C)
	{
		FLoFiTri Tri;
		Tri.V[0] = A;
		Tri.V[1] = B;
		Tri.V[2] = C;
		Tris.Add(Tri);
	}

	/** Four-sided tube from Start to End, skinned to one bone. */
	void AddPrism(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const int32 BoneIndex,
		const FVector& Start,
		const FVector& End,
		const float Radius)
	{
		const FVector Delta = End - Start;
		const float Length = Delta.Size();
		if (Length < 1.f || Radius < 0.5f || BoneIndex == INDEX_NONE)
		{
			return;
		}

		const FVector Axis = Delta / Length;
		FVector Side = FVector::CrossProduct(FVector::UpVector, Axis);
		if (Side.SizeSquared() < 0.01)
		{
			Side = FVector::CrossProduct(FVector::ForwardVector, Axis);
		}
		Side = Side.GetSafeNormal();
		const FVector Up = FVector::CrossProduct(Axis, Side).GetSafeNormal();

		constexpr int32 Sides = 4;
		const int32 Base = Verts.Num();
		for (int32 Ring = 0; Ring < 2; ++Ring)
		{
			const FVector Center = (Ring == 0) ? Start : End;
			for (int32 Step = 0; Step < Sides; ++Step)
			{
				const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(Sides);
				const FVector Offset = (Side * FMath::Cos(Angle) + Up * FMath::Sin(Angle)) * static_cast<double>(Radius);
				FLoFiVert Vert;
				Vert.Position = Center + Offset;
				Vert.Bone = BoneIndex;
				Verts.Add(Vert);
			}
		}

		auto Id = [Base](const int32 Ring, const int32 Step)
		{
			return Base + Ring * Sides + (Step % Sides);
		};

		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const int32 Next = Step + 1;
			const int32 A = Id(0, Step);
			const int32 B = Id(0, Next);
			const int32 C = Id(1, Next);
			const int32 D = Id(1, Step);
			AddTri(Tris, A, B, D);
			AddTri(Tris, B, C, D);
		}
		AddTri(Tris, Id(0, 0), Id(0, 2), Id(0, 1));
		AddTri(Tris, Id(0, 0), Id(0, 3), Id(0, 2));
		AddTri(Tris, Id(1, 0), Id(1, 1), Id(1, 2));
		AddTri(Tris, Id(1, 0), Id(1, 2), Id(1, 3));
	}

	void AddBox(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const int32 BoneIndex,
		const FVector& Center,
		const FVector& AxisX,
		const FVector& AxisY,
		const FVector& AxisZ,
		const float HalfX,
		const float HalfY,
		const float HalfZ)
	{
		if (BoneIndex == INDEX_NONE || HalfX < 0.5f || HalfY < 0.5f || HalfZ < 0.5f)
		{
			return;
		}

		const int32 Base = Verts.Num();
		for (int32 Z = 0; Z < 2; ++Z)
		{
			for (int32 Y = 0; Y < 2; ++Y)
			{
				for (int32 X = 0; X < 2; ++X)
				{
					const float SignX = (X == 0) ? -HalfX : HalfX;
					const float SignY = (Y == 0) ? -HalfY : HalfY;
					const float SignZ = (Z == 0) ? -HalfZ : HalfZ;
					FLoFiVert Vert;
					Vert.Position = Center
						+ AxisX * static_cast<double>(SignX)
						+ AxisY * static_cast<double>(SignY)
						+ AxisZ * static_cast<double>(SignZ);
					Vert.Bone = BoneIndex;
					Verts.Add(Vert);
				}
			}
		}

		auto Corner = [Base](const int32 X, const int32 Y, const int32 Z)
		{
			return Base + X + Y * 2 + Z * 4;
		};
		auto Face = [&Tris](const int32 A, const int32 B, const int32 C, const int32 D)
		{
			AddTri(Tris, A, B, C);
			AddTri(Tris, A, C, D);
		};

		Face(Corner(1, 0, 0), Corner(1, 1, 0), Corner(1, 1, 1), Corner(1, 0, 1));
		Face(Corner(0, 1, 0), Corner(0, 0, 0), Corner(0, 0, 1), Corner(0, 1, 1));
		Face(Corner(1, 1, 0), Corner(0, 1, 0), Corner(0, 1, 1), Corner(1, 1, 1));
		Face(Corner(0, 0, 0), Corner(1, 0, 0), Corner(1, 0, 1), Corner(0, 0, 1));
		Face(Corner(0, 0, 1), Corner(1, 0, 1), Corner(1, 1, 1), Corner(0, 1, 1));
		Face(Corner(0, 1, 0), Corner(1, 1, 0), Corner(1, 0, 0), Corner(0, 0, 0));
	}

	float SectionRadius(const ESolidBodyBoneRole Role, const float Length, const FSolidBodyParams& Params)
	{
		float Fraction = 0.18f;
		float Girth = Params.Bulk;
		switch (Role)
		{
		case ESolidBodyBoneRole::Spine:
		case ESolidBodyBoneRole::Pelvis:
			Fraction = 0.42f;
			Girth = Params.TorsoGirth;
			break;
		case ESolidBodyBoneRole::Neck:
			Fraction = 0.22f;
			Girth = Params.TorsoGirth;
			break;
		case ESolidBodyBoneRole::Clavicle:
			Fraction = 0.12f;
			break;
		case ESolidBodyBoneRole::Arm:
			Fraction = 0.16f;
			break;
		case ESolidBodyBoneRole::Hand:
			Fraction = 0.18f;
			break;
		case ESolidBodyBoneRole::Leg:
			Fraction = 0.22f;
			break;
		case ESolidBodyBoneRole::Foot:
			Fraction = 0.18f;
			break;
		case ESolidBodyBoneRole::Head:
			Fraction = 0.42f;
			Girth = Params.HeadSize;
			break;
		default:
			break;
		}
		return FMath::Clamp(FMath::Max(Length * Fraction, 2.5f) * Girth, 1.5f, 28.f);
	}

	int32 FindChild(const TArray<FSolidDeformBone>& Bones, const int32 Parent, const ESolidBodyBoneRole Role)
	{
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Bones[Index].ParentIndex == Parent && Bones[Index].Role == Role)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}

	bool ContinuesChain(const ESolidBodyBoneRole Role, const ESolidBodyBoneRole Child)
	{
		switch (Role)
		{
		case ESolidBodyBoneRole::Arm:
			return Child == ESolidBodyBoneRole::Arm || Child == ESolidBodyBoneRole::Hand;
		case ESolidBodyBoneRole::Hand:
			return Child == ESolidBodyBoneRole::Hand;
		case ESolidBodyBoneRole::Leg:
			return Child == ESolidBodyBoneRole::Leg || Child == ESolidBodyBoneRole::Foot;
		case ESolidBodyBoneRole::Foot:
			return Child == ESolidBodyBoneRole::Foot;
		default:
			return false;
		}
	}

	bool IsChainEnd(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		const ESolidBodyBoneRole Role = Bones[Index].Role;
		if (Role != ESolidBodyBoneRole::Arm
			&& Role != ESolidBodyBoneRole::Hand
			&& Role != ESolidBodyBoneRole::Leg
			&& Role != ESolidBodyBoneRole::Foot)
		{
			return false;
		}
		for (int32 Child = 0; Child < Bones.Num(); ++Child)
		{
			if (Bones[Child].ParentIndex == Index && ContinuesChain(Role, Bones[Child].Role))
			{
				return false;
			}
		}
		return true;
	}

	void AddPelvisVolume(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const TArray<FSolidDeformBone>& Bones,
		const TArray<FVector>& Positions,
		const int32 Pelvis,
		const FSolidBodyParams& Params)
	{
		const int32 Spine = FindChild(Bones, Pelvis, ESolidBodyBoneRole::Spine);
		FVector Up = FVector::UpVector;
		if (Positions.IsValidIndex(Spine))
		{
			const FVector ToSpine = Positions[Spine] - Positions[Pelvis];
			if (!ToSpine.IsNearlyZero())
			{
				Up = ToSpine.GetSafeNormal();
			}
		}

		TArray<int32, TInlineAllocator<4>> Legs;
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Bones[Index].ParentIndex == Pelvis && Bones[Index].Role == ESolidBodyBoneRole::Leg)
			{
				Legs.Add(Index);
			}
		}

		FVector Side = FVector::CrossProduct(Up, FVector::ForwardVector);
		if (Legs.Num() >= 2 && Positions.IsValidIndex(Legs[0]) && Positions.IsValidIndex(Legs[1]))
		{
			Side = Positions[Legs[1]] - Positions[Legs[0]];
		}
		else if (Legs.Num() == 1 && Positions.IsValidIndex(Legs[0]))
		{
			Side = Positions[Legs[0]] - Positions[Pelvis];
		}
		Side = Side - Up * FVector::DotProduct(Side, Up);
		if (Side.SizeSquared() < 0.01)
		{
			Side = FVector::CrossProduct(Up, FVector::ForwardVector);
			if (Side.SizeSquared() < 0.01)
			{
				Side = FVector::CrossProduct(Up, FVector::RightVector);
			}
		}
		Side = Side.GetSafeNormal();
		const FVector Forward = FVector::CrossProduct(Up, Side).GetSafeNormal();
		Side = FVector::CrossProduct(Forward, Up).GetSafeNormal();

		float HalfWidth = 8.f * Params.HipWidth;
		for (const int32 Leg : Legs)
		{
			if (!Positions.IsValidIndex(Leg))
			{
				continue;
			}
			const float Span = FMath::Abs(FVector::DotProduct(Positions[Leg] - Positions[Pelvis], Side));
			HalfWidth = FMath::Max(HalfWidth, static_cast<float>(Span) * 1.05f);
		}

		float HalfHeight = HalfWidth * 0.45f;
		if (Positions.IsValidIndex(Spine))
		{
			const float ToSpine = (Positions[Spine] - Positions[Pelvis]).Size();
			HalfHeight = FMath::Max(static_cast<float>(ToSpine) * 0.45f, HalfWidth * 0.35f);
		}
		HalfHeight *= Params.TorsoGirth;
		const float HalfDepth = HalfWidth * 0.62f * Params.TorsoGirth;
		AddBox(Verts, Tris, Pelvis, Positions[Pelvis], Side, Forward, Up, HalfWidth, HalfDepth, HalfHeight);
	}

	void BuildLowFi(
		const TArray<FSolidDeformBone>& Bones,
		const TArray<FVector>& Positions,
		const FSolidBodyParams& Params,
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris)
	{
		const int32 Count = FMath::Min(Bones.Num(), Positions.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			if (Bones[Index].Role == ESolidBodyBoneRole::Pelvis)
			{
				AddPelvisVolume(Verts, Tris, Bones, Positions, Index, Params);
			}
		}

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const ESolidBodyBoneRole Role = Bones[Index].Role;
			if (Role == ESolidBodyBoneRole::Root || Role == ESolidBodyBoneRole::Pelvis)
			{
				continue;
			}

			const int32 Parent = Bones[Index].ParentIndex;
			const bool bParent = Bones.IsValidIndex(Parent) && Positions.IsValidIndex(Parent);
			FVector Dir = FVector::UpVector;
			float Incoming = 0.f;
			if (bParent)
			{
				const FVector Delta = Positions[Index] - Positions[Parent];
				Incoming = Delta.Size();
				if (Incoming > 1.f)
				{
					Dir = Delta / Incoming;
				}
			}

			if (bParent && Bones[Parent].Role != ESolidBodyBoneRole::Root)
			{
				const ESolidBodyBoneRole RadiusRole = (Bones[Parent].Role == ESolidBodyBoneRole::Pelvis)
					? Role
					: Bones[Parent].Role;
				AddPrism(
					Verts,
					Tris,
					Parent,
					Positions[Parent],
					Positions[Index],
					SectionRadius(RadiusRole, Incoming, Params));
			}

			if (Role == ESolidBodyBoneRole::Head)
			{
				const float TipLen = FMath::Max(Incoming, 16.f) * Params.HeadSize;
				AddPrism(
					Verts,
					Tris,
					Index,
					Positions[Index],
					Positions[Index] + Dir * static_cast<double>(TipLen),
					TipLen * 0.4f);
			}
			else if (bParent && IsChainEnd(Bones, Index))
			{
				const float TipLen = FMath::Max(Incoming * 0.4f, 3.f);
				const ESolidBodyBoneRole RadiusRole = Bones[Parent].Role == ESolidBodyBoneRole::Root
					? Role
					: Bones[Parent].Role;
				AddPrism(
					Verts,
					Tris,
					Index,
					Positions[Index],
					Positions[Index] + Dir * static_cast<double>(TipLen),
					SectionRadius(RadiusRole, FMath::Max(Incoming, TipLen), Params));
			}
		}
	}

	FLinearColor SkinColorFor(const FSolidBodyParams& Params)
	{
		uint32 Hash = 2166136261u;
		auto Mix = [&Hash](const float Value)
		{
			uint32 Bits = 0;
			FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
			Hash ^= Bits;
			Hash *= 16777619u;
		};
		Mix(Params.Height);
		Mix(Params.ShoulderWidth);
		Mix(Params.HipWidth);
		Mix(Params.TorsoGirth);
		Mix(Params.ArmLength);
		Mix(Params.LegLength);
		Mix(Params.HeadSize);
		Mix(Params.NeckLength);
		Mix(Params.Bulk);

		FRandomStream Stream(static_cast<int32>(Hash));
		return FMath::Lerp(
			FLinearColor(0.23f, 0.13f, 0.08f),
			FLinearColor(0.94f, 0.76f, 0.61f),
			Stream.GetFraction());
	}

	void ApplySkinMaterial(USkeletalMesh* Mesh, const FSolidBodyParams& Params)
	{
		UMaterialInterface* Material = SolidMaterials::CreateSolidColor(
			Mesh,
			SkinColorFor(Params),
			TEXT("BodySkin"),
			0.68f);
		if (!Material)
		{
			return;
		}

		TArray<FSkeletalMaterial> Slots;
		FSkeletalMaterial Slot;
		Slot.MaterialInterface = Material;
		Slot.MaterialSlotName = TEXT("Body");
		Slot.ImportedMaterialSlotName = TEXT("Body");
		Slots.Add(Slot);
		Mesh->SetMaterials(Slots);
	}

	bool WriteDescription(
		FMeshDescription& Description,
		const FReferenceSkeleton& Ref,
		const FSolidDeformMesh& Deformed,
		const TArray<FLoFiVert>& Verts,
		const TArray<FLoFiTri>& Tris)
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

		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
		if (SlotNames.IsValid())
		{
			SlotNames.Set(Group, FName(TEXT("Body")));
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
		for (const FLoFiTri& Tri : Tris)
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
	const FSolidBodyParams Clamped = Params.Clamped();
	const FSolidDeformMesh Deformed = SolidBodyDeform::Apply(Bind, Clamped);

	TArray<FVector> Positions;
	SolidBodyDeform::ComponentPositions(Deformed.Bones, Positions);

	TArray<FLoFiVert> Verts;
	TArray<FLoFiTri> Tris;
	BuildLowFi(Deformed.Bones, Positions, Clamped, Verts, Tris);
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
	ApplySkinMaterial(Mesh, Clamped);

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
	ApplySkinMaterial(Mesh, Clamped);
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

	const FSolidBodyParams Params = FSolidBodyParams::FromSeed(Seed);

#if WITH_EDITORONLY_DATA
	if (TWeakObjectPtr<USkeletalMesh>* Found = BodyCache().Find(Seed))
	{
		if (USkeletalMesh* Cached = Found->Get())
		{
			if (HasRenderableBody(Cached))
			{
				WearBody(MeshComp, Cached, Seed, Params, DebugLabel, true);
				return;
			}
		}
		BodyCache().Remove(Seed);
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
	BodyCache().Add(Seed, Built);
	WearBody(MeshComp, Built, Seed, Params, DebugLabel, false);
#else
	(void)Built;
#endif
}
