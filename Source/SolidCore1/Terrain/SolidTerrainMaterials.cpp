#include "SolidTerrainMaterials.h"
#include "SolidCore1.h"
#include "SolidMaterials.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UMaterialInterface* SolidTerrainMaterials::FindFabGrass()
{
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game/Fab")));
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
			UE_LOG(LogSolid, Warning,
				TEXT("Terrain material: Fab grass %s"), *Asset.GetObjectPathString());
			return Grass;
		}
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
