#include "SolidContentLookup.h"
#include "SolidFabManifest.h"
#include "SolidCore1.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

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

	bool ContainsWord(const FString& Name, const TCHAR* Word)
	{
		return Word && Word[0] && Name.Contains(Word, ESearchCase::IgnoreCase);
	}

	bool IsVelociraptorMeshName(const FString& Name)
	{
		if (!ContainsWord(Name, TEXT("Velociraptor")))
		{
			return false;
		}
		return !ContainsWord(Name, TEXT("Skeleton"))
			&& !ContainsWord(Name, TEXT("Physics"))
			&& !ContainsWord(Name, TEXT("Morph"));
	}

	bool IsVelociraptorIdleName(const FString& Name)
	{
		if (!ContainsWord(Name, TEXT("Idle")))
		{
			return false;
		}
		const TCHAR* Banned[] = {
			TEXT("Root"), TEXT("Attack"), TEXT("Death"), TEXT("Hit"),
			TEXT("Roar"), TEXT("Walk"), TEXT("Run"),
		};
		for (const TCHAR* Word : Banned)
		{
			if (ContainsWord(Name, Word))
			{
				return false;
			}
		}
		return true;
	}

	bool IsGreenerName(const FString& Name)
	{
		return ContainsWord(Name, TEXT("Green")) && !ContainsWord(Name, TEXT("Physics"));
	}

	/** SK_ wins. Otherwise the shorter name wins, then the earlier path. */
	bool PreferAssetName(const FString& Name, const FString& Path, const FString& BestName, const FString& BestPath)
	{
		const bool bSk = Name.StartsWith(TEXT("SK_"), ESearchCase::IgnoreCase);
		const bool bBestSk = BestName.StartsWith(TEXT("SK_"), ESearchCase::IgnoreCase);
		if (bSk != bBestSk)
		{
			return bSk;
		}
		if (Name.Len() != BestName.Len())
		{
			return Name.Len() < BestName.Len();
		}
		return Path < BestPath;
	}

	/** The shorter name wins, then the earlier path. */
	bool PreferShorterName(const FString& Name, const FString& Path, const FString& BestName, const FString& BestPath)
	{
		if (Name.Len() != BestName.Len())
		{
			return Name.Len() < BestName.Len();
		}
		return Path < BestPath;
	}

	void GatherAssets(
		const TArray<FName>& PackagePaths,
		const TArray<FTopLevelAssetPath>& ClassPaths,
		const bool bRecursive,
		TArray<FAssetData>& OutAssets)
	{
		OutAssets.Reset();
		if (PackagePaths.Num() == 0)
		{
			return;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Append(PackagePaths);
		Filter.bRecursivePaths = bRecursive;
		Filter.ClassPaths.Append(ClassPaths);
		Filter.bRecursiveClasses = true;
		AssetRegistry.GetAssets(Filter, OutAssets);
	}

	FString PackageFolder(const UObject* Object)
	{
		if (!Object)
		{
			return FString();
		}
		const FString Package = Object->GetOutermost()->GetName();
		int32 Slash = INDEX_NONE;
		if (!Package.FindLastChar(TEXT('/'), Slash) || Slash <= 0)
		{
			return FString();
		}
		return Package.Left(Slash);
	}

	FString ParentPackage(const FString& Package)
	{
		int32 Slash = INDEX_NONE;
		if (!Package.FindLastChar(TEXT('/'), Slash) || Slash <= 0)
		{
			return FString();
		}
		return Package.Left(Slash);
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

USkeletalMesh* SolidContentLookup::FindVelociraptorMesh()
{
	const FString Manifest = SolidFabManifest::LoadManifest();
	TArray<FString> Recorded;
	SolidFabManifest::CollectContentFiles(Manifest, TEXT("velociraptor"), Recorded);

	USkeletalMesh* BestRecorded = nullptr;
	FString BestName;
	FString BestPath;
	for (const FString& File : Recorded)
	{
		USkeletalMesh* Mesh = Cast<USkeletalMesh>(
			SolidFabManifest::TryLoadContentFile(File, USkeletalMesh::StaticClass()));
		if (!Mesh || !IsVelociraptorMeshName(Mesh->GetName()))
		{
			continue;
		}
		const FString Path = Mesh->GetPathName();
		if (!BestRecorded || PreferAssetName(Mesh->GetName(), Path, BestName, BestPath))
		{
			BestRecorded = Mesh;
			BestName = Mesh->GetName();
			BestPath = Path;
		}
	}
	if (BestRecorded)
	{
		UE_LOG(LogSolid, Warning, TEXT("Velociraptor mesh recorded: %s"), *BestRecorded->GetPathName());
		return BestRecorded;
	}

	auto Accept = [](USkeletalMesh* Mesh) -> USkeletalMesh*
	{
		if (Mesh)
		{
			SolidFabManifest::RememberResolvedPath(TEXT("velociraptor"), Mesh);
			UE_LOG(LogSolid, Warning, TEXT("Velociraptor mesh: %s"), *Mesh->GetPathName());
		}
		return Mesh;
	};

	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(USkeletalMesh::StaticClass()->GetClassPathName());

	FString RecordedFolder;
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("velociraptor"), TEXT("resolvedFolder"), RecordedFolder);
	if (SolidFabManifest::ContentDirectoryExists(RecordedFolder))
	{
		const FString Package = SolidFabManifest::ContentFolderToPackagePath(RecordedFolder);
		TArray<FName> Packages;
		if (!Package.IsEmpty())
		{
			Packages.Add(FName(*Package));
		}
		TArray<FAssetData> Assets;
		GatherAssets(Packages, Classes, true, Assets);
		const FAssetData* Best = nullptr;
		FString FolderBestName;
		FString FolderBestPath;
		for (const FAssetData& Asset : Assets)
		{
			const FString Name = Asset.AssetName.ToString();
			if (!IsVelociraptorMeshName(Name))
			{
				continue;
			}
			const FString Path = Asset.PackageName.ToString();
			if (!Best || PreferAssetName(Name, Path, FolderBestName, FolderBestPath))
			{
				Best = &Asset;
				FolderBestName = Name;
				FolderBestPath = Path;
			}
		}
		if (Best)
		{
			if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Best->GetAsset()))
			{
				return Accept(Mesh);
			}
		}
		UE_LOG(LogSolid, Warning, TEXT("No Velociraptor mesh in recorded folder %s. Searching."), *RecordedFolder);
	}

	TArray<FName> Content;
	Content.Add(FName(TEXT("/Game")));
	TArray<FAssetData> Assets;
	GatherAssets(Content, Classes, true, Assets);
	const FAssetData* Best = nullptr;
	FString SearchName;
	FString SearchPath;
	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		if (!IsVelociraptorMeshName(Name))
		{
			continue;
		}
		const FString Path = Asset.PackageName.ToString();
		if (!Best || PreferAssetName(Name, Path, SearchName, SearchPath))
		{
			Best = &Asset;
			SearchName = Name;
			SearchPath = Path;
		}
	}
	if (Best)
	{
		if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(Best->GetAsset()))
		{
			return Accept(Mesh);
		}
	}

	UE_LOG(LogSolid, Warning, TEXT("Velociraptor mesh not found under Content."));
	return nullptr;
}

