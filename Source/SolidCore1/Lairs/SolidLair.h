#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"
#include "SolidTravel.h"
#include "SolidGameState.h"
#include "SolidLairId.h"
#include "SolidWorldChart.h"

/**
 * Closed visits, apart from the open world. Monsters come later.
 * A lair has a floor plan and mouths on the world map. Each mouth uses
 * the cave's stone frame on its own cliff. A click on the dark opening
 * enters. A click inside leaves by that end. A mouth stays shut until
 * its lair is open.
 */
namespace SolidLair
{
	inline constexpr int32 MazeCols = 11;
	inline constexpr int32 MazeRows = 9;
	/** Corridor width. Wider than the captain. */
	inline constexpr float CellCm = 200.f;
	inline constexpr float RoomHeightCm = 320.f;
	inline constexpr float ShellCm = 48.f;
	inline constexpr float MouthHeightCm = 240.f;
	inline constexpr float DoorwayWidthCm = 160.f;
	/** A dim glow at each mouth. The corridor stays dark. */
	inline constexpr float MouthLightIntensity = 800.f;
	inline constexpr float MouthLightRadiusCm = 900.f;
	/** A held torch of wood and pitch. It lights the captain, not the maze. */
	inline constexpr float TorchLightIntensity = 4000.f;
	inline constexpr float TorchLightRadiusCm = 700.f;

	inline FLinearColor MouthLightColor()
	{
		return FLinearColor(1.f, 0.82f, 0.55f);
	}

	/** Orange flame. Pitch burns darker than a clean wood fire. */
	inline FLinearColor TorchLightColor()
	{
		return FLinearColor(1.f, 0.52f, 0.14f);
	}

	/**
	 * The torch sits inside the captain. His body must not cast a shadow in a lair,
	 * or the flame lights nothing. Outside, the hidden body still casts a shadow.
	 */
	inline bool BodyCastsShadow(const bool bInLair)
	{
		return !bInLair;
	}

	/** True when the main hand or the off hand holds a torch. The pack does not count. */
	inline bool HandsHoldTorch(const SolidInventory::FLoadout& Loadout)
	{
		const auto IsTorch = [](const TCHAR* Id)
		{
			return Id && FCString::Strcmp(Id, SolidItemType::IdTorch) == 0;
		};
		const int32 Main = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
		const int32 Off = SolidInventory::WornIndex(SolidInventory::ESlot::OffHand);
		const bool bMain = Main != INDEX_NONE && IsTorch(Loadout.Worn[Main].Id());
		const bool bOff = Off != INDEX_NONE && IsTorch(Loadout.Worn[Off].Id());
		return bMain || bOff;
	}

	inline const TCHAR* EnterLabel = TEXT("Enter tunnels");
	inline const TCHAR* LeaveLabel = TEXT("Leave tunnels");
	inline const TCHAR* VisitValue = TEXT("Lair");
	inline const TCHAR* EndOptionKey = TEXT("LairEnd");

	struct FDefinition
	{
		/** Eight hex digits. */
		const TCHAR* Id = TEXT("");
		const TCHAR* Name = TEXT("");
	};

	inline const FDefinition Definitions[] = {
		{ IdIglinTunnels, TEXT("Iglin tunnels") },
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Definitions);

	inline const FDefinition& IglinTunnels = Definitions[0];

	inline const FDefinition* FindById(const TCHAR* Id)
	{
		if (!Id || !Id[0])
		{
			return nullptr;
		}
		for (const FDefinition& Lair : Definitions)
		{
			if (Lair.Id && FCString::Strcmp(Lair.Id, Id) == 0)
			{
				return &Lair;
			}
		}
		return nullptr;
	}

	inline const FDefinition* FindByName(const FString& Name)
	{
		for (const FDefinition& Lair : Definitions)
		{
			if (Name.Equals(Lair.Name, ESearchCase::IgnoreCase))
			{
				return &Lair;
			}
		}
		return nullptr;
	}

