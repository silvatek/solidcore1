#include "SolidClipLocomotion.h"
#include "SolidContentLookup.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "SolidCore1.h"
#include "UObject/SoftObjectPath.h"

UAnimSequence* SolidClipLocomotion::LoadClip(TSoftObjectPtr<UAnimSequence>& Soft, const TCHAR* FallbackPath)
{
	if (UAnimSequence* Loaded = Soft.LoadSynchronous())
	{
		return Loaded;
	}
	if (FallbackPath)
	{
		if (UAnimSequence* FromPath = Cast<UAnimSequence>(
			StaticLoadObject(UAnimSequence::StaticClass(), nullptr, FallbackPath)))
		{
			return FromPath;
		}
	}
	// Launcher Add to Project may have dropped the pack under /Game/Fab/<listing>/.
	if (FallbackPath && FCString::Stricmp(FallbackPath, DefaultIdlePath) == 0)
	{
		return SolidContentLookup::FindVikingIdle();
	}
	if (FallbackPath && FCString::Stricmp(FallbackPath, DefaultWalkPath) == 0)
	{
		return SolidContentLookup::FindVikingWalk();
	}
	if (FallbackPath && FCString::Stricmp(FallbackPath, DefaultRunPath) == 0)
	{
		return SolidContentLookup::FindVikingRun();
	}
	if (FallbackPath && FCString::Stricmp(FallbackPath, DefaultJumpPath) == 0)
	{
		return SolidContentLookup::FindVikingJump();
	}
	return nullptr;
}

USkeletalMesh* SolidClipLocomotion::LoadMesh(TSoftObjectPtr<USkeletalMesh>& Soft, const TCHAR* FallbackPath)
{
	if (USkeletalMesh* Loaded = Soft.LoadSynchronous())
	{
		return Loaded;
	}
	if (FallbackPath)
	{
		if (USkeletalMesh* FromPath = Cast<USkeletalMesh>(
			StaticLoadObject(USkeletalMesh::StaticClass(), nullptr, FallbackPath)))
		{
			return FromPath;
		}
	}
	if (!FallbackPath || FCString::Stricmp(FallbackPath, DefaultMeshPath) == 0)
	{
		return SolidContentLookup::FindVikingMesh();
	}
	return nullptr;
}

bool SolidClipLocomotion::ApplyMeshAndSingleNodeMode(
	USkeletalMeshComponent* MeshComp,
	TSoftObjectPtr<USkeletalMesh>& SoftMesh,
	const TCHAR* FallbackMeshPath,
	const TCHAR* DebugLabel,
	bool bOnlyIfMeshUnset)
{
	if (!MeshComp)
	{
		return false;
	}

	const bool bNeedsMesh =
		!bOnlyIfMeshUnset
		|| MeshComp->GetSkeletalMeshAsset() == nullptr;

	if (bNeedsMesh)
	{
		USkeletalMesh* LoadedMesh = LoadMesh(SoftMesh, FallbackMeshPath);
		if (!LoadedMesh)
		{
			UE_LOG(LogSolid, Error,
				TEXT("%s: skeletal mesh missing (%s)."),
				DebugLabel ? DebugLabel : TEXT("ClipLocomotion"),
				FallbackMeshPath ? FallbackMeshPath : TEXT("<no fallback>"));
			return false;
		}

		if (MeshComp->GetSkeletalMeshAsset() != LoadedMesh)
		{
			MeshComp->SetSkeletalMeshAsset(LoadedMesh);
			UE_LOG(LogSolid, Warning,
				TEXT("%s mesh: %s"),
				DebugLabel ? DebugLabel : TEXT("ClipLocomotion"),
				*LoadedMesh->GetPathName());
		}
	}

	if (!MeshComp->GetSkeletalMeshAsset())
	{
		return false;
	}

	MeshComp->SetVisibility(true);
	MeshComp->SetHiddenInGame(false);
	MeshComp->SetCastShadow(true);

	// Custom skeletons — single-node clip playback (not Epic AnimBP).
	MeshComp->SetAnimInstanceClass(nullptr);
	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	MeshComp->bPauseAnims = false;
	MeshComp->bNoSkeletonUpdate = false;
	MeshComp->InitAnim(true);
	return true;
}

bool SolidClipLocomotion::PlayLoopingClip(
	USkeletalMeshComponent* MeshComp,
	UAnimSequence* Anim,
	TObjectPtr<UAnimSequence>& ActiveAnim,
	const TCHAR* DebugLabel)
{
	if (!MeshComp || !Anim)
	{
		return false;
	}

	MeshComp->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	MeshComp->InitAnim(true);

	if (UAnimSingleNodeInstance* SingleNode = MeshComp->GetSingleNodeInstance())
	{
		if (SingleNode->GetAnimationAsset() != Anim || !SingleNode->IsPlaying())
		{
			SingleNode->SetAnimationAsset(Anim, false);
			SingleNode->SetLooping(true);
			SingleNode->SetPlaying(true);
			SingleNode->SetPlayRate(1.f);
		}
		ActiveAnim = Anim;
		return true;
	}

	// Fallback path used by some engine versions.
	MeshComp->PlayAnimation(Anim, true);
	if (UAnimSingleNodeInstance* SingleNode = MeshComp->GetSingleNodeInstance())
	{
		ActiveAnim = Anim;
		return SingleNode->GetAnimationAsset() == Anim;
	}

	UE_LOG(LogSolid, Error,
		TEXT("%s: failed AnimSingleNodeInstance for %s"),
		DebugLabel ? DebugLabel : TEXT("ClipLocomotion"),
		*Anim->GetName());
	return false;
}

bool SolidClipLocomotion::IsPlayingClip(
	const USkeletalMeshComponent* MeshComp,
	const UAnimSequence* ActiveAnim,
	const UAnimSequence* Desired)
{
	if (!MeshComp || !Desired || ActiveAnim != Desired)
	{
		return false;
	}

	const UAnimSingleNodeInstance* SingleNode = MeshComp->GetSingleNodeInstance();
	return SingleNode
		&& SingleNode->GetAnimationAsset() == Desired
		&& SingleNode->IsPlaying();
}
