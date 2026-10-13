#include "SolidBodyDeform.h"

namespace
{
	struct FComponentPose
	{
		TArray<FVector> Position;
		TArray<FQuat> Rotation;
	};

	bool IsBone(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		return Bones.IsValidIndex(Index);
	}

	void BuildBone(
		const int32 Index,
		const TArray<FSolidDeformBone>& Bones,
		FComponentPose& Out,
		TArray<uint8>& State,
		const int32 Depth)
	{
		if (!IsBone(Bones, Index) || State[Index] == 2)
		{
			return;
		}
		if (State[Index] == 1 || Depth > Bones.Num())
		{
			State[Index] = 2;
			Out.Position[Index] = Bones[Index].LocalTranslation;
			Out.Rotation[Index] = Bones[Index].LocalRotation;
			return;
		}

		State[Index] = 1;
		const int32 Parent = Bones[Index].ParentIndex;
		FVector ParentPos = FVector::ZeroVector;
		FQuat ParentRot = FQuat::Identity;
		if (IsBone(Bones, Parent) && Parent != Index)
		{
			BuildBone(Parent, Bones, Out, State, Depth + 1);
			if (State[Parent] == 2)
			{
				ParentPos = Out.Position[Parent];
				ParentRot = Out.Rotation[Parent];
			}
		}

		Out.Rotation[Index] = ParentRot * Bones[Index].LocalRotation;
		Out.Position[Index] = ParentPos + ParentRot.RotateVector(Bones[Index].LocalTranslation);
		State[Index] = 2;
	}

	void BuildComponentPose(const TArray<FSolidDeformBone>& Bones, FComponentPose& Out)
	{
		const int32 Count = Bones.Num();
		Out.Position.SetNum(Count);
		Out.Rotation.SetNum(Count);
		TArray<uint8> State;
		State.Init(0, Count);
		for (int32 Index = 0; Index < Count; ++Index)
		{
			BuildBone(Index, Bones, Out, State, 0);
		}
	}

	/**
	 * Scale the component-space offset from parent joint to this joint.
	 * Limb length scales the whole segment, so an A-pose keeps its angle.
	 * Hip width then widens the horizontal part of the thigh where it leaves the pelvis.
	 */
	FVector ScaleSegment(
		const ESolidBodyBoneRole Role,
		const ESolidBodyBoneRole ParentRole,
		const FVector& Offset,
		const FSolidBodyParams& Params)
	{
		switch (Role)
		{
		case ESolidBodyBoneRole::Leg:
		case ESolidBodyBoneRole::Foot:
		{
			const float Length = Params.LegLength * Params.Height;
			FVector Scaled = Offset * Length;
			if (Role == ESolidBodyBoneRole::Leg
				&& (ParentRole == ESolidBodyBoneRole::Pelvis || ParentRole == ESolidBodyBoneRole::Root))
			{
				Scaled.X *= Params.HipWidth;
				Scaled.Y *= Params.HipWidth;
			}
			return Scaled;
		}
		case ESolidBodyBoneRole::Arm:
		case ESolidBodyBoneRole::Hand:
			return Offset * Params.ArmLength;
		case ESolidBodyBoneRole::Spine:
			return Offset * Params.Height;
		case ESolidBodyBoneRole::Neck:
			return Offset * Params.NeckLength;
		case ESolidBodyBoneRole::Clavicle:
			return Offset * Params.ShoulderWidth;
		default:
			return Offset;
		}
	}

	float GirthFor(const ESolidBodyBoneRole Role, const FSolidBodyParams& Params)
	{
		switch (Role)
		{
		case ESolidBodyBoneRole::Leg:
		case ESolidBodyBoneRole::Foot:
		case ESolidBodyBoneRole::Arm:
		case ESolidBodyBoneRole::Hand:
			return Params.Bulk;
		case ESolidBodyBoneRole::Spine:
		case ESolidBodyBoneRole::Pelvis:
			return Params.TorsoGirth;
		default:
			return 1.f;
		}
	}

