#include "SolidTerrainMaterials.h"
#include "SolidBiome.h"
#include "SolidCore1.h"
#include "SolidFabManifest.h"
#include "SolidMaterials.h"
#include "SolidWorldChart.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionVertexColor.h"
#endif

namespace SolidTerrainMaterials
{
	const FBiomeMaterial Grassland{FLinearColor(0.45f, 0.72f, 0.28f), true, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Forest{FLinearColor(0.08f, 0.28f, 0.08f), true, FLinearColor(0.40f, 0.40f, 0.40f, 1.f)};
	const FBiomeMaterial Mountain{FLinearColor(0.16f, 0.16f, 0.18f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Town{FLinearColor(0.45f, 0.28f, 0.12f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Desert{FLinearColor(0.85f, 0.75f, 0.32f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Swamp{FLinearColor(0.22f, 0.32f, 0.18f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Sea{FLinearColor(0.15f, 0.35f, 0.75f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial River{FLinearColor(0.15f, 0.35f, 0.75f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Road{FLinearColor(0.52f, 0.38f, 0.20f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
	const FBiomeMaterial Unknown{FLinearColor(0.4f, 0.4f, 0.4f), false, FLinearColor(1.f, 1.f, 1.f, 1.f)};
}

namespace
{
	UMaterialInterface* FindGrassInPackages(const TArray<FName>& PackagePaths)
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
		Filter.ClassPaths.Add(UMaterial::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(UMaterialInstance::StaticClass()->GetClassPathName());
		Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);

		for (const FAssetData& Asset : Assets)
		{
			const FString Name = Asset.AssetName.ToString();
			if (!Name.Equals(TEXT("Mat_025_grass"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			// Skip the StaticMeshes package that shares the asset name.
			if (Asset.PackageName.ToString().Contains(TEXT("/StaticMeshes/"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			if (UMaterialInterface* Grass = Cast<UMaterialInterface>(Asset.GetAsset()))
			{
				return Grass;
			}
		}
		return nullptr;
	}

bool HasGrassFactor(UMaterialInterface* Material)
{
	if (!Material)
	{
		return false;
	}

	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	Material->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		if (Info.Name == TEXT("BaseColorFactor") || Info.Name == TEXT("BaseColorFactor_RGB"))
		{
			return true;
		}
	}
	return false;
}

UMaterialInterface* DarkenGrass(UObject* Outer, UMaterialInterface* Grass, const FLinearColor& Factor)
{
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Grass, Outer);
	if (!MID)
	{
		return Grass;
	}

	TArray<FMaterialParameterInfo> Infos;
	TArray<FGuid> Ids;
	MID->GetAllVectorParameterInfo(Infos, Ids);
	for (const FMaterialParameterInfo& Info : Infos)
	{
		if (Info.Name == TEXT("BaseColorFactor") || Info.Name == TEXT("BaseColorFactor_RGB"))
		{
			MID->SetVectorParameterValue(Info.Name, Factor);
		}
	}
	MID->RecacheUniformExpressions(true);
	UE_LOG(LogSolid, Warning, TEXT("Terrain material: darker grass factor (%.2f, %.2f, %.2f)"),
		Factor.R, Factor.G, Factor.B);
	return MID;
}
}

UMaterialInterface* SolidTerrainMaterials::FindFabGrass()
{
	const FString Manifest = SolidFabManifest::LoadManifest();
	FString RecordedFile;
	FString RecordedFolder;
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("grass"), TEXT("resolvedPath"), RecordedFile);
	SolidFabManifest::ReadJsonStringField(Manifest, TEXT("grass"), TEXT("resolvedFolder"), RecordedFolder);
	const bool bFile = SolidFabManifest::ContentFileExists(RecordedFile);
	const bool bFolder = SolidFabManifest::ContentDirectoryExists(RecordedFolder);

	if (SolidFabManifest::ChooseFabLookup(bFile, bFolder) == ESolidFabLookup::RecordedFile)
	{
		if (UMaterialInterface* Recorded = Cast<UMaterialInterface>(
			SolidFabManifest::TryLoadContentFile(RecordedFile, UMaterialInterface::StaticClass())))
		{
			UE_LOG(LogSolid, Warning, TEXT("Terrain material: recorded Fab grass %s"), *Recorded->GetPathName());
			return Recorded;
		}
		UE_LOG(LogSolid, Warning, TEXT("Recorded Fab grass is missing. Searching."));
	}

	auto Accept = [](UMaterialInterface* Grass) -> UMaterialInterface*
	{
		if (Grass)
		{
			SolidFabManifest::RememberDiscoveredFile(TEXT("grass"), Grass);
			UE_LOG(LogSolid, Warning, TEXT("Terrain material: Fab grass %s"), *Grass->GetPathName());
		}
		return Grass;
	};

	if (bFolder)
	{
		const FString Package = SolidFabManifest::ContentFolderToPackagePath(RecordedFolder);
		TArray<FName> Packages;
		if (!Package.IsEmpty())
		{
			Packages.Add(FName(*Package));
		}
		if (UMaterialInterface* Grass = FindGrassInPackages(Packages))
		{
			return Accept(Grass);
		}
		UE_LOG(LogSolid, Warning, TEXT("No Mat_025_grass in recorded folder %s. Searching."), *RecordedFolder);
	}

	TArray<FName> FabPackages;
	FabPackages.Add(FName(TEXT("/Game/Fab")));
	if (UMaterialInterface* Grass = FindGrassInPackages(FabPackages))
	{
		return Accept(Grass);
	}

	UE_LOG(LogSolid, Warning, TEXT("Fab Mat_025_grass not found under /Game/Fab."));
	return nullptr;
}

UMaterialInterface* SolidTerrainMaterials::CreateFlatColGrass(const FResolveParams& Params)
{
	if (!Params.Outer)
	{
		return nullptr;
	}

	const FLinearColor MidGrass =
		FLinearColor::LerpUsingHSV(Params.GrassDarkColor, Params.GrassColor, 0.55f);
	UMaterialInterface* GrassMID =
		SolidMaterials::CreateSolidColor(Params.Outer, MidGrass, TEXT("FlatColGrass"));
	if (GrassMID)
	{
		UE_LOG(LogSolid, Warning, TEXT("Terrain material: solid green (FlatCol fallback)"));
	}
	return GrassMID;
}

UMaterialInterface* SolidTerrainMaterials::MakeMatteInstance(
	UObject* Outer,
	UMaterialInterface* Parent,
	float Roughness,
	float Specular)
{
	if (!Parent)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Parent, Outer);
	if (!MID)
	{
		return Parent;
	}

	Roughness = FMath::Clamp(Roughness, 0.f, 1.f);
	Specular = FMath::Clamp(Specular, 0.f, 1.f);

	// Blanket sets for common Fab / Quixel / FlatCol names.
	static const TCHAR* RoughnessNames[] = {
		TEXT("Roughness"), TEXT("roughness"), TEXT("RoughnessAmount"), TEXT("RoughnessIntensity"),
		TEXT("Roughness Min"), TEXT("RoughnessMax"), TEXT("Roughness Max"), TEXT("RoughnessMultiply"),
		TEXT("Roughness Scale"), TEXT("RoughnessScale"), TEXT("ORM Roughness"),
	};
	static const TCHAR* SpecularNames[] = {
		TEXT("Specular"), TEXT("specular"), TEXT("SpecularAmount"), TEXT("SpecularIntensity"),
		TEXT("Spec"), TEXT("SpecularScale"),
	};
	static const TCHAR* MetallicNames[] = {
		TEXT("Metallic"), TEXT("metallic"), TEXT("MetallicAmount"), TEXT("Metalness"),
	};

	for (const TCHAR* Name : RoughnessNames)
	{
		MID->SetScalarParameterValue(Name, Roughness);
	}
	for (const TCHAR* Name : SpecularNames)
	{
		MID->SetScalarParameterValue(Name, Specular);
	}
	for (const TCHAR* Name : MetallicNames)
	{
		MID->SetScalarParameterValue(Name, 0.f);
	}

	// Also drive any scalar the parent actually exposes whose name looks relevant.
	TArray<FMaterialParameterInfo> ScalarInfos;
	TArray<FGuid> ScalarIds;
	Parent->GetAllScalarParameterInfo(ScalarInfos, ScalarIds);
	for (const FMaterialParameterInfo& Info : ScalarInfos)
	{
		const FString Name = Info.Name.ToString();
		if (Name.Contains(TEXT("Rough"), ESearchCase::IgnoreCase))
		{
			MID->SetScalarParameterValue(Info.Name, Roughness);
		}
		else if (Name.Contains(TEXT("Spec"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Gloss"), ESearchCase::IgnoreCase)
			|| Name.Contains(TEXT("Shine"), ESearchCase::IgnoreCase))
		{
			// Gloss/shine often inverted vs roughness — keep low for less shine.
			const bool bLooksLikeGloss = Name.Contains(TEXT("Gloss"), ESearchCase::IgnoreCase)
				|| Name.Contains(TEXT("Shine"), ESearchCase::IgnoreCase);
			MID->SetScalarParameterValue(Info.Name, bLooksLikeGloss ? (1.f - Roughness) : Specular);
		}
		else if (Name.Contains(TEXT("Metal"), ESearchCase::IgnoreCase))
		{
			MID->SetScalarParameterValue(Info.Name, 0.f);
		}
	}

	UE_LOG(LogSolid, Warning,
		TEXT("Terrain material: matte MID on %s (roughness=%.2f specular=%.2f, %d scalar params scanned)"),
		*Parent->GetName(), Roughness, Specular, ScalarInfos.Num());
	return MID;
}

UMaterialInterface* SolidTerrainMaterials::Resolve(
	const FResolveParams& Params,
	TObjectPtr<UMaterialInterface>& InOutCached)
{
	if (InOutCached)
	{
		return InOutCached;
	}

	if (!Params.Outer)
	{
		UE_LOG(LogSolid, Error, TEXT("Terrain material: Resolve called with null Outer."));
		return nullptr;
	}

	// Never use M_PrototypeGrid (hard-wired grey checker).

	// 1) Explicit override (skip PrototypeGrid if someone set it).
	if (Params.OverrideMaterial)
	{
		const FString MatName = Params.OverrideMaterial->GetName();
		if (!MatName.Contains(TEXT("PrototypeGrid"), ESearchCase::IgnoreCase))
		{
			InOutCached = MakeMatteInstance(
				Params.Outer,
				Params.OverrideMaterial,
				Params.GrassRoughness,
				Params.GrassSpecular);
			UE_LOG(LogSolid, Warning, TEXT("Terrain material: override %s (matte)"), *MatName);
			return InOutCached;
		}
		UE_LOG(LogSolid, Warning,
			TEXT("Ignoring TerrainMaterial '%s' (PrototypeGrid cannot be tinted)."), *MatName);
	}

	// 2) Fab seamless grass (Content/Fab/.../Mat_025_grass), forced matte.
	if (UMaterialInterface* FabGrass = FindFabGrass())
	{
		InOutCached = MakeMatteInstance(
			Params.Outer, FabGrass, Params.GrassRoughness, Params.GrassSpecular);
		return InOutCached;
	}

	// 3) FlatCol solid green fallback.
	if (UMaterialInterface* FlatGrass = CreateFlatColGrass(Params))
	{
		InOutCached = FlatGrass;
		return InOutCached;
	}

	UE_LOG(LogSolid, Error, TEXT("Terrain material: no grass material could be created."));
	return nullptr;
}

UMaterialInterface* SolidTerrainMaterials::ResolveForBiome(
	const ESolidBiome Biome,
	const FResolveParams& Params,
	const USolidWorldChart* WorldMap,
	TObjectPtr<UMaterialInterface>& InOutGrassCached,
	TMap<ESolidBiome, TObjectPtr<UMaterialInterface>>& InOutBiomeMaterials)
{
	if (TObjectPtr<UMaterialInterface>* Existing = InOutBiomeMaterials.Find(Biome))
	{
		if (Existing->Get())
		{
			return Existing->Get();
		}
	}

	if (!Params.Outer)
	{
		return nullptr;
	}

	const FBiomeMaterial& Defined = SolidBiome::Of(Biome).Material();
	if (Defined.bGrassTexture)
	{
		UMaterialInterface* Grass = Resolve(Params, InOutGrassCached);
		if (!Grass)
		{
			return nullptr;
		}

		UMaterialInterface* Built = Grass;
		const bool bCanTint = HasGrassFactor(Grass);
		const FLinearColor Authored(1.f, 1.f, 1.f, 1.f);
		if (bCanTint && !Defined.GrassFactor.Equals(Authored))
		{
			Built = DarkenGrass(Params.Outer, Grass, Defined.GrassFactor);
		}
		else if (!bCanTint && Biome != ESolidBiome::Grassland)
		{
			Built = SolidMaterials::CreateSolidColor(
				Params.Outer,
				Defined.Color,
				SolidBiome::Of(Biome).Name(),
				Params.GrassRoughness);
		}
		InOutBiomeMaterials.Add(Biome, Built);
		return Built;
	}

	const FLinearColor Color = WorldMap
		? WorldMap->GetBiomeColor(Biome)
		: Defined.Color;

	UMaterialInterface* Solid = SolidMaterials::CreateSolidColor(
		Params.Outer,
		Color,
		SolidBiome::Of(Biome).Name(),
		Params.GrassRoughness);
	InOutBiomeMaterials.Add(Biome, Solid);
	return Solid;
}

UMaterialInterface* SolidTerrainMaterials::FadeMaterial()
{
	static TStrongObjectPtr<UMaterialInterface> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

#if WITH_EDITOR
	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
	if (!Material)
	{
		return nullptr;
	}

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);

	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	if (!EditorData)
	{
		UE_LOG(LogSolid, Error, TEXT("Biome fade: GetEditorOnlyData() null."));
		return nullptr;
	}

	UMaterialExpressionVertexColor* VertexColor = NewObject<UMaterialExpressionVertexColor>(Material);
	VertexColor->MaterialExpressionEditorX = -280;
	VertexColor->MaterialExpressionEditorY = 0;

	UMaterialExpressionConstant* Roughness = NewObject<UMaterialExpressionConstant>(Material);
	Roughness->R = 0.97f;
	Roughness->MaterialExpressionEditorX = -280;
	Roughness->MaterialExpressionEditorY = 180;

	EditorData->ExpressionCollection.Expressions.Add(VertexColor);
	EditorData->ExpressionCollection.Expressions.Add(Roughness);
	EditorData->BaseColor.Expression = VertexColor;
	EditorData->Roughness.Expression = Roughness;

	Material->SetMaterialUsage(MATUSAGE_StaticMesh);
	Material->PreEditChange(nullptr);
	Material->PostEditChange();

	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
	if (Instance)
	{
		Cached.Reset(Instance);
		return Instance;
	}

	Cached.Reset(Material);
	return Material;
#else
	return nullptr;
#endif
}

bool SolidTerrainMaterials::FadeWeight(
	const ESolidBiome B00,
	const ESolidBiome B10,
	const ESolidBiome B01,
	const ESolidBiome B11,
	float OutWeights[4])
{
	const ESolidBiome Corners[4] = { B00, B10, B01, B11 };
	const FSolidBiome& First = SolidBiome::Of(B00);
	if (!First.FlatColour())
	{
		return false;
	}

	const float Shelf = First.HeightOffsetCm();
	ESolidBiome Other = B00;
	for (const ESolidBiome Corner : Corners)
	{
		const FSolidBiome& Ground = SolidBiome::Of(Corner);
		if (!Ground.FlatColour() || Ground.HeightOffsetCm() != Shelf)
		{
			return false;
		}
		if (Corner == B00)
		{
			continue;
		}
		if (Other == B00)
		{
			Other = Corner;
		}
		else if (Corner != Other)
		{
			return false;
		}
	}
	if (Other == B00)
	{
		return false;
	}

	for (int32 Index = 0; Index < 4; ++Index)
	{
		OutWeights[Index] = Corners[Index] == B00 ? 0.f : 1.f;
	}
	return true;
}
