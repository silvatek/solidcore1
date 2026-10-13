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
}

void SolidBodyDeform::ComponentPositions(
	const TArray<FSolidDeformBone>& Bones,
	TArray<FVector>& OutPositions)
{
	FComponentPose Pose;
	BuildComponentPose(Bones, Pose);
	OutPositions = MoveTemp(Pose.Position);
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