	FVector SegmentAxis(
		const TArray<FSolidDeformBone>& Bones,
		const FComponentPose& Pose,
		const int32 Index)
	{
		const int32 Parent = Bones[Index].ParentIndex;
		if (IsBone(Bones, Parent))
		{
			const FVector Incoming = Pose.Position[Index] - Pose.Position[Parent];
			if (!Incoming.IsNearlyZero())
			{
				return Incoming.GetSafeNormal();
			}
		}
		for (int32 Child = 0; Child < Bones.Num(); ++Child)
		{
			if (Bones[Child].ParentIndex == Index)
			{
				const FVector Outgoing = Pose.Position[Child] - Pose.Position[Index];
				if (!Outgoing.IsNearlyZero())
				{
					return Outgoing.GetSafeNormal();
				}
			}
		}
		return FVector::UpVector;
	}

	/** Push flesh off the bone. Distance along the bone stays. */
	FVector ScaleFlesh(const FVector& Local, const FVector& Axis, const float Girth)
	{
		const float Along = FVector::DotProduct(Local, Axis);
		const FVector Perp = Local - Axis * Along;
		return Axis * Along + Perp * Girth;
	}

	FVector DeformLocal(
		const FVector& Local,
		const ESolidBodyBoneRole Role,
		const FVector& Axis,
		const FSolidBodyParams& Params)
	{
		if (Role == ESolidBodyBoneRole::Head)
		{
			if (FMath::IsNearlyEqual(Params.HeadSize, 1.f))
			{
				return Local;
			}
			return Local * Params.HeadSize;
		}

		const float Girth = GirthFor(Role, Params);
		if (FMath::IsNearlyEqual(Girth, 1.f))
		{
			return Local;
		}
		return ScaleFlesh(Local, Axis, Girth);
	}

	int32 StructuralParentIndex(const TArray<FSolidDeformBone>& Bones, const int32 Index)
	{
		int32 Parent = Bones.IsValidIndex(Index) ? Bones[Index].ParentIndex : INDEX_NONE;
		int32 Guard = 0;
		while (Bones.IsValidIndex(Parent) && Parent != Index && Bones[Parent].bTwist && Guard < Bones.Num())
		{
			Parent = Bones[Parent].ParentIndex;
			++Guard;
		}
		return Parent;
	}

	bool IsStructuralBone(const FSolidDeformBone& Bone)
	{
		return !Bone.bTwist && !Bone.bDigit;
	}

	int32 SpineChainDepth(
		const TArray<int32>& SpineParent,
		const int32 Index,
		TArray<uint8>& Visiting,
		const int32 Guard)
	{
		if (Index == INDEX_NONE || !Visiting.IsValidIndex(Index) || Guard > SpineParent.Num() || Visiting[Index])
		{
			return 0;
		}
		Visiting[Index] = 1;
		int32 Best = 0;
		for (int32 Child = 0; Child < SpineParent.Num(); ++Child)
		{
			if (SpineParent[Child] == Index)
			{
				Best = FMath::Max(Best, 1 + SpineChainDepth(SpineParent, Child, Visiting, Guard + 1));
			}
		}
		Visiting[Index] = 0;
		return Best;
	}

	/** Longest run of spine joints from the pelvis toward the chest. */
	void CollectSpineChain(const TArray<FSolidDeformBone>& Bones, TArray<int32>& OutChain)
	{
		TArray<int32> SpineParent;
		SpineParent.Init(INDEX_NONE, Bones.Num());
		TArray<int32> Spines;
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (!IsStructuralBone(Bones[Index]) || Bones[Index].Role != ESolidBodyBoneRole::Spine)
			{
				continue;
			}
			Spines.Add(Index);
			const int32 Parent = StructuralParentIndex(Bones, Index);
			if (Bones.IsValidIndex(Parent)
				&& IsStructuralBone(Bones[Parent])
				&& Bones[Parent].Role == ESolidBodyBoneRole::Spine)
			{
				SpineParent[Index] = Parent;
			}
		}
		if (Spines.Num() == 0)
		{
			return;
		}

		TArray<uint8> Visiting;
		Visiting.Init(0, Bones.Num());

		bool bSawPelvis = false;
		for (const int32 Index : Spines)
		{
			if (SpineParent[Index] != INDEX_NONE)
			{
				continue;
			}
			const int32 Parent = StructuralParentIndex(Bones, Index);
			if (Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Pelvis)
			{
				bSawPelvis = true;
				break;
			}
		}