UAnimSequence* SolidContentLookup::FindVelociraptorIdle(const USkeletalMesh* Mesh)
{
	const FString Manifest = SolidFabManifest::LoadManifest();
	TArray<FString> Recorded;
	SolidFabManifest::CollectContentFiles(Manifest, TEXT("velociraptor"), Recorded);

	UAnimSequence* BestRecorded = nullptr;
	FString BestName;
	FString BestPath;
	for (const FString& File : Recorded)
	{
		UAnimSequence* Clip = Cast<UAnimSequence>(
			SolidFabManifest::TryLoadContentFile(File, UAnimSequence::StaticClass()));
		if (!Clip || !IsVelociraptorIdleName(Clip->GetName()))
		{
			continue;
		}
		const FString Path = Clip->GetPathName();
		if (!BestRecorded || PreferShorterName(Clip->GetName(), Path, BestName, BestPath))
		{
			BestRecorded = Clip;
			BestName = Clip->GetName();
			BestPath = Path;
		}
	}
	if (BestRecorded)
	{
		return BestRecorded;
	}

	if (!Mesh)
	{
		return nullptr;
	}

	auto Search = [](const FString& Package) -> UAnimSequence*
	{
		if (Package.IsEmpty())
		{
			return nullptr;
		}
		TArray<FTopLevelAssetPath> Classes;
		Classes.Add(UAnimSequence::StaticClass()->GetClassPathName());
		TArray<FName> Packages;
		Packages.Add(FName(*Package));
		TArray<FAssetData> Assets;
		GatherAssets(Packages, Classes, true, Assets);
		const FAssetData* Best = nullptr;
		FString NameBest;
		FString PathBest;
		for (const FAssetData& Asset : Assets)
		{
			const FString Name = Asset.AssetName.ToString();
			if (!IsVelociraptorIdleName(Name))
			{
				continue;
			}
			const FString Path = Asset.PackageName.ToString();
			if (!Best || PreferShorterName(Name, Path, NameBest, PathBest))
			{
				Best = &Asset;
				NameBest = Name;
				PathBest = Path;
			}
		}
		return Best ? Cast<UAnimSequence>(Best->GetAsset()) : nullptr;
	};

	const FString Folder = PackageFolder(Mesh);
	UAnimSequence* Idle = Search(Folder);
	if (!Idle)
	{
		Idle = Search(ParentPackage(Folder));
	}
	if (Idle)
	{
		SolidFabManifest::RememberResolvedPath(TEXT("velociraptor"), Idle);
	}
	return Idle;
}

