#pragma once

#include "CoreMinimal.h"
#include "SolidStrongholdGrid.h"
#include "SolidTerrainTypes.h"

/** What the company base is. Dungeon is the first; Fortress and Tower come later. */
enum class ESolidStrongholdType : uint8
{
	Dungeon,
	Fortress,
	Tower,
};

/**
 * How grand the company base is.
 * Meagre is the start. Later upgrades move up this list.
 */
enum class ESolidStrongholdSize : uint8
{
	Meagre,
	Humble,
	Average,
	Impressive,
	Grand,
	Stately,
	Imperious,
};

/** The closed visit. Cave is the dungeon trip. Tower is the tower trip. None does not travel. */
enum class ESolidStrongholdTrip : uint8
{
	None,
	Cave,
	Tower,
};

/** Which way a stronghold exit faces. */
enum class ESolidStrongholdOrientation : uint8
{
	North,
	East,
	South,
	West,
};

class AActor;
class USolidWorldMap;

/**
 * A company base: one type, one size, the map cell it stands on, and the way its exit faces.
 * Dungeon, Fortress, and Tower are the subclasses.
 * These are plain C++ classes, not Unreal objects and not Blueprints.
 */
class FSolidStronghold
{
public:
	FSolidStronghold() = default;
	FSolidStronghold(
		ESolidStrongholdType InType,
		ESolidStrongholdSize InSize,
		FIntPoint InCell = FIntPoint::ZeroValue,
		FVector2D InLocation = FVector2D::ZeroVector)
		: Type(InType)
		, Size(InSize)
		, Cell(InCell)
		, Location(InLocation)
	{
	}

	virtual ~FSolidStronghold() = default;
	FSolidStronghold(const FSolidStronghold&) = default;
	FSolidStronghold& operator=(const FSolidStronghold&) = default;

	ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
	/** Map cell. X is the column and Y is the row, counting from 0. Prefer Location. */
	FIntPoint Cell = FIntPoint::ZeroValue;
	/** World centimetres of the cell center. Height is the ground under this point. */
	FVector2D Location = FVector2D::ZeroVector;
	/** Direction the exit faces. The starting cave faces north. */
	ESolidStrongholdOrientation Orientation = ESolidStrongholdOrientation::North;

	/** True when the current code can build this type at Size. */
	virtual bool Supported(ESolidStrongholdSize) const { return false; }

	/** The cliff mouth. A stronghold that places its own exterior returns false. */
	virtual bool UsesCliffMouth() const { return false; }

	/** Which trip this type takes. */
	virtual ESolidStrongholdTrip Trip() const { return ESolidStrongholdTrip::None; }

	/** Actor class for this stronghold's exterior. Null when this kind draws none. */
	virtual UClass* ExteriorClass() const { return nullptr; }

	/** Fill an actor of ExteriorClass() from this stronghold. */
	virtual void ConfigureExterior(AActor* Exterior) const {}

	/**
	 * World yaw of that exterior.
	 * A dungeon points local +X along its exit. A tower turns its door toward the grass.
	 */
	virtual float ExteriorYawDeg(const USolidWorldMap* Map) const;

	/**
	 * Ground-floor hexes this type keeps at Size.
	 * Empty when that size has no floor. A move onto an empty floor is refused.
	 */
	virtual void CollectGroundHexes(
		ESolidStrongholdSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
	}

	/** A copy that keeps this subclass, the way a body part clones its own mesh. */
	virtual TUniquePtr<FSolidStronghold> Clone() const;
};

namespace SolidStronghold
{
	/** An owned dungeon, fortress, or tower of this size, standing at WorldXY. */
	TUniquePtr<FSolidStronghold> Make(
		ESolidStrongholdType Type,
		ESolidStrongholdSize Size,
		FIntPoint Cell,
		FVector2D WorldXY);

	/**
	 * Cell center on the shipped terrain, in world centimetres.
	 * The terrain is 513 points at 200 cm, centered on the origin, and the map is 128 by 64 cells.
	 */
	FVector2D WorldXY(FIntPoint Cell);
}

/**
 * Camera when the captain arrives.
 * Pitch 0 is level. Yaw is degrees.
 * Inside, yaw 0 looks in (interior +X, away from the mouth) and 180 looks out at the mouth.
 * Outside, yaw is added to the mouth's facing yaw: 0 looks out over the grass, 180 looks back in at the cliff.
 */
struct FSolidStrongholdView
{
	float PitchDeg = 0.f;
	float YawDeg = 0.f;
};

namespace SolidStronghold
{
	inline const ESolidStrongholdType Types[] = {
		ESolidStrongholdType::Dungeon,
		ESolidStrongholdType::Fortress,
		ESolidStrongholdType::Tower,
	};

	inline constexpr ESolidStrongholdSize Sizes[] = {
		ESolidStrongholdSize::Meagre,
		ESolidStrongholdSize::Humble,
		ESolidStrongholdSize::Average,
		ESolidStrongholdSize::Impressive,
		ESolidStrongholdSize::Grand,
		ESolidStrongholdSize::Stately,
		ESolidStrongholdSize::Imperious,
	};

	inline constexpr int32 TypeCount = UE_ARRAY_COUNT(Types);
	inline constexpr int32 SizeCount = UE_ARRAY_COUNT(Sizes);

	/** WorldMap.txt row of that dungeon, counting from 1 at the top. */
	inline constexpr int32 StartingRow = 9;
	/** WorldMap.txt column of that dungeon, counting from 1 at the left. */
	inline constexpr int32 StartingColumn = 17;

	/** Map cell of the starting stronghold. X is the column, Y is the row, counting from 0. */
	inline FIntPoint StartingCell()
	{
		return FIntPoint(StartingColumn - 1, StartingRow - 1);
	}

