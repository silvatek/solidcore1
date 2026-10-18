#pragma once

#include "CoreMinimal.h"
#include "Misc/Parse.h"
#include "SolidDungeonLevel.h"
#include "SolidDungeonPassage.h"
#include "SolidGameState.h"
#include "SolidTowerStronghold.h"
#include "SolidWorldMap.h"

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
		const bool bMain = Main != INDEX_NONE && IsTorch(Loadout.Worn[Main]);
		const bool bOff = Off != INDEX_NONE && IsTorch(Loadout.Worn[Off]);
		return bMain || bOff;
	}

	inline const TCHAR* EnterLabel = TEXT("Enter tunnels");
	inline const TCHAR* LeaveLabel = TEXT("Leave tunnels");
	inline const TCHAR* VisitValue = TEXT("Lair");
	inline const TCHAR* EndOptionKey = TEXT("LairEnd");

	struct FDefinition
	{
		const TCHAR* Name = TEXT("");
	};

	inline const FDefinition IglinTunnels = { TEXT("Iglin tunnels") };

	inline const FDefinition* FindByName(const FString& Name)
	{
		if (Name.Equals(IglinTunnels.Name, ESearchCase::IgnoreCase))
		{
			return &IglinTunnels;
		}
		return nullptr;
	}

	/**
	 * Floor plan. '#' is rock. '.' is open floor. An end marker is a mouth.
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
		return Upper == USolidWorldMap::XMarker || Upper == USolidWorldMap::YMarker;
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

	/** Midpoint of the doorway on the outer side of that mouth's cell. */
	inline FVector2D MouthMidpoint(const TCHAR End)
	{
		const FIntPoint Cell = CellOf(End);
		const FVector2D Center = CellCenter(Cell.X, Cell.Y);
		const float Side = FChar::ToUpper(End) == USolidWorldMap::XMarker ? -0.5f : 0.5f;
		return Center + FVector2D(Side * CellCm, 0.f);
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
		return FChar::ToUpper(End) == USolidWorldMap::XMarker ? 0.f : 180.f;
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
	inline bool TunnelsOpen(const FGameState& State)
	{
		const int32 Index = SolidEvents::FindIndexById(SolidEvents::IdDrivenFromVillage);
		return Index != INDEX_NONE && Index < SolidEvents::MaxEvents && State.bFired[Index];
	}

	inline FString EnterOptions(const TCHAR End)
	{
		return FString::Printf(
			TEXT("?%s=%s?%s=%c"),
			SolidDungeonLevel::OptionKey,
			VisitValue,
			EndOptionKey,
			FChar::ToUpper(End));
	}

	inline bool RequestsVisit(const FString& Options)
	{
		FString Value;
		if (!FParse::Value(*Options, *FString::Printf(TEXT("%s="), SolidDungeonLevel::OptionKey), Value))
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
	 * A LairEntrance ! for these tunnels is the interior X end, and a LairExit !
	 * is the interior Y end. The interior grid still uses those two letters.
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
		const USolidWorldMap* WorldMap,
		FVector2D WorldMinXY,
		FVector2D WorldMaxXY,
		TArray<FEntrance>& OutEntrances);

	/**
	 * A click on a closed mouth. Enter and leave are the same two actions as the cave.
	 * bLair marks a lair mouth. EndMarker names which end.
	 */
	struct FPassageHit
	{
		SolidDungeonPassage::EHit Action = SolidDungeonPassage::EHit::None;
		bool bLair = false;
		TCHAR EndMarker = 0;
		/** A tower door. TowerCell is the map cell of that door. */
		bool bTower = false;
		FIntPoint TowerCell = FIntPoint::ZeroValue;
	};

	inline FPassageHit HitOnMouth(const bool bLairMouth, const TCHAR EndMarker, const bool bInside)
	{
		FPassageHit Hit;
		Hit.bLair = bLairMouth;
		Hit.EndMarker = bLairMouth ? FChar::ToUpper(EndMarker) : 0;
		Hit.Action = bInside ? SolidDungeonPassage::EHit::Leave : SolidDungeonPassage::EHit::Enter;
		return Hit;
	}

	/** A tower door. The door stays open even while the cave is shut. */
	inline FPassageHit HitOnTower(const FIntPoint Cell, const bool bInside)
	{
		FPassageHit Hit;
		Hit.bTower = true;
		Hit.TowerCell = Cell;
		Hit.Action = bInside ? SolidDungeonPassage::EHit::Leave : SolidDungeonPassage::EHit::Enter;
		return Hit;
	}

	/** Enter is ignored while that place is shut. Leave stays available. A tower door stays open. */
	inline FPassageHit AllowPassage(const FPassageHit& Hit, const bool bCaveEnabled, const bool bTunnelsOpen)
	{
		if (Hit.Action != SolidDungeonPassage::EHit::Enter)
		{
			return Hit;
		}
		if (Hit.bTower)
		{
			return Hit;
		}
		if (Hit.bLair)
		{
			return bTunnelsOpen ? Hit : FPassageHit();
		}
		return bCaveEnabled ? Hit : FPassageHit();
	}

	inline const TCHAR* PassageTip(const FPassageHit& Hit)
	{
		if (Hit.Action == SolidDungeonPassage::EHit::None)
		{
			return TEXT("");
		}
		if (Hit.bTower)
		{
			return Hit.Action == SolidDungeonPassage::EHit::Enter
				? SolidTowerStronghold::EnterLabel
				: SolidTowerStronghold::LeaveLabel;
		}
		if (Hit.bLair)
		{
			return Hit.Action == SolidDungeonPassage::EHit::Enter ? EnterLabel : LeaveLabel;
		}
		return SolidDungeonPassage::TipText(Hit.Action);
	}
}
