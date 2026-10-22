#include "SolidRaptor.h"
#include "SolidRaptorActor.h"
#include "SolidClipLocomotion.h"
#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "BoneIndices.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "ReferenceSkeleton.h"
#include "Rendering/SkeletalMeshRenderData.h"

namespace SolidRaptorPrivate
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

	/** Z of each LOD0 vertex. That is the mesh the player sees, not the bone span. */
	void CollectVertexZ(const USkeletalMesh* Mesh, TArray<float>& OutZ)
	{
		OutZ.Reset();
		if (!Mesh)
		{
			return;
		}

		const FSkeletalMeshRenderData* Render = Mesh->GetResourceForRendering();
		if (!Render || Render->LODRenderData.Num() == 0)
		{
			return;
		}

		const FPositionVertexBuffer& Positions = Render->LODRenderData[0].StaticVertexBuffers.PositionVertexBuffer;
		const uint32 Count = Positions.GetNumVertices();
		OutZ.Reserve(Count);
		for (uint32 Index = 0; Index < Count; ++Index)
		{
			OutZ.Add(Positions.VertexPosition(Index).Z);
		}
	}
}

float FSolidRaptor::DrawnBodyFactor() const
{
	switch (Size)
	{
	case ESolidMonsterSize::Small:
		return SmallDrawnBodyFactor;
	default:
		// Later sizes scale from Small. Until those factors exist, draw as Small.
		return SmallDrawnBodyFactor;
	}
}

AActor* FSolidRaptor::Spawn(UWorld* World, const FVector& GroundLocation) const
{
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASolidRaptor* RaptorActor = World->SpawnActor<ASolidRaptor>(
		ASolidRaptor::StaticClass(),
		GroundLocation,
		FRotator(0.f, YawDeg, 0.f),
		Params);
	if (!RaptorActor)
	{
		return nullptr;
	}

	RaptorActor->SetRaptor(*this);
	return RaptorActor;
}

float FSolidRaptor::BodyScale(const float SpanHeightCm) const
{
	return UniformScale(SpanHeightCm) * DrawnBodyFactor();
}

USkeletalMesh* FSolidRaptor::FindMesh() const
{
	return SolidContentLookup::FindVelociraptorMesh();
}

UMaterialInterface* FSolidRaptor::FindGreen(USkeletalMesh* Mesh) const
{
	return SolidContentLookup::FindVelociraptorGreen(Mesh);
}

UAnimSequence* FSolidRaptor::FindIdle(USkeletalMesh* Mesh) const
{
	return SolidContentLookup::FindVelociraptorIdle(Mesh);
}

void FSolidRaptor::ApplyVisuals(USkeletalMeshComponent* Body, TObjectPtr<UAnimSequence>& ActiveIdle) const
{
	if (!Body)
	{
		return;
	}

	USkeletalMesh* Mesh = FindMesh();
	if (!Mesh)
	{
		UE_LOG(LogSolid, Error, TEXT("SolidRaptor: Velociraptor mesh missing."));
		return;
	}

	Body->SetSkeletalMeshAsset(Mesh);
	TSoftObjectPtr<USkeletalMesh> UnusedMesh;
	SolidClipLocomotion::ApplyMeshAndSingleNodeMode(Body, UnusedMesh, nullptr, TEXT("Monster"), true);

	TArray<float> Samples;
	SolidRaptorPrivate::CollectVertexZ(Mesh, Samples);
	const bool bFromMesh = Samples.Num() > 0;
	if (!bFromMesh)
	{
		SolidRaptorPrivate::CollectRefPoseZ(Mesh, Samples);
	}
	FSolidMonsterStandingSpan Span = StandingSpan(Samples);
	if (Span.Height <= KINDA_SMALL_NUMBER)
	{
		const FBoxSphereBounds Bounds = Mesh->GetBounds();
		Span.BottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
		Span.Height = Bounds.BoxExtent.Z * 2.f;
	}

	const float Scale = BodyScale(Span.Height);
	const float FootZ = Samples.Num() > 0 ? Samples[0] : Span.BottomZ;
	const float LocalZ = -FootZ * Scale;
	Body->SetRelativeScale3D(FVector(Scale));
	Body->SetRelativeLocation(FVector(0.f, 0.f, LocalZ));
	const float FullHeight = Samples.Num() > 0 ? Samples.Last() - Samples[0] : 0.f;
	UE_LOG(LogSolid, Log, TEXT("SolidRaptor %s samples %d, full %.1f cm, chosen %.1f cm, foot %.1f cm, localZ %+.1f, scale %.3f"),
		bFromMesh ? TEXT("mesh") : TEXT("bones"),
		Samples.Num(),
		FullHeight,
		Span.Height,
		FootZ,
		LocalZ,
		Scale);
	Body->SetRelativeRotation(FRotator(0.f, MeshFacingYawDeg, 0.f));

	if (UMaterialInterface* Green = FindGreen(Mesh))
	{
		const int32 Slots = Body->GetNumMaterials();
		for (int32 Slot = 0; Slot < Slots; ++Slot)
		{
			Body->SetMaterial(Slot, Green);
		}
	}

	UAnimSequence* Idle = FindIdle(Mesh);
	SolidClipLocomotion::PlayLoopingClip(Body, Idle, ActiveIdle, TEXT("Monster"));
}

TUniquePtr<FSolidMonster> FSolidRaptor::Clone() const
{
	return MakeUnique<FSolidRaptor>(*this);
}
