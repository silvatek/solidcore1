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
