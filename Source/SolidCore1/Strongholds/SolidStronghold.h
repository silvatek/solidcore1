#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Furnishings/SolidStrongholdGrid.h"
#include "SolidTerrainTypes.h"
#include "Strongholds/Furnishings/SolidStrongholdFurniture.h"
#include "SolidStronghold.generated.h"

/** What the company base is. Dungeon is the first; Fortress and Tower come later. */
UENUM()
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
UENUM()
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

/**
 * One room size. Large, medium, and small.
 * Dungeon, Fortress, and Tower use these same rooms.
 */
enum class ESolidStrongholdRoom : uint8
{
	Large,
	Medium,
	Small,
};

/** One row on the site stone: a size and a type the player can take. */
struct FSolidStrongholdChoice
{
	ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
	ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
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
class ASolidStrongholdExterior;
struct FGameState;
class ASolidTerrainStreamer;
class USolidWorldMap;
class UWorld;
struct FActorSpawnParameters;

/**
 * A company base: one type, one size, a name, the map cell it stands on, and the way its exit faces.
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
		FVector2D InLocation = FVector2D::ZeroVector);

	virtual ~FSolidStronghold() = default;
	FSolidStronghold(const FSolidStronghold&) = default;
	FSolidStronghold& operator=(const FSolidStronghold&) = default;

	ESolidStrongholdType Type = ESolidStrongholdType::Dungeon;
	ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
	/**
	 * What the company calls this stronghold.
	 * A new one is "{size} {type}", such as "Average Fortress".
	 * The starting cave is "The Cave". A later story lets the player change it.
	 */
	FString Name;
	/** Map cell. X is the column and Y is the row, counting from 0. Prefer Location. */
	FIntPoint Cell = FIntPoint::ZeroValue;
	/** World centimetres of the cell center. Height is the ground under this point. */
	FVector2D Location = FVector2D::ZeroVector;
	/** Direction the exit faces. The starting cave faces north. */
	ESolidStrongholdOrientation Orientation = ESolidStrongholdOrientation::North;

	/** True when the current code can build this type at Size. */
	virtual bool Supported(ESolidStrongholdSize) const { return false; }

	/** Which trip this type takes. */
	virtual ESolidStrongholdTrip Trip() const { return ESolidStrongholdTrip::None; }

	/** True when a click on the opening at this map cell may enter. */
	virtual bool AcceptsEnter(const FGameState&, FIntPoint) const { return false; }

	/** Actor class for this stronghold's exterior. Null when this kind draws none. */
	virtual UClass* ExteriorClass() const { return nullptr; }

	/** Fill an actor of ExteriorClass() from this stronghold. */
	virtual void ConfigureExterior(AActor* Exterior) const {}

	/** Actor class for this stronghold's interior. Null when this kind has no visit. */
	virtual UClass* InteriorClass() const { return nullptr; }

	/**
	 * Fill an actor of InteriorClass() from this stronghold.
	 * Furnishings are the pieces the company carried in.
	 */
	virtual void ConfigureInterior(
		AActor*,
		const TArray<SolidStrongholdFurniture::FUsed>&) const
	{
	}

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

	/** Centimetres from the cell center to where the captain stands on leaving, along the exit. */
	virtual float ExitReachCm() const { return 250.f; }

	/** Centimetres across the exit that put that stand on the entrance. */
	virtual float ExitLateralCm() const { return 0.f; }

	/** World position just outside the entrance, along Orientation. */
	FVector2D ExitStandXY() const;

	/** Records the direction the exit faces when this type takes it from the map. */
	virtual void RememberExit(const USolidWorldMap* Map);

	/** After Make sets the size. A tower copies that size's measurements into its plan. */
	virtual void BindPlan() {}
};

namespace SolidStronghold
{
	/** An owned dungeon, fortress, or tower of this size, standing at WorldXY. */
	TUniquePtr<FSolidStronghold> Make(
		ESolidStrongholdType Type,
		ESolidStrongholdSize Size,
		FIntPoint Cell,
		FVector2D WorldXY);

