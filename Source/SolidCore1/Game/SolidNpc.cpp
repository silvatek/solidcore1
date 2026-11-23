#include "SolidNpc.h"

#include "SolidCore1.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Content/SolidContentLookup.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	const TCHAR* MeshPath = TEXT("/Game/Characters/MH1/exports1.exports1");

#if WITH_EDITOR
	void LogBindPose(USkinnedMeshComponent* BodyMesh, const USkeletalMesh* Figure);
#endif
}

ASolidNpc::ASolidNpc()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(true);
}

void ASolidNpc::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidNpc::BuildVisuals()
{
	if (!Body)
	{
		return;
	}

	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
	if (!Mesh)
	{
		UE_LOG(LogSolid, Warning, TEXT("Npc mesh missing: %s"), MeshPath);
		return;
	}

	Body->SetSkeletalMeshAsset(Mesh);
#if WITH_EDITOR
	LogBindPose(Body, Mesh);
#endif
}

const FSolidNpcRetargetChain ASolidNpcChains[] = {
	{ TEXT("Spine"), TEXT("spine_01"), TEXT("spine_03") },
	{ TEXT("LeftArm"), TEXT("clavicle_l"), TEXT("hand_l") },
	{ TEXT("RightArm"), TEXT("clavicle_r"), TEXT("hand_r") },
	{ TEXT("LeftLeg"), TEXT("thigh_l"), TEXT("foot_l") },
	{ TEXT("RightLeg"), TEXT("thigh_r"), TEXT("foot_r") },
	{ TEXT("Head"), TEXT("neck_01"), TEXT("head") },
};

static_assert(UE_ARRAY_COUNT(ASolidNpcChains) == ASolidNpc::RetargetChainCount, "Retarget chain count");

const FSolidNpcRetargetChain& ASolidNpc::RetargetChain(const int32 Index)
{
	static const FSolidNpcRetargetChain Empty = { TEXT(""), TEXT(""), TEXT("") };
	if (Index < 0 || Index >= RetargetChainCount)
	{
		return Empty;
	}
	return ASolidNpcChains[Index];
}

#if WITH_EDITOR
#include "AnimPose.h"
#include "Retargeter/IKRetargeter.h"
#include "RetargetEditor/IKRetargeterController.h"
#include "Retargeter/IKRetargetProcessor.h"
#include "Retargeter/IKRetargetProfile.h"
#include "Retargeter/RetargetOps/FKChainsOp.h"
#include "Retargeter/RetargetOps/PelvisMotionOp.h"
#include "Rig/IKRigDefinition.h"
#include "RigEditor/IKRigController.h"

namespace
{
	const TCHAR* DrivenChainName = TEXT("LeftLeg");

	void UseLeftLegOnly(UIKRetargeterController& Controller, const int32 ChainOp)
	{
		UIKRetargetFKChainsController* Chains =
			Cast<UIKRetargetFKChainsController>(Controller.GetOpController(ChainOp));
		if (!Chains)
		{
			return;
		}

		FIKRetargetFKChainsOpSettings Settings = Chains->GetSettings();
		for (FRetargetFKChainSettings& Chain : Settings.ChainsToRetarget)
		{
			Chain.EnableFK = Chain.TargetChainName == DrivenChainName;
			Chain.RotationMode = EFKChainRotationMode::MatchScaledChain;
			Chain.TranslationMode = EFKChainTranslationMode::None;
		}
		Chains->SetSettings(Settings);
	}

	void HoldPelvisTranslation(UIKRetargeterController& Controller, const int32 PelvisOp)
	{
		UIKRetargetPelvisMotionController* Pelvis =
			Cast<UIKRetargetPelvisMotionController>(Controller.GetOpController(PelvisOp));
		if (!Pelvis)
		{
			return;
		}

		Pelvis->SetSourcePelvisBone(TEXT("pelvis"));
		Pelvis->SetTargetPelvisBone(TEXT("pelvis"));
		FIKRetargetOpBase* Op = Controller.GetRetargetOpByIndex(PelvisOp);
		FIKRetargetPelvisMotionOpSettings* Settings = Op
			? static_cast<FIKRetargetPelvisMotionOpSettings*>(Op->GetSettings())
			: nullptr;
		if (Settings)
		{
			Settings->RotationAlpha = 0.0;
			Settings->TranslationAlpha = 0.0;
		}
	}

