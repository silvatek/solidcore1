#pragma once

#include "CoreMinimal.h"
#include "SolidCore1TerrainTypes.generated.h"

/** Coarse gameplay biome for a terrain simulation point. */
UENUM(BlueprintType)
enum class ESolidCore1Biome : uint8
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
struct FSolidCore1TerrainPoint
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
	ESolidCore1Biome Biome = ESolidCore1Biome::Grassland;

	/** Relative danger in [0, 1]. */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Threat = 0.f;

	/** Exploration fog / obscurity in [0, 1] (1 = fully fogged). */
	UPROPERTY(BlueprintReadOnly, Category = "Terrain")
	float Fog = 1.f;
};

namespace SolidCore1TerrainTypes
{
	FORCEINLINE const TCHAR* BiomeToString(ESolidCore1Biome Biome)
	{
		switch (Biome)
		{
		case ESolidCore1Biome::Grassland: return TEXT("Grassland");
		case ESolidCore1Biome::Forest: return TEXT("Forest");
		case ESolidCore1Biome::Mountain: return TEXT("Mountain");
		case ESolidCore1Biome::Town: return TEXT("Town");
		case ESolidCore1Biome::Desert: return TEXT("Desert");
		case ESolidCore1Biome::Swamp: return TEXT("Swamp");
		default: return TEXT("Unknown");
		}
	}
}
