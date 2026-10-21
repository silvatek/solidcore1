#include "SolidContentLookup.h"
#include "SolidFabManifest.h"
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
	const FString Manifest = SolidFabManifest::LoadManifest();
	FString RecordedFile;
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("viking"), TEXT("SK_Viking.uasset"), RecordedFile);
	const bool bFile = SolidFabManifest::ContentFileExists(RecordedFile);
	if (SolidFabManifest::ChooseFabLookup(bFile, false) == ESolidFabLookup::RecordedFile)
	{
		if (USkeletalMesh* Recorded = Cast<USkeletalMesh>(
			SolidFabManifest::TryLoadContentFile(RecordedFile, USkeletalMesh::StaticClass())))
		{
			UE_LOG(LogSolid, Warning, TEXT("Viking mesh recorded: %s"), *Recorded->GetPathName());
			return Recorded;
		}
		UE_LOG(LogSolid, Warning, TEXT("Recorded Viking mesh is missing. Searching."));
	}

	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(USkeletalMesh::StaticClass()->GetClassPathName());
	USkeletalMesh* Mesh = Cast<USkeletalMesh>(FindNamedAsset(VikingMeshName, Classes));
	if (Mesh)
	{
		UE_LOG(LogSolid, Warning, TEXT("Viking mesh: %s"), *Mesh->GetPathName());
		SolidFabManifest::RememberNamedFile(TEXT("viking"), TEXT("SK_Viking.uasset"), Mesh);
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

	const FString Field = FString(AssetName) + TEXT(".uasset");
	const FString Manifest = SolidFabManifest::LoadManifest();
	FString RecordedFile;
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("viking"), *Field, RecordedFile);
	const bool bFile = SolidFabManifest::ContentFileExists(RecordedFile);
	if (SolidFabManifest::ChooseFabLookup(bFile, false) == ESolidFabLookup::RecordedFile)
	{
		if (UAnimSequence* Recorded = Cast<UAnimSequence>(
			SolidFabManifest::TryLoadContentFile(RecordedFile, UAnimSequence::StaticClass())))
		{
			return Recorded;
		}
	}

	UAnimSequence* Clip = Cast<UAnimSequence>(FindNamedAsset(AssetName, Classes));
	if (Clip)
	{
		SolidFabManifest::RememberNamedFile(TEXT("viking"), *Field, Clip);
	}
	return Clip;
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