	/** The subclass for this enum. The same instance on every call. */
	const FSolidStronghold& TypeOf(ESolidStrongholdType Type);

	/**
	 * Cell center on the shipped terrain, in world centimetres.
	 * The terrain is 513 points at 200 cm, centered on the origin, and the map is 128 by 64 cells.
	 */
	FVector2D WorldXY(FIntPoint Cell);

	/**
	 * Full size of one map cell along an exit, in centimetres.
	 * North and south use the taller cell. East and west use the shorter one.
	 */
	float CellExtentAlongExitCm(ESolidStrongholdOrientation Orientation);

	/**
	 * Spawns the exterior of each company stronghold.
	 * The stronghold names the actor, the world position, and the facing.
	 */
	void SpawnStrongholds(
		UWorld* World,
		ASolidTerrainStreamer* Streamer,
		const TArray<FSolidStronghold*>& Company,
		const FActorSpawnParameters& SpawnParams,
		TArray<TObjectPtr<ASolidStrongholdExterior>>& Exteriors);
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

	/** Usable hexes in one room of this size. A large room is 20, a medium room 12, a small room 6. */
	inline constexpr int32 RoomHexes(const ESolidStrongholdRoom Room)
	{
		switch (Room)
		{
		case ESolidStrongholdRoom::Large: return 20;
		case ESolidStrongholdRoom::Medium: return 12;
		case ESolidStrongholdRoom::Small: return 6;
		default: return 0;
		}
	}

	/**
	 * How many large, medium, and small rooms one stronghold size has.
	 * Dungeon, Fortress, and Tower at that size have these same rooms.
	 * The Meagre cave still uses its fourteen-cell floor. These counts do not replace that floor.
	 */
	struct FRooms
	{
		ESolidStrongholdSize Size = ESolidStrongholdSize::Meagre;
		int32 Large = 0;
		int32 Medium = 0;
		int32 Small = 0;
	};

	/** Rooms for each stronghold size, in the same order as Sizes. */
	inline constexpr FRooms RoomsBySize[] = {
		{ ESolidStrongholdSize::Meagre, 0, 1, 0 },
		{ ESolidStrongholdSize::Humble, 0, 1, 1 },
		{ ESolidStrongholdSize::Average, 0, 1, 2 },
		{ ESolidStrongholdSize::Impressive, 0, 3, 2 },
		{ ESolidStrongholdSize::Grand, 1, 4, 2 },
		{ ESolidStrongholdSize::Stately, 2, 4, 2 },
		{ ESolidStrongholdSize::Imperious, 3, 4, 3 },
	};

	static_assert(UE_ARRAY_COUNT(RoomsBySize) == SizeCount, "every stronghold size has rooms");

	/** Usable hexes in these rooms. */
	inline constexpr int32 Hexes(const FRooms& Rooms)
	{
		return Rooms.Large * RoomHexes(ESolidStrongholdRoom::Large)
			+ Rooms.Medium * RoomHexes(ESolidStrongholdRoom::Medium)
			+ Rooms.Small * RoomHexes(ESolidStrongholdRoom::Small);
	}

	/** The rooms for this stronghold size. A size with no row has none of them. */
	inline FRooms RoomsFor(const ESolidStrongholdSize Size)
	{
		for (const FRooms& Rooms : RoomsBySize)
		{
			if (Rooms.Size == Size)
			{
				return Rooms;
			}
		}
		FRooms None;
		None.Size = Size;
		return None;
	}

	/** WorldMap.txt row of that dungeon, counting from 1 at the top. */
	inline constexpr int32 StartingRow = 9;
	/** WorldMap.txt column of that dungeon, counting from 1 at the left. */
	inline constexpr int32 StartingColumn = 17;
	/** The new-game Meagre Dungeon. Later strongholds use "{size} {type}" until renamed. */
	inline const TCHAR* StartingStrongholdName = TEXT("The Cave");

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

