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

	const TCHAR* FallbackAssetId(const TCHAR* Token)
	{
		if (!Token)
		{
			return TEXT("");
		}
		if (FCString::Stricmp(Token, TEXT("Assassin")) == 0)
		{
			return TEXT("assassin");
		}
		if (FCString::Stricmp(Token, TEXT("Blacksmith")) == 0)
		{
			return TEXT("blacksmith");
		}
		return TEXT("");
	}

	USkeletalMesh* FindBestSkeletalMesh(
		const TArray<FName>& PackagePaths,
		const TCHAR* Token,
		const TCHAR* RequiredFolder,
		const TArray<FTopLevelAssetPath>& ClassPaths)
	{
		if (PackagePaths.Num() == 0)
		{
			return nullptr;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Append(PackagePaths);
		Filter.bRecursivePaths = true;
		Filter.ClassPaths = ClassPaths;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		const FAssetData* Best = nullptr;
		for (const FAssetData& Asset : Assets)
		{
			if (RequiredFolder && RequiredFolder[0]
				&& !SolidContentLookup::PackageContainsFolder(Asset.PackageName.ToString(), RequiredFolder))
			{
				continue;
			}
			const FString AssetName = Asset.AssetName.ToString();
			if (!Best
				|| SolidContentLookup::IsBetterCompanionMesh(AssetName, Best->AssetName.ToString(), Token))
			{
				Best = &Asset;
			}
		}

		return Best ? Cast<USkeletalMesh>(Best->GetAsset()) : nullptr;
	}

	USkeletalMesh* FindCompanionBody(
		const TCHAR* PackFolder,
		const TArray<FName>& PackagePaths,
		const TArray<FTopLevelAssetPath>& ClassPaths)
	{
		if (!PackFolder || !PackFolder[0] || PackagePaths.Num() == 0)
		{
			return nullptr;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Append(PackagePaths);
		Filter.bRecursivePaths = true;
		Filter.ClassPaths = ClassPaths;
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		const FAssetData* Best = nullptr;
		for (const FAssetData& Asset : Assets)
		{
			if (!Asset.AssetName.ToString().Equals(TEXT("SK_Mannequin"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			const FString PackageName = Asset.PackageName.ToString();
			if (!SolidContentLookup::PackageContainsFolder(PackageName, PackFolder)
				|| !SolidContentLookup::PackageContainsFolder(PackageName, TEXT("Meshes"))
				|| !SolidContentLookup::PackageContainsFolder(PackageName, TEXT("base_mesh")))
			{
				continue;
			}
			if (!Best || PackageName.Len() < Best->PackageName.ToString().Len())
			{
				Best = &Asset;
			}
		}

		return Best ? Cast<USkeletalMesh>(Best->GetAsset()) : nullptr;
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

USkeletalMesh* SolidContentLookup::FindSkeletalMeshByToken(const TCHAR* Token)
{
	if (!Token || !Token[0])
	{
		return nullptr;
	}

	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	const FString Manifest = SolidFabManifest::LoadManifest();
	FString AssetId = SolidFabManifest::AssetIdForToken(Manifest, Token);
	if (AssetId.IsEmpty())
	{
		AssetId = FallbackAssetId(Token);
	}

	FString RecordedFile;
	FString RecordedFolder;
	if (!AssetId.IsEmpty())
	{
		SolidFabManifest::ReadJsonStringField(Manifest, *AssetId, TEXT("resolvedPath"), RecordedFile);
		SolidFabManifest::ReadJsonStringField(Manifest, *AssetId, TEXT("resolvedFolder"), RecordedFolder);
	}
	const bool bFile = SolidFabManifest::ContentFileExists(RecordedFile);
	const bool bFolder = SolidFabManifest::ContentDirectoryExists(RecordedFolder);
	const TCHAR* PackFolder = CompanionMeshFolder(Token);

	auto Remember = [&AssetId, PackFolder](USkeletalMesh* Mesh) -> USkeletalMesh*
	{
		if (Mesh && !AssetId.IsEmpty())
		{
			SolidFabManifest::RememberDiscoveredMesh(*AssetId, PackFolder, Mesh);
		}
		return Mesh;
	};

	if (bFile && IsCompanionBodyPath(RecordedFile))
	{
		if (USkeletalMesh* Recorded = Cast<USkeletalMesh>(
			SolidFabManifest::TryLoadContentFile(RecordedFile, USkeletalMesh::StaticClass())))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s body: %s"), Token, *Recorded->GetPathName());
			return Recorded;
		}
		UE_LOG(LogSolid, Warning, TEXT("Recorded body for token %s is missing. Searching."), Token);
	}

	if (bFolder)
	{
		const FString BodyFile = CompanionBodyFile(RecordedFolder);
		if (USkeletalMesh* Body = Cast<USkeletalMesh>(
			SolidFabManifest::TryLoadContentFile(BodyFile, USkeletalMesh::StaticClass())))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s body in %s -> %s"),
				Token, *RecordedFolder, *Body->GetPathName());
			return Remember(Body);
		}
	}

	if (PackFolder && PackFolder[0])
	{
		TArray<FName> Packages;
		Packages.Add(FName(TEXT("/Game")));
		if (USkeletalMesh* Body = FindCompanionBody(PackFolder, Packages, Classes))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s body -> %s"), Token, *Body->GetPathName());
			return Remember(Body);
		}
	}

	if (bFile)
	{
		if (USkeletalMesh* Recorded = Cast<USkeletalMesh>(
			SolidFabManifest::TryLoadContentFile(RecordedFile, USkeletalMesh::StaticClass())))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s recorded: %s"), Token, *Recorded->GetPathName());
			return Recorded;
		}
		UE_LOG(LogSolid, Warning, TEXT("Recorded mesh for token %s is missing. Searching."), Token);
	}

	if (bFolder)
	{
		const FString Package = SolidFabManifest::ContentFolderToPackagePath(RecordedFolder);
		TArray<FName> Packages;
		if (!Package.IsEmpty())
		{
			Packages.Add(FName(*Package));
		}
		if (USkeletalMesh* InFolder = FindBestSkeletalMesh(Packages, Token, nullptr, Classes))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s in recorded folder %s -> %s"),
				Token, *RecordedFolder, *InFolder->GetPathName());
			return Remember(InFolder);
		}
		UE_LOG(LogSolid, Warning, TEXT("No skeletal mesh in recorded folder %s. Searching."), *RecordedFolder);
	}

	if (PackFolder && PackFolder[0])
	{
		TArray<FName> Packages;
		Packages.Add(FName(TEXT("/Game")));
		if (USkeletalMesh* InFolder = FindBestSkeletalMesh(Packages, Token, PackFolder, Classes))
		{
			UE_LOG(LogSolid, Warning, TEXT("Mesh token %s in %s -> %s"),
				Token, PackFolder, *InFolder->GetPathName());
			return Remember(InFolder);
		}
		UE_LOG(LogSolid, Warning,
			TEXT("No skeletal mesh in Content folder %s for token \"%s\"."),
			PackFolder, Token);
	}

	if (USkeletalMesh* Exact = Cast<USkeletalMesh>(FindNamedAsset(Token, Classes)))
	{
		return Remember(Exact);
	}

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Viking")));
	Filter.PackagePaths.Add(FName(TEXT("/Game/Fab")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths = Classes;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistry.GetAssets(Filter, Assets);

	const FAssetData* Best = nullptr;
	for (const FAssetData& Asset : Assets)
	{
		const FString AssetName = Asset.AssetName.ToString();
		if (!AssetName.Contains(Token, ESearchCase::IgnoreCase))
		{
			continue;
		}
		if (!Best)
		{
			Best = &Asset;
			continue;
		}
		const bool bBestSk = Best->AssetName.ToString().StartsWith(TEXT("SK_"));
		const bool bThisSk = AssetName.StartsWith(TEXT("SK_"));
		if (bThisSk != bBestSk)
		{
			if (bThisSk)
			{
				Best = &Asset;
			}
			continue;
		}
		if (AssetName.Len() < Best->AssetName.ToString().Len())
		{
			Best = &Asset;
		}
	}

	if (!Best)
	{
		UE_LOG(LogSolid, Warning,
			TEXT("No skeletal mesh under /Game/Viking or /Game/Fab contains \"%s\"."),
			Token);
		return nullptr;
	}

	UE_LOG(LogSolid, Warning, TEXT("Mesh token %s -> %s"), Token, *Best->GetObjectPathString());
	return Remember(Cast<USkeletalMesh>(Best->GetAsset()));
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