	int32 NamedBone(const TArray<FName>& Names, const TCHAR* Bone)
	{
		return Names.IndexOfByKey(FName(Bone));
	}

	void ComponentReferencePose(const FReferenceSkeleton& Skeleton, TArray<FTransform>& OutPose)
	{
		const TArray<FTransform>& Local = Skeleton.GetRefBonePose();
		OutPose.SetNum(Local.Num());
		for (int32 BoneIndex = 0; BoneIndex < Local.Num(); ++BoneIndex)
		{
			const int32 Parent = Skeleton.GetParentIndex(BoneIndex);
			OutPose[BoneIndex] = Parent == INDEX_NONE ? Local[BoneIndex] : Local[BoneIndex] * OutPose[Parent];
		}
	}

	FVector PoseTranslation(const TArray<FTransform>& Pose, const int32 BoneIndex)
	{
		return Pose.IsValidIndex(BoneIndex) ? Pose[BoneIndex].GetTranslation() : FVector::ZeroVector;
	}

	float RotationDegrees(const FTransform& From, const FTransform& To)
	{
		return FMath::RadiansToDegrees(From.GetRotation().AngularDistance(To.GetRotation()));
	}

	void LogIdlePose(
		UPoseableMeshComponent* BodyMesh,
		const USkeletalMesh* Figure,
		const FRetargetSkeleton& SourceSkeleton,
		const TArray<FTransform>& SourceComponentPose,
		const FRetargetSkeleton& TargetSkeleton,
		const TArray<FTransform>& TargetPose)
	{
		const int32 SourcePelvis = NamedBone(SourceSkeleton.BoneNames, TEXT("pelvis"));
		const int32 OutputPelvis = NamedBone(TargetSkeleton.BoneNames, TEXT("pelvis"));
		TArray<FTransform> ReferencePose;
		int32 ReferencePelvis = INDEX_NONE;
		if (!Figure)
		{
			return;
		}
		ComponentReferencePose(Figure->GetRefSkeleton(), ReferencePose);
		ReferencePelvis = Figure->GetRefSkeleton().FindBoneIndex(TEXT("pelvis"));
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc idle pelvis cm: source %s, reference %s, output %s"),
			*PoseTranslation(SourceComponentPose, SourcePelvis).ToString(),
			*PoseTranslation(ReferencePose, ReferencePelvis).ToString(),
			*PoseTranslation(TargetPose, OutputPelvis).ToString());

