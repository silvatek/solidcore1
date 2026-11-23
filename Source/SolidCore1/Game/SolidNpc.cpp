#include "SolidNpc.h"

#include "SolidClipLocomotion.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/SoftObjectPath.h"

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

	const FSoftObjectPath IdlePath(SolidClipLocomotion::DefaultIdlePath);
	TSoftObjectPtr<UAnimSequence> IdleSoft(IdlePath);
	UAnimSequence* Idle = SolidClipLocomotion::LoadClip(IdleSoft, SolidClipLocomotion::DefaultIdlePath);
	if (Idle)
	{
		RetargetFrom(Mesh->GetSkeleton(), Idle->GetSkeleton());
	}

	TSoftObjectPtr<USkeletalMesh> UnusedMesh;
	if (!SolidClipLocomotion::ApplyMeshAndSingleNodeMode(Body, UnusedMesh, nullptr, TEXT("Npc"), true))
	{
		return;
	}

	TObjectPtr<UAnimSequence> ActiveIdle;
	SolidClipLocomotion::PlayLoopingClip(Body, Idle, ActiveIdle, TEXT("Npc"));
}

void ASolidNpc::RetargetFrom(USkeleton* Target, const USkeleton* Source)
{
	if (!Target || !Source || Target == Source)
	{
		return;
	}

	Target->AddCompatibleSkeleton(Source);
	const int32 BoneCount = Target->GetReferenceSkeleton().GetRawBoneNum();
	for (int32 Index = 0; Index < BoneCount; ++Index)
	{
		Target->SetBoneTranslationRetargetingMode(
			Index,
			EBoneTranslationRetargetingMode::Skeleton,
			false);
	}
}