		int32 BestRoot = INDEX_NONE;
		int32 BestDepth = -1;
		for (const int32 Index : Spines)
		{
			if (SpineParent[Index] != INDEX_NONE)
			{
				continue;
			}
			const int32 Parent = StructuralParentIndex(Bones, Index);
			const bool bPelvis = Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Pelvis;
			if (bSawPelvis && !bPelvis)
			{
				continue;
			}
			const int32 Depth = SpineChainDepth(SpineParent, Index, Visiting, 0);
			if (Depth > BestDepth)
			{
				BestDepth = Depth;
				BestRoot = Index;
			}
		}
		if (BestRoot == INDEX_NONE)
		{
			return;
		}

		int32 Cursor = BestRoot;
		int32 Guard = 0;
		while (Cursor != INDEX_NONE && Guard < Bones.Num())
		{
			OutChain.Add(Cursor);
			int32 Next = INDEX_NONE;
			int32 NextDepth = -1;
			for (const int32 Child : Spines)
			{
				if (SpineParent[Child] != Cursor)
				{
					continue;
				}
				const int32 Depth = SpineChainDepth(SpineParent, Child, Visiting, 0);
				if (Depth > NextDepth)
				{
					NextDepth = Depth;
					Next = Child;
				}
			}
			Cursor = Next;
			++Guard;
		}
	}
}

void SolidBodyDeform::ComponentPositions(
	const TArray<FSolidDeformBone>& Bones,
	TArray<FVector>& OutPositions)
{
	FComponentPose Pose;
	BuildComponentPose(Bones, Pose);
	OutPositions = MoveTemp(Pose.Position);
}