		float FarthestCm = 0.f;
		FName FarthestBone = NAME_None;
		const FReferenceSkeleton& ReferenceSkeleton = Figure->GetRefSkeleton();
		for (int32 BoneIndex = 0; BoneIndex < TargetSkeleton.BoneNames.Num() && BoneIndex < TargetPose.Num(); ++BoneIndex)
		{
			const FName BoneName = TargetSkeleton.BoneNames[BoneIndex];
			const int32 ReferenceIndex = ReferenceSkeleton.FindBoneIndex(BoneName);
			if (ReferenceIndex == INDEX_NONE || !ReferencePose.IsValidIndex(ReferenceIndex))
			{
				continue;
			}
			const float MovedCm = FVector::Distance(
				TargetPose[BoneIndex].GetTranslation(),
				ReferencePose[ReferenceIndex].GetTranslation());
			if (MovedCm >= FarthestCm)
			{
				FarthestCm = MovedCm;
				FarthestBone = BoneName;
			}
		}
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc idle farthest bone: %s moved %.1f cm from its reference pose"),
			*FarthestBone.ToString(),
			FarthestCm);

		auto LogLegRotation = [&TargetSkeleton, &TargetPose, &ReferenceSkeleton, &ReferencePose](const TCHAR* Bone)
		{
			const int32 OutputIndex = NamedBone(TargetSkeleton.BoneNames, Bone);
			const int32 ReferenceIndex = ReferenceSkeleton.FindBoneIndex(Bone);
			if (!TargetPose.IsValidIndex(OutputIndex) || !ReferencePose.IsValidIndex(ReferenceIndex))
			{
				return -1.f;
			}
			return RotationDegrees(ReferencePose[ReferenceIndex], TargetPose[OutputIndex]);
		};
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc idle left leg rotation deg: thigh %.1f, foot %.1f"),
			LogLegRotation(TEXT("thigh_l")),
			LogLegRotation(TEXT("foot_l")));

		if (BodyMesh)
		{
			BodyMesh->UpdateBounds();
			UE_LOG(
				LogSolid,
				Warning,
				TEXT("Npc idle bounds extent cm: %s"),
				*BodyMesh->Bounds.GetBox().GetExtent().ToString());
		}
	}

	void LogBindPose(USkinnedMeshComponent* BodyMesh, const USkeletalMesh* Figure)
	{
		if (!BodyMesh || !Figure)
		{
			return;
		}

		const FReferenceSkeleton& ReferenceSkeleton = Figure->GetRefSkeleton();
		TArray<FTransform> ReferencePose;
		ComponentReferencePose(ReferenceSkeleton, ReferencePose);
		const int32 Pelvis = ReferenceSkeleton.FindBoneIndex(TEXT("pelvis"));
		const FVector ReferencePelvis = PoseTranslation(ReferencePose, Pelvis);
		const FTransform PosedPelvis = BodyMesh->GetBoneTransform(TEXT("pelvis"), RTS_Component);
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc bind pelvis cm: reference %s, posed %s"),
			*ReferencePelvis.ToString(),
			*PosedPelvis.GetTranslation().ToString());

		float FarthestCm = 0.f;
		FName FarthestBone = NAME_None;
		for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNum(); ++BoneIndex)
		{
			if (!ReferencePose.IsValidIndex(BoneIndex))
			{
				continue;
			}
			const FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
			const FTransform Posed = BodyMesh->GetBoneTransform(BoneName, RTS_Component);
			const float MovedCm = FVector::Distance(
				Posed.GetTranslation(),
				ReferencePose[BoneIndex].GetTranslation());
			if (MovedCm >= FarthestCm)
			{
				FarthestCm = MovedCm;
				FarthestBone = BoneName;
			}
		}
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc bind farthest bone: %s moved %.1f cm from its reference pose"),
			*FarthestBone.ToString(),
			FarthestCm);

		auto RotationFromReference = [&ReferenceSkeleton, &ReferencePose, BodyMesh](const TCHAR* Bone)
		{
			const int32 ReferenceIndex = ReferenceSkeleton.FindBoneIndex(Bone);
			if (!ReferencePose.IsValidIndex(ReferenceIndex))
			{
				return -1.f;
			}
			const FTransform Posed = BodyMesh->GetBoneTransform(Bone, RTS_Component);
			return RotationDegrees(ReferencePose[ReferenceIndex], Posed);
		};
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc bind left leg rotation deg: thigh %.1f, foot %.1f"),
			RotationFromReference(TEXT("thigh_l")),
			RotationFromReference(TEXT("foot_l")));

		BodyMesh->UpdateBounds();
		UE_LOG(
			LogSolid,
			Warning,
			TEXT("Npc bind bounds extent cm: %s"),
			*BodyMesh->Bounds.GetBox().GetExtent().ToString());
	}
}

UIKRigDefinition* ASolidNpc::MakeRetargetRig(USkeletalMesh* Mesh)
{
	if (!Mesh)
	{
		return nullptr;
	}

	UIKRigDefinition* Rig = NewObject<UIKRigDefinition>(GetTransientPackage());
	UIKRigController* Controller = UIKRigController::GetController(Rig);
	if (!Controller || !Controller->SetSkeletalMesh(Mesh))
	{
		return nullptr;
	}

	Controller->SetRetargetRoot(TEXT("pelvis"));
	for (int32 Index = 0; Index < RetargetChainCount; ++Index)
	{
		const FSolidNpcRetargetChain& Chain = RetargetChain(Index);
		const FName Added = Controller->AddRetargetChain(
			Chain.Name,
			Chain.StartBone,
			Chain.EndBone,
			NAME_None);
		if (Added.IsNone())
		{
			return nullptr;
		}
	}
	return Rig;
}

