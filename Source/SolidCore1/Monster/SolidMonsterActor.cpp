#include "SolidMonsterActor.h"
#include "SolidMonster.h"
#include "SolidClipLocomotion.h"
#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BoneIndices.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#include "ReferenceSkeleton.h"

namespace SolidMonsterActorPrivate
{
	/** Z of each reference-pose bone in component space. That span is the body, not the asset bounds. */
	void CollectRefPoseZ(const USkeletalMesh* Mesh, TArray<float>& OutZ)
	{
		OutZ.Reset();
		if (!Mesh)
		{
			return;
		}

		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const TArray<FTransform>& LocalPose = Ref.GetRefBonePose();
		const int32 BoneCount = LocalPose.Num();
		if (BoneCount <= 0)
		{
			return;
		}

		TArray<FTransform> ComponentPose;
		ComponentPose.SetNum(BoneCount);
		TArray<FBoneIndexType> Required;
		Required.Reserve(BoneCount);
		for (int32 Bone = 0; Bone < BoneCount; ++Bone)
		{
			Required.Add(static_cast<FBoneIndexType>(Bone));
		}
		Mesh->FillComponentSpaceTransforms(LocalPose, Required, ComponentPose);

		OutZ.Reserve(BoneCount);
		for (const FTransform& Bone : ComponentPose)
		{
			OutZ.Add(Bone.GetLocation().Z);
		}
	}
}

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

	TArray<float> BoneZ;
	SolidMonsterActorPrivate::CollectRefPoseZ(Mesh, BoneZ);
	SolidMonster::FStandingSpan Span = SolidMonster::StandingSpan(BoneZ);
	if (Span.Height <= KINDA_SMALL_NUMBER)
	{
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		Span.BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
		Span.Height = Bounds.BoxExtent.Z * 2.f;
	}

	const float Scale = SolidMonster::UniformScale(Span.Height);
	Body->SetRelativeScale3D(FVector(Scale));
	Body->SetRelativeLocation(FVector(0.f, 0.f, -Span.BottomZ * Scale));
	UE_LOG(LogSolid, Log, TEXT("SolidMonster standing height %.1f cm, scale %.3f"), Span.Height, Scale);
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