	/** Enter and leave aims for one stronghold. Missing rows use a level in/out aim. */
	struct FVisitView
	{
		ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
		ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
		FSolidStrongholdView Enter;
		FSolidStrongholdView Leave;
	};

	/** Meagre Dungeon: level, into the cave on entry, and level, out over the grass on leave. */
	inline const FVisitView VisitViews[] = {
		{ ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre, { 0.f, 0.f }, { 0.f, 0.f } },
	};

	inline constexpr int32 VisitViewCount = UE_ARRAY_COUNT(VisitViews);

	inline FVisitView VisitViewFor(const FSolidStronghold& Stronghold)
	{
		for (const FVisitView& View : VisitViews)
		{
			if (View.Type == Stronghold.Type && View.Size == Stronghold.Size)
			{
				return View;
			}
		}
		return { Stronghold.Type, Stronghold.Size, { 0.f, 0.f }, { 0.f, 0.f } };
	}

	/** World yaw for the grass outside a mouth that already faces the grass. */
	inline float LeaveYawDeg(const float MouthYawDeg, const FSolidStrongholdView& Leave)
	{
		return FRotator::NormalizeAxis(MouthYawDeg + Leave.YawDeg);
	}

	/** World yaw of an exit. Local +X of the exterior points this way. North is world +Y. */
	inline float OrientationYawDeg(const ESolidStrongholdOrientation Orientation)
	{
		switch (Orientation)
		{
		case ESolidStrongholdOrientation::North: return 90.f;
		case ESolidStrongholdOrientation::East: return 0.f;
		case ESolidStrongholdOrientation::South: return -90.f;
		case ESolidStrongholdOrientation::West: return 180.f;
		default: return 90.f;
		}
	}

	inline const TCHAR* TypeName(ESolidStrongholdType Type)
	{
		switch (Type)
		{
		case ESolidStrongholdType::Dungeon: return TEXT("Dungeon");
		case ESolidStrongholdType::Fortress: return TEXT("Fortress");
		case ESolidStrongholdType::Tower: return TEXT("Tower");
		default: return TEXT("");
		}
	}

	inline const TCHAR* SizeName(ESolidStrongholdSize Size)
	{
		switch (Size)
		{
		case ESolidStrongholdSize::Meagre: return TEXT("Meagre");
		case ESolidStrongholdSize::Humble: return TEXT("Humble");
		case ESolidStrongholdSize::Average: return TEXT("Average");
		case ESolidStrongholdSize::Impressive: return TEXT("Impressive");
		case ESolidStrongholdSize::Grand: return TEXT("Grand");
		case ESolidStrongholdSize::Stately: return TEXT("Stately");
		case ESolidStrongholdSize::Imperious: return TEXT("Imperious");
		default: return TEXT("");
		}
	}

	/** "Meagre Dungeon". */
	inline FString Label(const FSolidStronghold& Stronghold)
	{
		return FString::Printf(TEXT("%s %s"), SizeName(Stronghold.Size), TypeName(Stronghold.Type));
	}

	/**
	 * The shelf a defined stronghold sits on. The map cell uses this biome.
	 * A size and type with no row keeps the marker's own biome.
	 */
	struct FShelf
	{
		ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
		ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
		ESolidBiome Biome = ESolidBiome::Grassland;
	};

	inline const FShelf Shelves[] = {
		{ ESolidStrongholdType::Dungeon, ESolidStrongholdSize::Meagre, ESolidBiome::Grassland },
		{ ESolidStrongholdType::Tower, ESolidStrongholdSize::Imperious, ESolidBiome::Grassland },
	};

	inline constexpr int32 ShelfCount = UE_ARRAY_COUNT(Shelves);

	/** True when this stronghold names a shelf. */
	inline bool BiomeFor(const FSolidStronghold& Stronghold, ESolidBiome& OutBiome)
	{
		for (const FShelf& Shelf : Shelves)
		{
			if (Shelf.Type == Stronghold.Type && Shelf.Size == Stronghold.Size)
			{
				OutBiome = Shelf.Biome;
				return true;
			}
		}
		return false;
	}

	/** The cliff mouth. A stronghold that places its own exterior does not use this. */
	bool UsesCaveMouth(const FSolidStronghold& Stronghold);

	/** "MeagreDungeon" or "Meagre Dungeon": size, then type. */
	inline bool FromToken(const FString& Token, ESolidStrongholdType& OutType, ESolidStrongholdSize& OutSize)
	{
		FString Compact = Token;
		Compact.ReplaceInline(TEXT(" "), TEXT(""));
		if (Compact.IsEmpty())
		{
			return false;
		}
		for (const ESolidStrongholdSize Size : Sizes)
		{
			for (const ESolidStrongholdType Type : Types)
			{
				const FString Joined = FString(SizeName(Size)) + TypeName(Type);
				if (Compact.Equals(Joined, ESearchCase::IgnoreCase))
				{
					OutType = Type;
					OutSize = Size;
					return true;
				}
			}
		}
		return false;
	}

	/**
	 * A size name alone, such as "Humble" or "humble".
	 * "Humble Tower" and "MeagreDungeon" are stronghold tokens, not sizes.
	 */
	inline bool SizeFromToken(const FString& Token, ESolidStrongholdSize& Out)
	{
		const FString Compact = Token.TrimStartAndEnd();
		if (Compact.IsEmpty() || Compact.Contains(TEXT(" ")))
		{
			return false;
		}
		for (const ESolidStrongholdSize Size : Sizes)
		{
			if (Compact.Equals(SizeName(Size), ESearchCase::IgnoreCase))
			{
				Out = Size;
				return true;
			}
		}
		return false;
	}
}
