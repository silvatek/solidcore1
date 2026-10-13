#include "SolidBodyMesh.h"

#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
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
	struct FLoFiVert
	{
		FVector Position = FVector::ZeroVector;
		int32 Bone = INDEX_NONE;
		uint8 Slot = 0;
	};

	constexpr uint8 SlotTorso = 0;
	constexpr uint8 SlotLegs = 1;
	constexpr uint8 SlotSkin = 2;
	constexpr int32 TubeSides = 10;
	constexpr int32 TubeSegments = 3;

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
			const FString BoneName = Infos[Index].Name.ToString();
			Bone.ParentIndex = Infos[Index].ParentIndex;
			Bone.Role = SolidBodyMesh::RoleForBoneName(BoneName);
			Bone.bTwist = SolidBodyMesh::IsTwistBoneName(BoneName);
			Bone.bDigit = SolidBodyMesh::IsDigitBoneName(BoneName);
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

	uint8 SlotForRole(const ESolidBodyBoneRole Role)
	{
		if (Role == ESolidBodyBoneRole::Leg
			|| Role == ESolidBodyBoneRole::Foot
			|| Role == ESolidBodyBoneRole::Pelvis)
		{
			return SlotLegs;
		}
		if (Role == ESolidBodyBoneRole::Head)
		{
			return SlotSkin;
		}
		return SlotTorso;
	}

	/** Tapered tube. Flatten squishes the ring's second axis so a foot or a hand reads as a paddle. */
	void AddTube(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const int32 BoneIndex,
		const FVector& Start,
		const FVector& End,
		const float RadiusStart,
		const float RadiusEnd,
		const int32 Sides,
		const int32 Segments,
		const uint8 Slot,
		const float Flatten,
		const FVector& PreferredSide)
	{
		const FVector Delta = End - Start;
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
		const float Squash = FMath::Clamp(Flatten, 0.2f, 1.f);
		const int32 Base = Verts.Num();

		for (int32 Ring = 0; Ring < RingCount; ++Ring)
		{
			const float T = (RingCount == 1)
				? 0.f
				: static_cast<float>(Ring) / static_cast<float>(RingCount - 1);
			const FVector Center = FMath::Lerp(Start, End, static_cast<double>(T));
			const float Radius = FMath::Lerp(RadiusStart, RadiusEnd, T);
			for (int32 Step = 0; Step < SideCount; ++Step)
			{
				const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(SideCount);
				const FVector Offset = Side * (FMath::Cos(Angle) * Radius)
					+ Up * (FMath::Sin(Angle) * Radius * Squash);
				FLoFiVert Vert;
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

	/** Skull above the head joint. The front is flattened along mesh +Y so a face can sit on it. */
	void AddHead(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const int32 BoneIndex,
		const FVector& Joint,
		const FVector& Up,
		const float Height,
		const float Radius,
		const uint8 Slot)
	{
		FVector Forward = FVector(0.f, 1.f, 0.f) - Up * FVector::DotProduct(FVector(0.f, 1.f, 0.f), Up);
		if (Forward.SizeSquared() < 0.01)
		{
			Forward = FVector(1.f, 0.f, 0.f) - Up * FVector::DotProduct(FVector(1.f, 0.f, 0.f), Up);
		}
		Forward = Forward.GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Up, Forward).GetSafeNormal();
		Forward = FVector::CrossProduct(Side, Up).GetSafeNormal();

		constexpr int32 Sides = 10;
		constexpr int32 Rings = 6;
		const FVector Center = Joint + Up * (Height * 0.42);
		const int32 Base = Verts.Num();
		const float FacePlane = Radius * 0.42f;

		for (int32 Ring = 0; Ring < Rings; ++Ring)
		{
			const float T = static_cast<float>(Ring) / static_cast<float>(Rings - 1);
			const float Bell = FMath::Sin(T * PI);
			const float Slice = FMath::Lerp(SolidBodyMesh::HeadChinSlice, 1.f, Bell);
			const float Along = (T - 0.42f) * Height;
			for (int32 Step = 0; Step < Sides; ++Step)
			{
				const float Angle = (2.f * PI * static_cast<float>(Step)) / static_cast<float>(Sides);
				FVector Offset = (Forward * FMath::Cos(Angle) + Side * FMath::Sin(Angle)) * (Radius * Slice);
				const float ForwardAmt = FVector::DotProduct(Offset, Forward);
				if (ForwardAmt > FacePlane)
				{
					Offset -= Forward * (ForwardAmt - FacePlane);
				}
				FLoFiVert Vert;
				Vert.Position = Center + Up * Along + Offset;
				Vert.Bone = BoneIndex;
				Vert.Slot = Slot;
				Verts.Add(Vert);
			}
		}

		auto Id = [Base](const int32 Ring, const int32 Step)
		{
			return Base + Ring * Sides + (Step % Sides);
		};
		for (int32 Ring = 0; Ring < Rings - 1; ++Ring)
		{
			for (int32 Step = 0; Step < Sides; ++Step)
			{
				const int32 Next = Step + 1;
				AddTri(Tris, Id(Ring, Step), Id(Ring, Next), Id(Ring + 1, Step));
				AddTri(Tris, Id(Ring, Next), Id(Ring + 1, Next), Id(Ring + 1, Step));
			}
		}
		for (int32 Step = 1; Step < Sides - 1; ++Step)
		{
			AddTri(Tris, Id(0, 0), Id(0, Step + 1), Id(0, Step));
			AddTri(Tris, Id(Rings - 1, 0), Id(Rings - 1, Step), Id(Rings - 1, Step + 1));
		}
	}

	/** First limb bone under Parent, walking through twist bones. */
	int32 FindStructuralChild(const TArray<FSolidDeformBone>& Bones, const int32 Parent, const ESolidBodyBoneRole Role)
	{
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Bones[Index].ParentIndex != Parent)
			{
				continue;
			}
			if (Bones[Index].bTwist)
			{
				const int32 Deeper = FindStructuralChild(Bones, Index, Role);
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

	bool IsDigitBone(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		return Bones.IsValidIndex(Index) && Bones[Index].bDigit;
	}

	/** Walk past twist bones so a shin still starts at the calf. */
	int32 StructuralParent(const TArray<FSolidDeformBone>& Bones, const int32 Index)
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

	/**
	 * The ankle is the foot bone that hangs off the calf.
	 * The calf's own parent is the thigh, so the calf is the thigh segment, not the foot.
	 */
	bool IsAnkleBone(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		if (!Bones.IsValidIndex(Index)
			|| Bones[Index].Role != ESolidBodyBoneRole::Foot
			|| Bones[Index].bDigit
			|| Bones[Index].bTwist)
		{
			return false;
		}
		const int32 Parent = StructuralParent(Bones, Index);
		return Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Foot;
	}

	/**
	 * One cap from the chest out to the arm joint.
	 * Its narrow end is the upper arm. Its outer edge stays on the chest circle,
	 * which is already as wide as both arms. Helper clavicles draw nothing.
	 */
	void AddShoulder(
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris,
		const TArray<FSolidDeformBone>& Bones,
		const TArray<FVector>& Positions,
		const TArray<float>& Radii,
		const int32 Clavicle)
	{
		if (!Positions.IsValidIndex(Clavicle))
		{
			return;
		}

		const int32 Parent = StructuralParent(Bones, Clavicle);
		const bool bChest = Bones.IsValidIndex(Parent) && Positions.IsValidIndex(Parent);
		const int32 Arm = FindStructuralChild(Bones, Clavicle, ESolidBodyBoneRole::Arm);
		if (Arm == INDEX_NONE || !Positions.IsValidIndex(Arm) || !Radii.IsValidIndex(Arm))
		{
			return;
		}

		const FVector Cap = Positions[Clavicle];
		const FVector Chest = bChest ? Positions[Parent] : Cap;
		const FVector Joint = Positions[Arm];
		FVector Out = Joint - Chest;
		Out.Z = 0.0;
		if (Out.SizeSquared() < 1.0)
		{
			Out = Cap - Chest;
			Out.Z = 0.0;
		}
		if (Out.SizeSquared() < 1.0)
		{
			return;
		}
		Out = Out.GetSafeNormal();

		FVector Flat = Joint - Chest;
		Flat.Z = 0.0;
		const float Lateral = static_cast<float>(Flat.Size());
		const float EndRadius = Radii[Arm];
		const float ChestRadius = (bChest && Radii.IsValidIndex(Parent))
			? Radii[Parent]
			: SolidBodyMesh::SplitHalfWidth(Lateral, EndRadius);
		const float StartRadius = FMath::Max(EndRadius, FMath::Min(ChestRadius * 0.5f, EndRadius * 1.8f));
		const float StartLateral = FMath::Max(0.f, ChestRadius - StartRadius);
		const FVector Start = Chest + Out * static_cast<double>(StartLateral);
		const FVector End = Joint;
		if ((End - Start).Size() < 1.0)
		{
			return;
		}

		AddTube(
			Verts,
			Tris,
			Clavicle,
			Start,
			End,
			StartRadius,
			EndRadius,
			TubeSides,
			3,
			SlotTorso,
			1.f,
			FVector::UpVector);
	}

	void BuildLowFi(
		const TArray<FSolidDeformBone>& Bones,
		const TArray<FVector>& Positions,
		const FSolidBodyParams& Params,
		TArray<FLoFiVert>& Verts,
		TArray<FLoFiTri>& Tris)
	{
		TArray<float> Radii;
		SolidBodyMesh::JointRadii(Bones, Positions, Params, Radii);

		const int32 Count = FMath::Min(Bones.Num(), Positions.Num());
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const ESolidBodyBoneRole Role = Bones[Index].Role;
			if (Role == ESolidBodyBoneRole::Root
				|| Role == ESolidBodyBoneRole::Pelvis
				|| Bones[Index].bTwist
				|| IsDigitBone(Bones, Index))
			{
				continue;
			}

			const int32 Parent = StructuralParent(Bones, Index);
			const bool bParent = Bones.IsValidIndex(Parent) && Positions.IsValidIndex(Parent);
			const ESolidBodyBoneRole ParentRole = bParent
				? Bones[Parent].Role
				: ESolidBodyBoneRole::Root;
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

			// The calf's translation is the thigh. The ankle's translation is the shin.
			const bool bThigh = Role == ESolidBodyBoneRole::Foot && ParentRole == ESolidBodyBoneRole::Leg;
			const bool bShin = Role == ESolidBodyBoneRole::Foot && ParentRole == ESolidBodyBoneRole::Foot;
			// The shoulder cap owns the span from the clavicle to the upper arm.
			const bool bShoulderSpan = Role == ESolidBodyBoneRole::Arm && ParentRole == ESolidBodyBoneRole::Clavicle;
			// The hip joint is where the thigh cone starts. A cone from the pelvis center
			// out to that joint is the crumpled mass in the groin.
			const bool bHipStub = Role == ESolidBodyBoneRole::Leg && ParentRole == ESolidBodyBoneRole::Pelvis;
			const bool bTorsoSpan = Role == ESolidBodyBoneRole::Spine || Role == ESolidBodyBoneRole::Neck;
			const float MinIncoming = bTorsoSpan ? 0.5f : 4.f;

			if (Role == ESolidBodyBoneRole::Clavicle)
			{
				AddShoulder(Verts, Tris, Bones, Positions, Radii, Index);
			}
			else if (!bShoulderSpan && !bHipStub && bParent && ParentRole != ESolidBodyBoneRole::Root && Incoming >= MinIncoming)
			{
				float RadiusStart = 0.f;
				float RadiusEnd = 0.f;
				SolidBodyMesh::SegmentRadii(Radii, Parent, Index, RadiusStart, RadiusEnd);
				// The cone that ends on the chin is skin. The shirt stops where the neck begins.
				const uint8 Slot = (Role == ESolidBodyBoneRole::Head)
					? SlotSkin
					: ((bThigh || bShin || Role == ESolidBodyBoneRole::Leg || Role == ESolidBodyBoneRole::Foot)
						? SlotLegs
						: SlotTorso);
				AddTube(
					Verts,
					Tris,
					Parent,
					Positions[Parent],
					Positions[Index],
					RadiusStart,
					RadiusEnd,
					TubeSides,
					TubeSegments,
					Slot,
					1.f,
					FVector::UpVector);
			}

			if (Role == ESolidBodyBoneRole::Head)
			{
				const float Height = SolidBodyMesh::HeadHeightCm(Params);
				AddHead(
					Verts,
					Tris,
					Index,
					Positions[Index],
					Dir,
					Height,
					Height * SolidBodyMesh::HeadRadiusOverHeight,
					SlotSkin);
			}
			else if (Role == ESolidBodyBoneRole::Hand && bParent)
			{
				const float Length = FMath::Max(Incoming * 0.5f, 8.f);
				const float Radius = Radii.IsValidIndex(Index) ? Radii[Index] : SolidBodyMesh::JointWrist;
				FVector Wide = FVector::CrossProduct(Dir, FVector::UpVector);
				if (Wide.SizeSquared() < 0.01)
				{
					Wide = FVector::CrossProduct(Dir, FVector::RightVector);
				}
				AddTube(
					Verts,
					Tris,
					Index,
					Positions[Index],
					Positions[Index] + Dir * static_cast<double>(Length),
					Radius,
					Radius * 1.15f,
					8,
					2,
					SlotSkin,
					0.42f,
					Wide);
			}
			else if (IsAnkleBone(Bones, Index))
			{
				// Mesh +Y is the face. Both feet point that way, level, and meet the shin.
				const float Length = FMath::Clamp(16.f * Params.Height, 12.f, 22.f);
				const float Radius = Radii.IsValidIndex(Index) ? Radii[Index] : SolidBodyMesh::JointAnkle;
				const FVector Ankle = Positions[Index];
				FVector Toe = Ankle + FVector(0.f, Length, -2.f);
				AddTube(
					Verts,
					Tris,
					Index,
					Ankle,
					Toe,
					Radius,
					Radius * 0.72f,
					8,
					2,
					SlotLegs,
					0.55f,
					FVector::RightVector);
			}
			else if (bParent && IsChainEnd(Bones, Index))
			{
				const float TipLen = FMath::Max(Incoming * 0.35f, 4.f);
				const float Radius = Radii.IsValidIndex(Index) ? Radii[Index] : SolidBodyMesh::JointWrist;
				AddTube(
					Verts,
					Tris,
					Index,
					Positions[Index],
					Positions[Index] + Dir * static_cast<double>(TipLen),
					Radius,
					Radius * 0.75f,
					TubeSides,
					2,
					SlotForRole(Role),
					1.f,
					FVector::UpVector);
			}
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
			FPolygonGroupID Group = TorsoGroup;
			if (Verts[Tri.V[0]].Slot == SlotLegs)
			{
				Group = LegGroup;
			}
			else if (Verts[Tri.V[0]].Slot == SlotSkin)
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
	SolidBodyDeform::FitHeadLengths(Bind.Bones);
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