UMaterialInterface* SolidContentLookup::FindVelociraptorGreen(const USkeletalMesh* Mesh)
{
	TArray<FName> Packages;
	const FString Folder = PackageFolder(Mesh);
	if (!Folder.IsEmpty())
	{
		Packages.Add(FName(*Folder));
		const FString Parent = ParentPackage(Folder);
		if (!Parent.IsEmpty())
		{
			Packages.Add(FName(*Parent));
		}
	}

	const FString Manifest = SolidFabManifest::LoadManifest();
	FString RecordedFolder;
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("velociraptor"), TEXT("resolvedFolder"), RecordedFolder);
	const FString RecordedPackage = SolidFabManifest::ContentFolderToPackagePath(RecordedFolder);
	if (!RecordedPackage.IsEmpty())
	{
		Packages.AddUnique(FName(*RecordedPackage));
	}
	if (Packages.Num() == 0)
	{
		return nullptr;
	}

	TArray<FTopLevelAssetPath> Classes;
	Classes.Add(UMaterialInterface::StaticClass()->GetClassPathName());
	TArray<FAssetData> Assets;
	GatherAssets(Packages, Classes, true, Assets);

	const FAssetData* Best = nullptr;
	FString BestName;
	FString BestPath;
	for (const FAssetData& Asset : Assets)
	{
		const FString Name = Asset.AssetName.ToString();
		const FString Path = Asset.PackageName.ToString();
		if (!IsGreenerName(Name) || !ContainsWord(Path, TEXT("Velociraptor")))
		{
			continue;
		}
		if (!Best || PreferShorterName(Name, Path, BestName, BestPath))
		{
			Best = &Asset;
			BestName = Name;
			BestPath = Path;
		}
	}
	return Best ? Cast<UMaterialInterface>(Best->GetAsset()) : nullptr;
}
