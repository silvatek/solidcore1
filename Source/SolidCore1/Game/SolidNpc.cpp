#include "SolidNpc.h"

#include "SolidCore1.h"
#include "Components/PoseableMeshComponent.h"
#include "Content/SolidContentLookup.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	const TCHAR* MeshPath = TEXT("/Game/Characters/MH1/exports1.exports1");
}

ASolidNpc::ASolidNpc()
{
	PrimaryActorTick.bCanEverTick = false;

	Body = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("Body"));
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

	Body->SetSkinnedAssetAndUpdate(Mesh, true);
#if WITH_EDITOR
	UAnimSequence* Idle = SolidContentLookup::FindVikingIdle();
	if (!ApplyRetargetedIdle(Body, Mesh, Idle))
	{
		UE_LOG(LogSolid, Warning, TEXT("Npc idle was not retargeted."));
	}
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
	UE_LOG(
		LogSolid,
		Warning,
		TEXT("Npc idle retarget: left leg only, match scaled chain, bones %d"),
		BoneCount);
	return BoneCount > 0;
}
#else
UIKRigDefinition* ASolidNpc::MakeRetargetRig(USkeletalMesh*)
{
	return nullptr;
}
#endif
