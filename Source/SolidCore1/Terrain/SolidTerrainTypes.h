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
	Swamp UMETA(DisplayName = "Swamp")
};

/** One cell in the world simulation grid (gameplay + heightfield source). */
USTRUCT(BlueprintType)
struct FSolidTerrainPoint
{
	GENERATED_BODY()

	/** World X in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float X = 0.f;

	/** World Y in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Y = 0.f;

	/** World surface height in cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Height = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	ESolidBiome Biome = ESolidBiome::Grassland;

	/** Relative danger in [0, 1]. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Threat = 0.f;

	/** Exploration fog / obscurity in [0, 1] (1 = fully fogged). */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Fog = 1.f;
};

namespace SolidTerrainTypes
{
	FORCEINLINE const TCHAR* BiomeToString(ESolidBiome Biome)
	{
		switch (Biome)
		{
		case ESolidBiome::Grassland: return TEXT("Grassland");
		case ESolidBiome::Forest: return TEXT("Forest");
		case ESolidBiome::Mountain: return TEXT("Mountain");
		case ESolidBiome::Town: return TEXT("Town");
		case ESolidBiome::Desert: return TEXT("Desert");
		case ESolidBiome::Swamp: return TEXT("Swamp");
		default: return TEXT("Unknown");
		}
	}
}
