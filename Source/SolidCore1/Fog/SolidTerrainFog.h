#pragma once

#include "CoreMinimal.h"

class AActor;
class AExponentialHeightFog;
class UMaterialInterface;
class USolidTerrainMap;
class UStaticMesh;
class UStaticMeshComponent;
class UObject;
class UWorld;

/**
 * Exploration fog-of-war helpers: distance bands, trail sampling, overlay mesh build,
 * materials, and (legacy) height-fog control.
 *
 * Visual FoW is world-space boundary curtains on fog=0 | fog>0 edges — not height fog
 * and not a dense lattice of volume cells.
 */
namespace SolidTerrainFog
{
	/** Points closer than this to origin (init) or player trail (runtime) are fully clear. */
	constexpr float ClearRadiusMeters = 25.f;
	/** Half fog starts at this distance from origin on the initial map fill. */
	constexpr float HalfFogStartMeters = 25.f;
	/** Full fog beyond this distance from origin on the initial map fill. */
	constexpr float FullFogStartMeters = 50.f;
	/** Samples at or below this count as clear (fog=0 side of a boundary). */
	constexpr float ClearFogEpsilon = 0.05f;
	/** Isocontour between half fog (0.5) and full fog (1.0). */
	constexpr float HalfFullFogThreshold = 0.75f;

	FORCEINLINE float MetersToCm(float Meters) { return Meters * 100.f; }

	/** Initial exploration fog from distance to world origin (meters). */
	float FogFromDistanceMeters(float DistM);

	struct FMeshBuildParams
	{
		FIntPoint ChunkCoord = FIntPoint::ZeroValue;
		float ChunkWorldSize = 6400.f;
		int32 FogQuadsPerSide = 16;
		float VolumeHeightCm = 3200.f;
		float VolumeHeightHalfCm = 2200.f;
		float CollisionHeightBias = 0.f;
	};

	struct FHeightFogStyle
	{
		float DensityAtHalf = 0.12f;
		float DensityAtFull = 0.35f;
		float MaxOpacityAtHalf = 0.75f;
		float MaxOpacityAtFull = 0.95f;
		FLinearColor MistColor = FLinearColor(0.72f, 0.78f, 0.82f);
	};

	UMaterialInterface* CreateVolumeMaterial(
		UObject* Outer,
		const FLinearColor& Color,
		float Opacity,
		const TCHAR* DebugName);

	UMaterialInterface* CreateHalfMaterial(UObject* Outer);
	UMaterialInterface* CreateFullMaterial(UObject* Outer);
	/** White curtain at the half→full fog boundary (~50m). */
	UMaterialInterface* CreateOuterMaterial(UObject* Outer);

	void ConfigureOverlayComponent(UStaticMeshComponent* Mesh);

	/**
	 * Build a non-colliding fog overlay mesh: marching-squares isocontour curtains
	 * at fog≈0 | fog>0 and at fog 0.5 | fog 1.0 (white outer ring).
	 */
	UStaticMesh* BuildChunkFogMesh(
		UObject* Outer,
		const USolidTerrainMap* TerrainMap,
		const FMeshBuildParams& Params,
		UMaterialInterface* HalfMaterial,
		UMaterialInterface* FullMaterial,
		UMaterialInterface* OuterMaterial = nullptr);

	/** HUD / legacy mist amount: max TerrainPoint.Fog on a pawn-centered ring (no camera). */
	float SampleMistAmountAround(const USolidTerrainMap* Map, const FVector& WorldLocation);

	void SilenceHeightFog(UWorld* World);
	AExponentialHeightFog* EnsureHeightFog(UWorld* World, AActor* Owner, const FLinearColor& MistColor);
	void ApplyHeightFogAmount(AExponentialHeightFog* FogActor, float Amount, const FHeightFogStyle& Style);
}
