#pragma once

#include "CoreMinimal.h"
#include "SolidTerrainTypes.generated.h"

/** Coarse gameplay biome for a terrain simulation point. */
UENUM(BlueprintType)
enum class ESolidBiome : uint8
{
	Grassland UMETA(DisplayName = "Grassland"),
	Forest UMETA(DisplayName = "Forest"),
	Mountain UMETA(DisplayName = "Mountain"),
	Town UMETA(DisplayName = "Town"),
	Desert UMETA(DisplayName = "Desert"),
	Swamp UMETA(DisplayName = "Swamp"),
	Sea UMETA(DisplayName = "Sea"),
	River UMETA(DisplayName = "River"),
	Road UMETA(DisplayName = "Road"),
};

/** One terrain point. WorldX and WorldY are world centimetres. */
USTRUCT(BlueprintType)
struct FSolidTerrainPoint
{
	GENERATED_BODY()

	/** WorldX in centimetres. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float WorldX = 0.f;

	/** WorldY in centimetres. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float WorldY = 0.f;

	/** World surface height in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Height = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	ESolidBiome Biome = ESolidBiome::Grassland;

	/** Constant. Play ignores it. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Threat = 0.f;

	/** Exploration fog / obscurity in [0, 1] (1 = fully fogged). Distance bands: SolidTerrainFog.h. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Fog = 1.f;
};