void SolidBodyDeform::FitHeadLengths(TArray<FSolidDeformBone>& Bones)
{
	if (Bones.Num() == 0)
	{
		return;
	}

	TArray<int32> Heads;
	TArray<int32> Ankles;
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (!IsStructuralBone(Bones[Index]))
		{
			continue;
		}
		if (Bones[Index].Role == ESolidBodyBoneRole::Head)
		{
			Heads.Add(Index);
			continue;
		}
		if (Bones[Index].Role != ESolidBodyBoneRole::Foot)
		{
			continue;
		}
		const int32 Parent = StructuralParentIndex(Bones, Index);
		if (Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Foot)
		{
			Ankles.Add(Index);
		}
	}
	if (Heads.Num() == 0 || Ankles.Num() == 0)
	{
		return;
	}

	FComponentPose OldPose;
	BuildComponentPose(Bones, OldPose);

	float GroundZ = OldPose.Position[Ankles[0]].Z;
	for (const int32 Ankle : Ankles)
	{
		GroundZ = FMath::Min(GroundZ, static_cast<float>(OldPose.Position[Ankle].Z));
	}

	const float Head = SolidBody::HeadLengthCm;
	const float CrownZ = GroundZ + SolidBody::ProportionSole * Head;
	auto Station = [CrownZ, Head](const float HeadsDown)
	{
		return CrownZ - HeadsDown * Head;
	};

	int32 ColumnBone = INDEX_NONE;
	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (IsStructuralBone(Bones[Index]) && Bones[Index].Role == ESolidBodyBoneRole::Pelvis)
		{
			ColumnBone = Index;
			break;
		}
	}
	const float ColumnX = ColumnBone != INDEX_NONE ? static_cast<float>(OldPose.Position[ColumnBone].X) : 0.f;
	const float ColumnY = ColumnBone != INDEX_NONE ? static_cast<float>(OldPose.Position[ColumnBone].Y) : 0.f;

	TArray<uint8> bSetZ;
	TArray<uint8> bSnapXY;
	TArray<uint8> bColumn;
	TArray<float> TargetZ;
	bSetZ.Init(0, Bones.Num());
	bSnapXY.Init(0, Bones.Num());
	bColumn.Init(0, Bones.Num());
	TargetZ.Init(0.f, Bones.Num());

	auto SetZ = [&bSetZ, &TargetZ](const int32 Index, const float Z)
	{
		bSetZ[Index] = 1;
		TargetZ[Index] = Z;
	};

	bool bChinOnNeck = false;
	for (const int32 HeadIndex : Heads)
	{
		const int32 Parent = StructuralParentIndex(Bones, HeadIndex);
		if (Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Neck)
		{
			bChinOnNeck = true;
			break;
		}
	}
	for (const int32 HeadIndex : Heads)
	{
		const int32 Parent = StructuralParentIndex(Bones, HeadIndex);
		const bool bOnNeck = Bones.IsValidIndex(Parent) && Bones[Parent].Role == ESolidBodyBoneRole::Neck;
		if (bChinOnNeck && !bOnNeck)
		{
			continue;
		}
		SetZ(HeadIndex, Station(SolidBody::ProportionChin));
		if (ColumnBone != INDEX_NONE)
		{
			bColumn[HeadIndex] = 1;
		}
		else
		{
			bSnapXY[HeadIndex] = 1;
		}
	}

	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		if (!IsStructuralBone(Bones[Index]) || Bones[Index].Role == ESolidBodyBoneRole::Head)
		{
			continue;
		}
		const int32 Parent = StructuralParentIndex(Bones, Index);
		const ESolidBodyBoneRole ParentRole = Bones.IsValidIndex(Parent)
			? Bones[Parent].Role
			: ESolidBodyBoneRole::Root;
		switch (Bones[Index].Role)
		{
		case ESolidBodyBoneRole::Neck:
			if (ParentRole != ESolidBodyBoneRole::Neck)
			{
				SetZ(Index, Station(SolidBody::ProportionNeck));
			}
			if (ColumnBone != INDEX_NONE)
			{
				bColumn[Index] = 1;
			}
			break;
		case ESolidBodyBoneRole::Spine:
			if (ColumnBone != INDEX_NONE)
			{
				bColumn[Index] = 1;
			}
			break;
		case ESolidBodyBoneRole::Clavicle:
			SetZ(Index, Station(SolidBody::ProportionNeck));
			break;
		case ESolidBodyBoneRole::Pelvis:
			SetZ(Index, Station(SolidBody::ProportionPubic));
			break;
		case ESolidBodyBoneRole::Leg:
			if (ParentRole == ESolidBodyBoneRole::Pelvis)
			{
				SetZ(Index, Station(SolidBody::ProportionPubic));
			}
			break;
		case ESolidBodyBoneRole::Foot:
			if (ParentRole == ESolidBodyBoneRole::Leg)
			{
				SetZ(Index, Station(SolidBody::ProportionKnee));
			}
			else if (ParentRole == ESolidBodyBoneRole::Foot)
			{
				SetZ(Index, Station(SolidBody::ProportionSole));
			}
			break;
		default:
			break;
		}
	}

	TArray<int32> SpineChain;
	CollectSpineChain(Bones, SpineChain);
	if (SpineChain.Num() == 1)
	{
		SetZ(SpineChain[0], Station(SolidBody::ProportionChest));
	}
	else if (SpineChain.Num() >= 2)
	{
		const float WaistZ = Station(SolidBody::ProportionWaist);
		const float ChestZ = Station(SolidBody::ProportionChest);
		const int32 Last = SpineChain.Num() - 1;
		for (int32 Step = 0; Step <= Last; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Last);
			SetZ(SpineChain[Step], FMath::Lerp(WaistZ, ChestZ, T));
		}
	}

	TArray<FVector> NewPos = OldPose.Position;
	TArray<uint8> Ready;
	Ready.Init(0, Bones.Num());
	for (int32 Pass = 0; Pass < Bones.Num(); ++Pass)
	{
		bool bProgress = false;
		for (int32 Index = 0; Index < Bones.Num(); ++Index)
		{
			if (Ready[Index])
			{
				continue;
			}
			const int32 Parent = Bones[Index].ParentIndex;
			const bool bHasParent = Bones.IsValidIndex(Parent) && Parent != Index;
			if (bHasParent && !Ready[Parent])
			{
				continue;
			}

			FVector Pos = OldPose.Position[Index];
			if (bHasParent)
			{
				Pos = NewPos[Parent] + (OldPose.Position[Index] - OldPose.Position[Parent]);
			}
			if (bSetZ[Index])
			{
				Pos.Z = TargetZ[Index];
			}
			if (bColumn[Index])
			{
				Pos.X = ColumnX;
				Pos.Y = ColumnY;
			}
			else if (bSnapXY[Index])
			{
				const int32 SnapTo = StructuralParentIndex(Bones, Index);
				if (Bones.IsValidIndex(SnapTo) && Ready[SnapTo])
				{
					Pos.X = NewPos[SnapTo].X;
					Pos.Y = NewPos[SnapTo].Y;
				}
			}
			NewPos[Index] = Pos;
			Ready[Index] = 1;
			bProgress = true;
		}
		if (!bProgress)
		{
			break;
		}
	}

	for (int32 Index = 0; Index < Bones.Num(); ++Index)
	{
		const int32 Parent = Bones[Index].ParentIndex;
		const bool bHasParent = Bones.IsValidIndex(Parent) && Parent != Index && Ready[Parent];
		if (!bHasParent)
		{
			Bones[Index].LocalTranslation = NewPos[Index];
			continue;
		}
		Bones[Index].LocalTranslation = OldPose.Rotation[Parent].UnrotateVector(NewPos[Index] - NewPos[Parent]);
	}
}

