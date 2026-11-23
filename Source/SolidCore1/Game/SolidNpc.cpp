#include "SolidNpc.h"

#include "SolidClipLocomotion.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
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

	TSoftObjectPtr<USkeletalMesh> UnusedMesh;
	if (!SolidClipLocomotion::ApplyMeshAndSingleNodeMode(Body, UnusedMesh, nullptr, TEXT("Npc"), true))
	{
		return;
	}

	const FSoftObjectPath IdlePath(SolidClipLocomotion::DefaultIdlePath);
	TSoftObjectPtr<UAnimSequence> IdleSoft(IdlePath);
	UAnimSequence* Idle = SolidClipLocomotion::LoadClip(IdleSoft, SolidClipLocomotion::DefaultIdlePath);
	TObjectPtr<UAnimSequence> ActiveIdle;
	SolidClipLocomotion::PlayLoopingClip(Body, Idle, ActiveIdle, TEXT("Npc"));
}
