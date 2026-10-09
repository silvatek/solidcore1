#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "SolidTerrainTypes.h"

class UMaterialInterface;
class UObject;
class USolidWorldMap;

/**
 * Terrain material resolution: grassland (Fab/FlatCol) + solid-color biomes from WorldMap.
 * Used by ASolidTerrainStreamer; keeps AssetRegistry / MID spraying out of the streamer.
 */
namespace SolidTerrainMaterials
{
	struct FResolveParams
	{
		UObject* Outer = nullptr;
		UMaterialInterface* OverrideMaterial = nullptr;
		FLinearColor GrassColor = FLinearColor(0.12f, 0.28f, 0.07f);
		FLinearColor GrassDarkColor = FLinearColor(0.04f, 0.11f, 0.03f);
		float GrassRoughness = 0.97f;
		float GrassSpecular = 0.05f;
	};

	/** Search /Game/Fab for Mat_025_grass (skips StaticMeshes package). */
	UMaterialInterface* FindFabGrass();

	/** FlatCol solid green MID from GrassColor / GrassDarkColor blend. */
	UMaterialInterface* CreateFlatColGrass(const FResolveParams& Params);

	/** MID that forces matte roughness / low specular on a Fab or override parent. */
	UMaterialInterface* MakeMatteInstance(
		UObject* Outer,
		UMaterialInterface* Parent,
		float Roughness,
		float Specular);

	/**
	 * Resolve grass material into InOutCached (no-op if already set).
	 * Order: override (non-PrototypeGrid) → Fab grass → FlatCol.
	 */
	UMaterialInterface* Resolve(const FResolveParams& Params, TObjectPtr<UMaterialInterface>& InOutCached);

	/**
	 * Material for a biome. Grassland uses Resolve(); others use FlatCol tinted by WorldMap
	 * key color (or built-in defaults). Results cached in InOutBiomeMaterials.
	 */
	UMaterialInterface* ResolveForBiome(
		ESolidBiome Biome,
		const FResolveParams& Params,
		const USolidWorldMap* WorldMap,
		TObjectPtr<UMaterialInterface>& InOutGrassCached,
		TMap<ESolidBiome, TObjectPtr<UMaterialInterface>>& InOutBiomeMaterials);
}