	inline const TCHAR* NameFor(const TCHAR* Id)
	{
		const FDefinition* Lair = FindById(Id);
		return Lair ? Lair->Name : nullptr;
	}

	/** The Iglin Tunnels Entrance. */
	inline constexpr TCHAR Entrance = TEXT('X');
	/** The Iglin Tunnels Exit. */
	inline constexpr TCHAR Exit = TEXT('Y');

	/**
	 * Floor plan. '#' is rock. '.' is open floor. The Entrance and Exit cells are mouths.
	 * Every open cell can be reached from a mouth.
	 * The straight run is there for early testing.
	 */
	inline const TCHAR* Maze[MazeRows] = {
		TEXT("###########"),
		TEXT("#X.......Y#"),
		TEXT("#.#.#.###.#"),
		TEXT("#.#...#...#"),
		TEXT("#.###.#.#.#"),
		TEXT("#.....#.#.#"),
		TEXT("###.#...#.#"),
		TEXT("#....##.#.#"),
		TEXT("###########"),
	};

	inline bool IsEndMarker(const TCHAR Marker)
	{
		const TCHAR Upper = FChar::ToUpper(Marker);
		return Upper == Entrance || Upper == Exit;
	}

	inline TCHAR MazeAt(const int32 Col, const int32 Row)
	{
		if (Col < 0 || Row < 0 || Col >= MazeCols || Row >= MazeRows)
		{
			return TEXT('#');
		}
		return Maze[Row][Col];
	}

	inline bool IsOpenCell(const int32 Col, const int32 Row)
	{
		const TCHAR Marker = MazeAt(Col, Row);
		return Marker == TEXT('.') || IsEndMarker(Marker);
	}