UIKRetargeter* ASolidNpc::MakeIdleRetargeter(USkeletalMesh* Viking, USkeletalMesh* Figure)
{
	if (!Viking || !Figure)
	{
		return nullptr;
	}

	UIKRigDefinition* SourceRig = MakeRetargetRig(Viking);
	UIKRigDefinition* TargetRig = MakeRetargetRig(Figure);
	UIKRetargeter* Retargeter = NewObject<UIKRetargeter>(GetTransientPackage());
	UIKRetargeterController* Controller = UIKRetargeterController::GetController(Retargeter);
	if (!SourceRig || !TargetRig || !Controller)
	{
		return nullptr;
	}

	Controller->SetIKRig(ERetargetSourceOrTarget::Source, SourceRig);
	Controller->SetIKRig(ERetargetSourceOrTarget::Target, TargetRig);
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Source, Viking);
	Controller->SetPreviewMesh(ERetargetSourceOrTarget::Target, Figure);
	const int32 PelvisOp = Controller->AddRetargetOp(TEXT("/Script/IKRig.IKRetargetPelvisMotionOp"));
	const int32 ChainOp = Controller->AddRetargetOp(TEXT("/Script/IKRig.IKRetargetFKChainsOp"));
	if (PelvisOp == INDEX_NONE || ChainOp == INDEX_NONE)
	{
		return nullptr;
	}

	Controller->AutoMapChains(EAutoMapChainType::Exact, true);
	HoldPelvisTranslation(*Controller, PelvisOp);
	UseLeftLegOnly(*Controller, ChainOp);

	UIKRetargetFKChainsController* Chains =
		Cast<UIKRetargetFKChainsController>(Controller->GetOpController(ChainOp));
	if (!Chains || Chains->GetSettings().ChainsToRetarget.Num() != RetargetChainCount)
	{
		return nullptr;
	}
	return Retargeter;
}

bool ASolidNpc::ApplyRetargetedIdle(UPoseableMeshComponent* BodyMesh, USkeletalMesh* Figure, UAnimSequence* Idle)
{
	USkeletalMesh* Viking = SolidContentLookup::FindVikingMesh();
	UIKRetargeter* Retargeter = MakeIdleRetargeter(Viking, Figure);
	if (!BodyMesh || !Figure || !Idle || !Viking || !Retargeter)
	{
		return false;
	}

	FIKRetargetProcessor Processor;
	FRetargetInitParameters Init;
	Init.SourceSkeletalMesh = Viking;
	Init.TargetSkeletalMesh = Figure;
	Init.RetargeterAsset = Retargeter;
	Processor.Initialize(Init);
	if (!Processor.IsInitialized())
	{
		return false;
	}

	const int32 FrameCount = Idle->GetNumberOfSampledKeys();
	const int32 Frame = FrameCount > 1 ? FrameCount / 2 : 0;
	FAnimPoseEvaluationOptions Options;
	Options.OptionalSkeletalMesh = Viking;
	Options.bExtractRootMotion = false;
	Options.bIncorporateRootMotionIntoPose = true;
	Options.bShouldRetarget = false;
	FAnimPose SourcePose;
	UAnimPoseExtensions::GetAnimPoseAtFrame(Idle, Frame, Options, SourcePose);

	const FRetargetSkeleton& SourceSkeleton = Processor.GetSkeleton(ERetargetSourceOrTarget::Source);
	TArray<FTransform> SourceComponentPose;
	SourceComponentPose.SetNum(SourceSkeleton.BoneNames.Num());
	for (int32 BoneIndex = 0; BoneIndex < SourceSkeleton.BoneNames.Num(); ++BoneIndex)
	{
		SourceComponentPose[BoneIndex] = UAnimPoseExtensions::GetBonePose(
			SourcePose,
			SourceSkeleton.BoneNames[BoneIndex],
			EAnimPoseSpaces::World);
		SourceComponentPose[BoneIndex].SetScale3D(FVector::OneVector);
	}

	FRetargetProfile Profile;
	Profile.FillProfileWithAssetSettings(Retargeter);
	FRetargetRunParameters Params;
	Params.SourceGlobalPose = &SourceComponentPose;
	Params.Profile = &Profile;
	const TArray<FTransform>& TargetPose = Processor.RunRetargeter(Params);

	const FRetargetSkeleton& TargetSkeleton = Processor.GetSkeleton(ERetargetSourceOrTarget::Target);
	const int32 BoneCount = FMath::Min(TargetSkeleton.BoneNames.Num(), TargetPose.Num());
	for (int32 BoneIndex = 0; BoneIndex < BoneCount; ++BoneIndex)
	{
		BodyMesh->SetBoneTransformByName(
			TargetSkeleton.BoneNames[BoneIndex],
			TargetPose[BoneIndex],
			EBoneSpaces::ComponentSpace);
	}
	BodyMesh->RefreshBoneTransforms();
	LogIdlePose(BodyMesh, Figure, SourceSkeleton, SourceComponentPose, TargetSkeleton, TargetPose);
	return BoneCount > 0;
}
#else
UIKRigDefinition* ASolidNpc::MakeRetargetRig(USkeletalMesh*)
{
	return nullptr;
}
#endif
