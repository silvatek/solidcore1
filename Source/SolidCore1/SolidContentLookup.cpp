#include "SolidContentLookup.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"

namespace
{
	int32 PackagePreference(const FString& PackageName)
	{
		if (PackageName.StartsWith(TEXT("/Game/Viking")))
		{
			return 0;
		}
		if (PackageName.StartsWith(TEXT("/Game/Fab")))
		{
			return 1;
		}
		return 2;
	}

	UObject* FindNamedAsset(const TCHAR* AssetName, const TArray<FTopLevelAssetPath>& ClassPaths)
	{
		if (!AssetName || !AssetName[0])
		{
			return nullptr;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Add(FName(TEXT("/Game/Viking")));
		Filter.PackagePaths.Add(FName(TEXT("/Game/Fab")));
		Filter.bRecursivePaths = true;
		Filter.ClassPaths.Append(ClassPaths);
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		const FAssetData* Best = nullptr;
		int32 BestRank = TNumericLimits<int32>::Max();
		for (const FAssetData& Asset : Assets)
		{
			if (!Asset.AssetName.ToString().Equals(AssetName, ESearchCase::IgnoreCase))
			{
				continue;
			}
			const int32 Rank = PackagePreference(Asset.PackageName.ToString());
			if (Rank < BestRank)
			{
				BestRank = Rank;
				Best = &Asset;
			}
		}

		if (!Best)
		{
			return nullptr;
		}
		return Best->GetAsset();
	}
}

USkeletalMesh* SolidContentLookup::FindVikingMesh()
{
	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(USkeletalMesh::StaticClass()->GetClassPathName());
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(FindNamedAsset(VikingMeshName, Classes));
	if (Mesh)
	{
		UE_LOG(LogSolid, Warning, TEXT("Viking mesh: %s"), *Mesh->GetPathName());
	}
	else
	{
		UE_LOG(LogSolid, Warning,
			TEXT("Viking mesh SK_Viking not found under /Game/Viking or /Game/Fab."));
	}
	return Mesh;
}

UAnimSequence* SolidContentLookup::FindVikingClip(const TCHAR* AssetName)
{
	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(UAnimSequence::StaticClass()->GetClassPathName());
	return Cast<UAnimSequence>(FindNamedAsset(AssetName, Classes));
}

UAnimSequence* SolidContentLookup::FindVikingIdle()
{
	return FindVikingClip(VikingIdleName);
}

UAnimSequence* SolidContentLookup::FindVikingWalk()
{
	return FindVikingClip(VikingWalkName);
}

UAnimSequence* SolidContentLookup::FindVikingRun()
{
	return FindVikingClip(VikingRunName);
}

UAnimSequence* SolidContentLookup::FindVikingJump()
{
	return FindVikingClip(VikingJumpName);
}
