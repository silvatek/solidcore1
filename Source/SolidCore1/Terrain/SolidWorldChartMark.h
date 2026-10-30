#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"
#include "SolidWorldChartMark.generated.h"

/** What a mark on the world chart is. A site has a size and no type. */
UENUM()
enum class ESolidWorldChartMarkKind : uint8
{
	Town,
	Stronghold,
	StrongholdSite,
	LairEntrance,
	LairExit,
	/** A short note. It does not change the land, and it is not a road end. */
	Custom,
	/** A catalog encounter. It does not change the land. */
	Encounter,
};

/**
 * One named place on the chart. Its position is the cell centre, in centimetres.
 * Asking the chart turns that position into a cell once, then keeps the cell.
 */
USTRUCT()
struct FSolidWorldChartMark
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidWorldChartMarkKind Kind = ESolidWorldChartMarkKind::Town;

	/**
	 * Town, stronghold, site, or lair name, or the text of a custom note.
	 * MeagreDungeon and Meagre Dungeon are the same stronghold name.
	 */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString Name;

	/** World position in centimetres, snapped to the cell centre. +X is west, +Y is north. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FVector2D PositionCm = FVector2D::ZeroVector;

	/** Set for a stronghold. A site has no type. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidStrongholdType StrongholdType = ESolidStrongholdType::Dungeon;

	/** Set for a stronghold and a stronghold site. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	ESolidStrongholdSize StrongholdSize = ESolidStrongholdSize::Meagre;

	/** Eight hex digits when this mark is a lair mouth. Empty otherwise. */
	UPROPERTY(EditAnywhere, Category = "WorldChart")
	FString LairId;
};
