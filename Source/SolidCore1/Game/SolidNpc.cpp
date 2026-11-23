#include "SolidNpc.h"

#include "SolidCore1.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	const TCHAR* MeshPath = TEXT("/Game/Characters/MH1/exports1.exports1");
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
}

const FSolidNpcRetargetChain ASolidNpcChains[] = {
	{ TEXT("Spine"), TEXT("pelvis"), TEXT("spine_03") },
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
#include "Rig/IKRigDefinition.h"
#include "RigEditor/IKRigController.h"

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
#else
UIKRigDefinition* ASolidNpc::MakeRetargetRig(USkeletalMesh*)
{
	return nullptr;
}
#endif