	inline FIntPoint CellOf(const TCHAR Marker)
	{
		const TCHAR Upper = FChar::ToUpper(Marker);
		for (int32 Row = 0; Row < MazeRows; ++Row)
		{
			for (int32 Col = 0; Col < MazeCols; ++Col)
			{
				if (FChar::ToUpper(MazeAt(Col, Row)) == Upper)
				{
					return FIntPoint(Col, Row);
				}
			}
		}
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	inline FVector2D CellCenter(const int32 Col, const int32 Row)
	{
		return FVector2D((static_cast<float>(Col) + 0.5f) * CellCm, (static_cast<float>(Row) + 0.5f) * CellCm);
	}

	inline FVector SpawnLocal(const TCHAR End)
	{
		const FIntPoint Cell = CellOf(End);
		const FVector2D Center = CellCenter(Cell.X, Cell.Y);
		return FVector(Center.X, Center.Y, 0.f);
	}

	/** Yaw that looks from this mouth into the maze. */
	inline float SpawnYawDeg(const TCHAR End)
	{
		return FChar::ToUpper(End) == Entrance ? 0.f : 180.f;
	}

	inline bool ContainsLocal(const FVector& Point)
	{
		if (Point.Z < 0.f || Point.Z >= RoomHeightCm)
		{
			return false;
		}
		const int32 Col = FMath::FloorToInt(Point.X / CellCm);
		const int32 Row = FMath::FloorToInt(Point.Y / CellCm);
		return IsOpenCell(Col, Row);
	}

	inline FString VisitJournalText(const bool bEnter)
	{
		return FString::Printf(TEXT("%s %s"), bEnter ? TEXT("Enter") : TEXT("Exit"), IglinTunnels.Name);
	}

	/** True when a click on a lair mouth may enter. */
	inline bool TunnelsOpen(const USolidGameState& State)
	{
		return SolidEvents::IsLairEnabled(State, IdIglinTunnels);
	}

	inline FString EnterOptions(const TCHAR End)
	{
		return FString::Printf(
			TEXT("?%s=%s?%s=%c"),
			SolidTravel::OptionKey,
			VisitValue,
			EndOptionKey,
			FChar::ToUpper(End));
	}

	inline bool RequestsVisit(const FString& Options)
	{
		FString Value;
		if (!FParse::Value(*Options, *FString::Printf(TEXT("%s="), SolidTravel::OptionKey), Value))
		{
			return false;
		}
		int32 Query = INDEX_NONE;
		if (Value.FindChar(TEXT('?'), Query))
		{
			Value.LeftInline(Query);
		}
		return Value.Equals(VisitValue, ESearchCase::IgnoreCase);
	}

	inline TCHAR RequestedEnd(const FString& Options)
	{
		FString Value;
		if (!FParse::Value(*Options, *FString::Printf(TEXT("%s="), EndOptionKey), Value) || Value.IsEmpty())
		{
			return 0;
		}
		int32 Query = INDEX_NONE;
		if (Value.FindChar(TEXT('?'), Query))
		{
			Value.LeftInline(Query);
		}
		const TCHAR End = FChar::ToUpper(Value[0]);
		return IsEndMarker(End) ? End : 0;
	}

	/**
	 * One stone mouth where a lair cell meets grassland.
	 * Neighbors are tried north, east, south, then west. The first grassland wins.
	 * A LairEntrance ! for these tunnels is the Iglin Tunnels Entrance, and a
	 * LairExit ! is the Iglin Tunnels Exit.
	 */
	struct FEntrance
	{
		TCHAR EndMarker = 0;
		FIntPoint Cell = FIntPoint::ZeroValue;
		FIntPoint GrassCell = FIntPoint::ZeroValue;
		FVector2D EdgeXY = FVector2D::ZeroVector;
		FVector2D WorldXY = FVector2D::ZeroVector;
		float FacingYawDeg = 0.f;
	};

	int32 CollectEntrances(
		const USolidWorldChart* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FEntrance>& OutEntrances);

	/** What a click on a dark opening does. */
	enum class EHit : uint8
	{
		None,
		Enter,
		Leave,
	};

	/**
	 * A click on a closed mouth. Enter and leave are the same two actions.
	 * bLair marks a lair mouth. EndMarker names which end.
	 */
	struct FPassageHit
	{
		EHit Action = EHit::None;
		bool bLair = false;
		TCHAR EndMarker = 0;
		/** A stronghold opening. StrongholdCell is the map cell of that opening. */
		bool bStronghold = false;
		FIntPoint StrongholdCell = FIntPoint::ZeroValue;
	};

	inline FPassageHit HitOnMouth(const bool bLairMouth, const TCHAR EndMarker, const bool bInside)
	{
		FPassageHit Hit;
		Hit.bLair = bLairMouth;
		Hit.EndMarker = bLairMouth ? FChar::ToUpper(EndMarker) : 0;
		Hit.Action = bInside ? EHit::Leave : EHit::Enter;
		return Hit;
	}

	/** A stronghold opening. The company stronghold decides whether the click may enter. */
	inline FPassageHit HitOnStronghold(const FIntPoint Cell, const bool bInside)
	{
		FPassageHit Hit;
		Hit.bStronghold = true;
		Hit.StrongholdCell = Cell;
		Hit.Action = bInside ? EHit::Leave : EHit::Enter;
		return Hit;
	}

	/** A lair enter is ignored while the tunnels are shut. Leave stays available. A stronghold opening is left for the stronghold. */
	inline FPassageHit AllowPassage(const FPassageHit& Hit, const bool bTunnelsOpen)
	{
		if (Hit.Action != EHit::Enter || Hit.bStronghold || !Hit.bLair)
		{
			return Hit;
		}
		return bTunnelsOpen ? Hit : FPassageHit();
	}

	inline const TCHAR* PassageTip(const FPassageHit& Hit)
	{
		if (Hit.Action == EHit::None)
		{
			return TEXT("");
		}
		if (Hit.Action == EHit::Leave && !Hit.bLair)
		{
			return SolidStronghold::ExitLabel;
		}
		if (Hit.bLair)
		{
			return Hit.Action == EHit::Enter ? EnterLabel : LeaveLabel;
		}
		if (Hit.bStronghold)
		{
			return SolidStronghold::EnterLabel;
		}
		return TEXT("");
	}
}
