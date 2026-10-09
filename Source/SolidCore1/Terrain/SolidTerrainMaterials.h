#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"

class UMaterialInterface;
class UObject;

/**
 * Terrain grass material resolution: optional override → Fab Mat_025_grass → FlatCol.
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
}
