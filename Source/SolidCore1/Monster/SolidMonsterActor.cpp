#include "SolidMonsterActor.h"
#include "SolidMonster.h"
#include "SolidClipLocomotion.h"
#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

ASolidMonster::ASolidMonster()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));
	Capsule->SetupAttachment(SceneRoot);
	Capsule->InitCapsuleSize(SolidMonster::CapsuleRadiusCm, SolidMonster::CapsuleHalfHeightCm);
	Capsule->SetRelativeLocation(FVector(0.f, 0.f, SolidMonster::CapsuleHalfHeightCm));
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Capsule->SetCollisionObjectType(ECC_WorldDynamic);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Capsule->SetGenerateOverlapEvents(false);

	Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(SceneRoot);
	Body->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Body->SetRelativeRotation(FRotator(0.f, SolidMonster::MeshFacingYawDeg, 0.f));
}

void ASolidMonster::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	BuildVisuals();
}

void ASolidMonster::BeginPlay()
{
	Super::BeginPlay();
	BuildVisuals();
}

void ASolidMonster::SetChoice(const int32 InChoiceIndex)
{
	ChoiceIndex = InChoiceIndex;
}

void ASolidMonster::BuildVisuals()
{
	if (!Body)
	{
		return;
	}

	USkeletalMesh* Mesh = SolidContentLookup::FindVelociraptorMesh();
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidMonster: Velociraptor mesh missing."));
		return;
	}

	Body->SetSkeletalMeshAsset(Mesh);
	TSoftObjectPtr<USkeletalMesh> UnusedMesh;
	SolidClipLocomotion::ApplyMeshAndSingleNodeMode(Body, UnusedMesh, nullptr, TEXT("Monster"), true);

	const FBoxSphereBounds Bounds = Mesh->GetBounds();
	const float Scale = SolidMonster::UniformScale(Bounds.BoxExtent.Z * 2.f);
	Body->SetRelativeScale3D(FVector(Scale));
	Body->SetRelativeLocation(FVector(
		0.f,
		0.f,
		SolidMonster::MeshFootOffsetCm(Bounds.Origin.Z, Bounds.BoxExtent.Z, Scale)));
	Body->SetRelativeRotation(FRotator(0.f, SolidMonster::MeshFacingYawDeg, 0.f));

	if (UMaterialInterface* Green = SolidContentLookup::FindVelociraptorGreen(Mesh))
	{
		const int32 Slots = Body->GetNumMaterials();
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			Body->SetMaterial(Slot, Green);
		}
	}

	UAnimSequence* Idle = SolidContentLookup::FindVelociraptorIdle(Mesh);
	SolidClipLocomotion::PlayLoopingClip(Body, Idle, ActiveIdle, TEXT("Monster"));
}
