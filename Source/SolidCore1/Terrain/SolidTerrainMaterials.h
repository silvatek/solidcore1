#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "SolidTerrainTypes.h"

class UMaterialInterface;
class UObject;
class USolidWorldChart;

/**
 * Terrain material resolution. Grassland and forest use the Fab grass texture.
 * Forest multiplies that texture by its darker factor. Other biomes are flat colours.
 * Used by ASolidTerrainStreamer; keeps AssetRegistry / MID spraying out of the streamer.
 */
namespace SolidTerrainMaterials
{
	/**
	 * Colour of one ground, and whether its mesh is the grass texture.
	 * Grassland and forest use the grass texture. GrassFactor multiplies that
	 * texture. Every other ground is a flat colour painted with Color.
	 */
	struct FBiomeMaterial
	{
		FLinearColor Color;
		bool bGrassTexture;
		FLinearColor GrassFactor;
	};

	SOLIDCORE1_API extern const FBiomeMaterial Grassland;
	SOLIDCORE1_API extern const FBiomeMaterial Forest;
	SOLIDCORE1_API extern const FBiomeMaterial Mountain;
	SOLIDCORE1_API extern const FBiomeMaterial Town;
	SOLIDCORE1_API extern const FBiomeMaterial Desert;
	SOLIDCORE1_API extern const FBiomeMaterial Swamp;
	SOLIDCORE1_API extern const FBiomeMaterial Sea;
	SOLIDCORE1_API extern const FBiomeMaterial River;
	SOLIDCORE1_API extern const FBiomeMaterial Road;
	SOLIDCORE1_API extern const FBiomeMaterial Unknown;

	struct FResolveParams
	{
		UObject* Outer = nullptr;
		UMaterialInterface* OverrideMaterial = nullptr;
		FLinearColor GrassColor = FLinearColor(0.12f, 0.28f, 0.07f);
		FLinearColor GrassDarkColor = FLinearColor(0.04f, 0.11f, 0.03f);
		float GrassRoughness = 0.97f;
		float GrassSpecular = 0.05f;
	};

	/** Recorded Fab path, then /Game/Fab, for Mat_025_grass (skips StaticMeshes). */
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
	 * Material for a biome. Grassland and forest use the grass texture from Resolve().
	 * Forest multiplies it by its GrassFactor. Other biomes use FlatCol tinted by the
	 * world-map colour, which is the biome material's Color. Results are cached.
	 */
	UMaterialInterface* ResolveForBiome(
		ESolidBiome Biome,
		const FResolveParams& Params,
		const USolidWorldChart* WorldMap,
		TObjectPtr<UMaterialInterface>& InOutGrassCached,
		TMap<ESolidBiome, TObjectPtr<UMaterialInterface>>& InOutBiomeMaterials);

	/**
	 * Lit material for a flat-colour fade. Its base colour is the vertex colour,
	 * and its roughness matches the flat biome colours. Every fading quad shares
	 * this one material. It is built once. Each corner carries that corner's colour,
	 * and the hardware blend across the quad is the fade.
	 */
	UMaterialInterface* FadeMaterial();

	/**
	 * Corner weights for a flat-colour fade, in mesh order 00, 10, 01, 11.
	 * Weight 0 is corner 00's biome. Weight 1 is the other flat colour on that shelf.
	 * The halfway line between those rows is the middle of the fade.
	 * False when the quad keeps the single material it uses now: one biome, three
	 * biomes, a road, grassland, or a higher shelf.
	 */
	bool FadeWeight(
		ESolidBiome B00,
		ESolidBiome B10,
		ESolidBiome B01,
		ESolidBiome B11,
		float OutWeights[4]);
}