	/** The cardinal an exit yaw names. 90 is north, 0 is east, -90 is south, 180 is west. */
	inline ESolidStrongholdOrientation OrientationFromYaw(const float YawDeg)
	{
		const float Yaw = FRotator::NormalizeAxis(YawDeg);
		if (FMath::IsNearlyEqual(Yaw, 90.f, 1.f))
		{
			return ESolidStrongholdOrientation::North;
		}
		if (FMath::IsNearlyEqual(Yaw, 0.f, 1.f))
		{
			return ESolidStrongholdOrientation::East;
		}
		if (FMath::IsNearlyEqual(Yaw, -90.f, 1.f))
		{
			return ESolidStrongholdOrientation::South;
		}
		return ESolidStrongholdOrientation::West;
	}

	/** Where ExitStronghold stands the captain: just outside the entrance, facing away. */
	struct FExitStand
	{
		FVector2D XY = FVector2D::ZeroVector;
		float YawDeg = 0.f;
		float PitchDeg = 0.f;
	};

	inline FExitStand ExitStand(const FSolidStronghold& Stronghold)
	{
		FExitStand Stand;
		const FVisitView Visit = VisitViewFor(Stronghold);
		Stand.YawDeg = LeaveYawDeg(OrientationYawDeg(Stronghold.Orientation), Visit.Leave);
		Stand.PitchDeg = Visit.Leave.PitchDeg;
		Stand.XY = Stronghold.ExitStandXY();
		return Stand;
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
	inline FString Label(const ESolidStrongholdSize Size, const ESolidStrongholdType Type)
	{
		return FString::Printf(TEXT("%s %s"), SizeName(Size), TypeName(Type));
	}

	inline FString Label(const FSolidStronghold& Stronghold)
	{
		return Label(Stronghold.Size, Stronghold.Type);
	}

	/** Second line of the entrance hover. */
	inline const TCHAR* EnterHoverPrompt = TEXT("Click to enter");

	/** Stronghold name, then "Click to enter" on the next line. */
	inline FString EnterHover(const FString& Name)
	{
		return Name + TEXT("\n") + EnterHoverPrompt;
	}

	/** Pointer box inside a stronghold. */
	inline const TCHAR* LeaveHover = TEXT("Leave stronghold");

	/** Journal line and click log for going in. The same words for every size and type. */
	inline const TCHAR* EnterLabel = TEXT("Enter stronghold");

	/** Journal line and click log for going out. The same words for every size and type. */
	inline const TCHAR* ExitLabel = TEXT("Exit stronghold");

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

/**
 * Where each stronghold meets the open world.
 * Pure logic — no actor spawning (safe for automation).
 * The dungeon interior is a separate visit; this only places the outside mouth.
 */
namespace SolidStrongholds
{
	/**
	 * How far the mouth stands from the shared cell edge, toward the grass (cm).
	 * Terrain points are 200 cm apart and the mountain shelf is the point on
	 * the cave side of that edge. 280 cm clears the slope so the sill sits on
	 * the grassland and the back of the frame meets the cliff.
	 */
	inline constexpr float FaceInsetCm = 280.f;

	inline constexpr float OpeningWidthCm = 380.f;
	inline constexpr float OpeningHeightCm = 420.f;
	inline constexpr float OpeningDepthCm = 240.f;
	/**
	 * Grass-facing face of the rock frame, in actor space (cm).
	 * Local +X faces the grass. The face sits toward the cliff so the actor
	 * origin, where the captain stands, is in front of the stone.
	 */
	inline constexpr float FrameFrontLocalX = -OpeningDepthCm * 0.5f;
	inline constexpr float FrameThicknessCm = 80.f;
	/** Black mouth sits this far behind the rock front so the faces do not z-fight. */
	inline constexpr float MouthRecessCm = 16.f;
	/** The dungeon mouth sits this far inside the facing cell border. */
	inline constexpr float MouthBorderGapCm = 120.f;
	/** Side gap so the mouth does not share a face with the jambs. */
	inline constexpr float MouthGapCm = 6.f;

	inline FLinearColor RockColor()
	{
		return FLinearColor(0.30f, 0.28f, 0.26f);
	}

	inline FLinearColor MouthColor()
	{
		return FLinearColor(0.015f, 0.012f, 0.01f);
	}

	struct FEntrance
	{
		/** Stronghold map cell. */
		FIntPoint Cell = FIntPoint::ZeroValue;
		/** Adjacent grassland the mouth faces. */
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		/** Midpoint of the shared edge. */
		FVector2D EdgeXY = FVector2D::ZeroVector;
		/** Spawn point on the grass side of that edge. */
		FVector2D WorldXY = FVector2D::ZeroVector;
		/** Local +X faces the grassland. */
		float FacingYawDeg = 0.f;
	};

	/**
	 * Grass face of one cell.
	 * The working door is the company's active stronghold.
	 * A previous stronghold uses the same face for its exterior mesh.
	 * False when the cell has no grassland neighbor or the world points fail.
	 */
	bool MakeEntrance(
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		FIntPoint Cell,
		FEntrance& OutEntrance);

	/** Grass neighbor of one map cell. North, then west, south, and east, matching MakeEntrance. */
	struct FGrassFace
	{
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		float FacingYawDeg = 0.f;
	};

	bool FindGrassFace(const USolidWorldMap* WorldMap, FIntPoint Cell, FGrassFace& Out);
}

/**
 * Moving the company from the stronghold it has to a site.
 * A click lists a size and a type when that size is unlocked and the subclass can build it.
 * While site restrictions apply, that size is also larger than the company stronghold.
 * The site marker does not pick the size. Choosing a row commits. Esc never reaches this.
 * The swap itself is immediate, so it does not show a progress bar.
 */
namespace SolidStrongholdTransition
{
	/** A pair the game will one day offer. Meagre Fortress, Meagre Tower, and Humble Tower are outside it. */
	bool IsFinalPair(const FSolidStronghold& Stronghold);

	/** A final pair that already has an interior. Meagre Dungeon, and an Average, Impressive, Grand, Stately, or Imperious tower. */
	bool IsBuilt(const FSolidStronghold& Stronghold);

	enum class EOffer : uint8
	{
		None,
		Choose,
	};

	/**
	 * Rows a click on a site stone may list.
	 * Each row is an unlocked size the subclass can build.
	 * While bStrongholdSiteRestrictions is set, that size is also larger than the company stronghold.
	 * None leaves the site as it is and does not ask.
	 */
	EOffer Consider(const FGameState& State, TArray<FSolidStrongholdChoice>& OutChoices);

	bool Offers(
		const TArray<FSolidStrongholdChoice>& Choices,
		ESolidStrongholdSize Size,
		ESolidStrongholdType Type);

	bool AreNeighbors(const SolidStrongholdGrid::FHex& A, const SolidStrongholdGrid::FHex& B);

	/**
	 * Put the carried pieces on kept cells of one floor.
	 * A bed takes two neighbors. The fireplace and the chest take one cell each.
	 */
	bool PlaceOnFloor(
		TArray<SolidStrongholdFurniture::FUsed>& Pieces,
		const TArray<SolidStrongholdGrid::FHex>& Hexes);

	/**
	 * The site becomes the company base. Furnishings move onto its ground floor.
	 * The stronghold the company leaves moves into PreviousStrongholds.
	 * The chest keeps its slots. The captain's pack stays with him.
	 * Returns false when the click would not have offered that size and type.
	 */
	bool Commit(
		FGameState& State,
		FIntPoint SiteCell,
		ESolidStrongholdSize Size,
		ESolidStrongholdType Type);

	/** The company chest is open on the inventory page while the captain is inside the base that holds it. */
	bool CompanyChestOpen(const FGameState& State, bool bInStronghold);
}