FSolidDeformMesh SolidBodyDeform::Apply(
	const FSolidDeformMesh& Source,
	const FSolidBodyParams& InParams)
{
	const FSolidBodyParams Params = InParams.Clamped();

	FComponentPose OldPose;
	BuildComponentPose(Source.Bones, OldPose);

	FSolidDeformMesh Result;
	Result.Bones.SetNum(Source.Bones.Num());
	for (int32 Index = 0; Index < Source.Bones.Num(); ++Index)
	{
		const FSolidDeformBone& Bone = Source.Bones[Index];
		FSolidDeformBone& OutBone = Result.Bones[Index];
		OutBone.ParentIndex = Bone.ParentIndex;
		OutBone.Role = Bone.Role;
		OutBone.LocalRotation = Bone.LocalRotation;
		OutBone.bTwist = Bone.bTwist;
		OutBone.bDigit = Bone.bDigit;

		const int32 Parent = Bone.ParentIndex;
		const bool bHasParent = IsBone(Source.Bones, Parent) && Parent != Index;
		const FQuat ParentRot = bHasParent ? OldPose.Rotation[Parent] : FQuat::Identity;
		const ESolidBodyBoneRole ParentRole = bHasParent
			? Source.Bones[Parent].Role
			: ESolidBodyBoneRole::Root;
		const FVector Offset = ParentRot.RotateVector(Bone.LocalTranslation);
		const FVector Scaled = ScaleSegment(Bone.Role, ParentRole, Offset, Params);
		OutBone.LocalTranslation = ParentRot.UnrotateVector(Scaled);
	}

	FComponentPose NewPose;
	BuildComponentPose(Result.Bones, NewPose);

	Result.Vertices.SetNum(Source.Vertices.Num());
	for (int32 Index = 0; Index < Source.Vertices.Num(); ++Index)
	{
		const FSolidDeformVertex& Vertex = Source.Vertices[Index];
		FSolidDeformVertex& OutVertex = Result.Vertices[Index];
		OutVertex = Vertex;

		FVector Accum = FVector::ZeroVector;
		float WeightSum = 0.f;
		for (int32 Influence = 0; Influence < FSolidDeformVertex::MaxInfluences; ++Influence)
		{
			const float Weight = Vertex.Weight[Influence];
			const int32 BoneIndex = Vertex.BoneIndex[Influence];
			if (Weight <= 0.f || !IsBone(Source.Bones, BoneIndex))
			{
				continue;
			}

			const FVector OldJoint = OldPose.Position[BoneIndex];
			const FVector NewJoint = NewPose.Position[BoneIndex];
			const FVector Local = Vertex.Position - OldJoint;
			const FVector Axis = SegmentAxis(Source.Bones, OldPose, BoneIndex);
			const FVector NewLocal = DeformLocal(Local, Source.Bones[BoneIndex].Role, Axis, Params);
			Accum += (NewJoint + NewLocal) * Weight;
			WeightSum += Weight;
		}

		if (WeightSum > KINDA_SMALL_NUMBER)
		{
			OutVertex.Position = Accum / WeightSum;
		}
	}

	return Result;
}
