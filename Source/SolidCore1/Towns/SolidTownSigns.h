#pragma once

#include "CoreMinimal.h"

class USolidWorldMap;

/**
 * Welcome signs for numbered WorldMap locations.
 * The board reads "Welcome to" on the first line and the town name on the second.
 * One placement per named cell (Kanfold's two cells each get a sign).
 * Pure logic — no actor spawning (safe for automation).
 */
namespace SolidTownSigns
{
	/** Board front (+local X) faces world -Y, toward a player standing south of the town. */
	inline constexpr float FacingYawDeg = -90.f;

	inline constexpr float PoleThicknessCm = 14.f;
	inline constexpr float PoleHeightCm = 260.f;
	inline constexpr float BoardDepthCm = 8.f;
	inline constexpr float TextWorldSize = 46.f;
	inline constexpr float OutlineOffsetCm = 3.5f;
	/** First line of every welcome board. */
	inline constexpr const TCHAR* WelcomeLine = TEXT("Welcome to");
	/** Two lines of glyphs, plus the padding the single-line board used. */
	inline constexpr float BoardHeightCm = TextWorldSize * 2.85f;

	inline FLinearColor PoleColor()
	{
		return FLinearColor(0.18f, 0.09f, 0.04f);
	}

	inline FLinearColor BoardColor()
	{
		return FLinearColor(0.62f, 0.42f, 0.22f);
	}

	struct FPlacement
	{
		int32 LocationId = INDEX_NONE;
		FString Name;
		FIntPoint Cell = FIntPoint::ZeroValue;
		FVector2D WorldXY = FVector2D::ZeroVector;
		/** Actor yaw from the town. The default faces south. */
		float YawDeg = FacingYawDeg;
	};

	/** "Welcome to" and the town name, on two lines. */
	FString MakeLabel(const FString& TownName);

	/** Width of the board for that label: the longer line, plus a margin. */
	float BoardWidthCm(const FString& Label);

	/**
	 * One placement per cell of each location that has a non-empty name.
	 * Ordered by location id, then map row, then column.
	 * @return Number of placements written.
	 */
	int32 CollectPlacements(
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FPlacement>& OutPlacements);
}
