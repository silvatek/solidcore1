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
}

void SolidBodyDeform::ComponentPositions(
	const TArray<FSolidDeformBone>& AdjustedBones,
	TArray<FVector>& OutJointPositions)
{
	FComponentPose Pose;
	BuildComponentPose(AdjustedBones, Pose);
	OutJointPositions = MoveTemp(Pose.Position);
}
