#pragma once

#include "CoreMinimal.h"
#include "Strongholds/Furnishings/SolidFloorSign.h"
#include "Strongholds/Furnishings/SolidHandrail.h"
#include "Strongholds/SolidStronghold.h"

/**
 * A round tower, measured in the tower's own plan.
 * FPlan is one size. Which Imperious floors that size keeps is TowerBands.
 * Local +X runs in from the door. Yaw 0 looks into the tower, the same way a dungeon visit does.
 * Placement turns the whole tower so this door faces the grass.
 * The Meagre floor stays the fourteen-cell outline. This floor uses the same 1.2 m pointy-top grid.
 * Each floor may shift that grid by at most 30 cm, so more whole cells sit in the circle.
 */
namespace SolidStronghold
{
	/**
	 * One tower's measurements. Height, clear height, interior radii, and the stair run are functions of these fields,
	 * so the wall stays one thickness and the top floor's ceiling is the cap.
	 * FSolidTower keeps the plan for its size. A shaft that is not a placed tower still uses this struct.
	 */
	struct FPlan
	{
		int32 FloorCount = 10;
		/**
		 * Imperious floor index of this plan's ground.
		 * 0 is the bottom of the shaft. Average starts at 6, which is story floor 7.
		 */
		int32 SourceFloor = 0;
		/** Walking surface to the walking surface above. */
		float FloorPitchCm = 300.f;
		float SlabCm = 20.f;
		/** Exterior radius minus interior radius, at every height. */
		float WallCm = 50.f;

		float GroundExteriorRadiusCm = 500.f;
		float TopExteriorRadiusCm = 300.f;

		/** Same clear opening as the cave door. */
		float DoorWidthCm = 140.f;
		float DoorHeightCm = 240.f;

		/** The flight occupies this band inside the wall. */
		float StairWidthCm = 140.f;
		int32 StairRiserCount = 17;
		int32 StairTreadCount = 16;
		float StairTreadCm = 26.f;

		/** Out through the door, local -X. */
		float DoorAngleRad = PI;
		/** The first flight starts this far past the door center, then the spiral continues. */
		float StairPastDoorRad = 20.f * PI / 180.f;
	};

	/** Outside of the stone, ground to the flat cap. */
	float HeightCm(const FPlan& Plan);
	/** Clear room under the slab. */
	float ClearHeightCm(const FPlan& Plan);
	float GroundInteriorRadiusCm(const FPlan& Plan);
	float TopInteriorRadiusCm(const FPlan& Plan);
	float StairRunCm(const FPlan& Plan);
	float RiserCm(const FPlan& Plan);

	/** The tower visit. A size in TowerBands is one this code builds. The ground floor is that plan's hex tiles. */
	class FSolidTower : public FSolidStronghold
	{
	public:
		FSolidTower();

		static const FSolidTower& Instance();

		/** Measurements for Size. Make fills this when it sets the size. */
		const FPlan& Plan() const { return StoredPlan; }

		float HeightCm() const { return SolidStronghold::HeightCm(StoredPlan); }
		float ClearHeightCm() const { return SolidStronghold::ClearHeightCm(StoredPlan); }
		float GroundInteriorRadiusCm() const { return SolidStronghold::GroundInteriorRadiusCm(StoredPlan); }
		float TopInteriorRadiusCm() const { return SolidStronghold::TopInteriorRadiusCm(StoredPlan); }
		float StairRunCm() const { return SolidStronghold::StairRunCm(StoredPlan); }
		float RiserCm() const { return SolidStronghold::RiserCm(StoredPlan); }

		/** Walking surface of a floor. Floor 0 is the ground. */
		float FloorZCm(int32 Floor) const;

		/** Interior radius at a height on this tower. */
		float InteriorRadiusCm(float ZCm) const;

		/**
		 * Hex tiles on one floor, row then column.
		 * An Imperious floor drops the outermost cells that fit until it has its count.
		 * The tiles stay connected. The ground floor leaves out the cell by the door,
		 * so a piece there cannot block the exit and the stair.
		 * Stairs and landings stay empty. Any other plan still returns every cell that fits.
		 */
		void CollectFloorHexes(int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes) const;

		/** Exterior radius at a height on this tower. */
		float ExteriorRadiusCm(float ZCm) const;

		/** The ground floor only. */
		bool HasDoor(int32 Floor) const;

		/** Direction from the center out through the door. */
		FVector2D DoorOutward() const;

		/** Center of the door slab. Local -X, bottom on the ground, proud of the ground course. */
		FVector DoorSlabCenterCm() const;

		/**
		 * Floor-0 cell nearest the door.
		 * The captain arrives there, and yaw 0 looks into the tower.
		 * On the Imperious ground floor that cell is not a hex tile.
		 */
		FVector2D DoorHexCenter() const;

		bool FloorHexesConnect(
			const TArray<SolidStrongholdGrid::FHex>& Hexes,
			const TArray<bool>& Keep,
			int32 Alive) const;

		virtual void BindPlan() override;

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual bool AcceptsEnter(const FGameState& State, FIntPoint OpeningCell) const override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;
		virtual UClass* InteriorClass() const override;
		virtual void ConfigureInterior(
			AActor* Interior,
			const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const override;
		/**
		 * Grass-facing yaw points local +X at the grass, the cave-mouth convention.
		 * The tower door is local -X, so the exterior turns half a turn from that yaw.
		 * North grass (facing yaw 90) puts the door at exterior yaw -90.
		 */
		float ExteriorYawDeg(const float GrassFacingYawDeg) const;

		virtual float ExteriorYawDeg(const USolidWorldMap* Map) const override;

		/**
		 * Grass side of the door, just past the stone.
		 * Grass-facing yaw 90 is north, so the stand point adds +Y.
		 */
		FVector2D OutsideDoorXY(const FVector2D& CellCenterXY, const float GrassFacingYawDeg) const;

		/** On the grass, looking out from the door. The same yaw the mouth uses. */
		float OutsideDoorYawDeg(const float GrassFacingYawDeg) const;

		virtual float ExitReachCm() const override;
		virtual void RememberExit(const USolidWorldMap* Map) override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;

	private:
		/** Inside the ground floor, just in from the door. */
		FVector2D DoorApproachPoint() const;

		/** The shipped ten-floor shaft. A resized copy of those radii is not this tower. */
		bool IsImperiousPlan() const;

		/**
		 * Hex tiles on one Imperious floor. Code floor 0 is story floor 1, the ground.
		 * Floors 1–3 keep 20, floors 4–7 keep 12, floors 8–10 keep 6.
		 */
		int32 ImperiousFloorHexCount(int32 Floor) const;

		/** Two cells share a side when their centers are one flat width apart. */
		bool HexesShareSide(const SolidStrongholdGrid::FHex& A, const SolidStrongholdGrid::FHex& B) const;

		FPlan StoredPlan;
	};

	/**
	 * How far one floor's hex grid sits from the tower center, in centimetres.
	 * Shift[Floor] is that floor. Floor 0 is the band's ground. A zero keeps the floor centered.
	 * A band shorter than Imperious leaves the rest zero.
	 */
	struct FHexGridNudge
	{
		ESolidStrongholdSize Size = ESolidStrongholdSize::Average;
		FVector2D Shift[10] = {};
	};

	/**
	 * Average through Imperious, in the same order as TowerBands.
	 * At most 30 cm, and only where the shift keeps another whole cell.
	 */
	inline const FHexGridNudge HexGridNudge[] = {
		// Average stays centered.
		{
			ESolidStrongholdSize::Average,
			{ { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f } },
		},
		// Story floor 7 keeps 11 cells when centered. This shift keeps the twelfth the medium room needs.
		{
			ESolidStrongholdSize::Impressive,
			{ { 0.f, 0.f }, { 0.f, 0.f }, { -4.f, -8.f }, { 0.f, 0.f }, { 0.f, 0.f } },
		},
		// Story floor 7 keeps 11 cells when centered. This shift keeps the twelfth the medium room needs.
		{
			ESolidStrongholdSize::Grand,
			{
				{ 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f },
				{ -2.f, 7.f }, { 0.f, 0.f }, { 0.f, 0.f },
			},
		},
		// Story floor 7 already keeps 12 cells, so this band stays centered.
		{
			ESolidStrongholdSize::Stately,
			{
				{ 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f },
				{ 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f },
			},
		},
		{
			ESolidStrongholdSize::Imperious,
			{
				{ 24.f, 8.f }, { 0.f, 30.f }, { 24.f, -14.f }, { 0.f, 0.f }, { -14.f, -24.f },
				{ 0.f, 0.f }, { 0.f, 0.f }, { 24.f, -14.f }, { 16.f, -10.f }, { -22.f, 12.f },
			},
		},
	};

	/**
	 * Imperious floors one tower size keeps.
	 * FirstFloor and LastFloor count from 1 at the ground of that shaft. The door is on FirstFloor.
	 * Meagre and Humble have no row.
	 */
	struct FSolidTowerBand
	{
		ESolidStrongholdSize Size = ESolidStrongholdSize::Average;
		int32 FirstFloor = 1;
		int32 LastFloor = 1;
	};

	/** Average through Imperious, in size order. */
	inline constexpr FSolidTowerBand TowerBands[] = {
		{ ESolidStrongholdSize::Average, 7, 9 },
		{ ESolidStrongholdSize::Impressive, 5, 9 },
		{ ESolidStrongholdSize::Grand, 3, 9 },
		{ ESolidStrongholdSize::Stately, 2, 9 },
		{ ESolidStrongholdSize::Imperious, 1, 10 },
	};

	/** How many floors this band keeps. */
	inline constexpr int32 BandFloorCount(const FSolidTowerBand& Band)
	{
		return Band.LastFloor - Band.FirstFloor + 1;
	}

	/** Imperious floor index of this band's ground. 0 is story floor 1. */
	inline constexpr int32 BandSourceFloor(const FSolidTowerBand& Band)
	{
		return Band.FirstFloor - 1;
	}

	/** The floors for this size. False when that size has no tower. */
	inline bool FindTowerBand(const ESolidStrongholdSize Size, FSolidTowerBand& Out)
	{
		for (const FSolidTowerBand& Band : TowerBands)
		{
			if (Band.Size == Size)
			{
				Out = Band;
				return true;
			}
		}
		return false;
	}

	static_assert(
		UE_ARRAY_COUNT(HexGridNudge) == UE_ARRAY_COUNT(TowerBands),
		"each tower band has one hex grid nudge row");


	/** 0 at Average, 1 at Imperious. The three sizes between them land on 1/4, 1/2, and 3/4. */
	float SizeAlong(const ESolidStrongholdSize Size);
	/** A halfway count rounds up. The steps from 4 floors to 10 are 6, 7, and 9. */
	int32 LerpCount(const int32 From, const int32 To, const float Along);
	/** Whole-number fields round. The other fields blend in a straight line. */
	FPlan LerpPlan(const FPlan& From, const FPlan& To, const float Along);
	/** The old four-floor shaft. The floor bands do not use it. Imperious is still the high end of that step. */
	FPlan LegacyLowPlan();
	/**
	 * A contiguous run of the Imperious shaft. SourceFloor 0 is story floor 1.
	 * The door is this band's ground, and the flight starts just past that door.
	 */
	FPlan MakeFloorBand(const int32 SourceFloor, const int32 FloorCount);
	/** The band for this size, or the old size step when that size has no tower floors. */
	FPlan MakePlan(const ESolidStrongholdSize Size);
	/** Imperious floors 7–9. Three storeys, the door on the lowest of them. */
	extern const FPlan Average;
	/** Imperious floors 5–9. Five storeys, the door on floor 5. 15 m tall, 8.4 m across at the grass. */
	extern const FPlan Impressive;
	/** Imperious floors 3–9. Seven storeys, the door on floor 3. 21 m tall, 9.2 m across at the grass. */
	extern const FPlan Grand;
	/** Imperious floors 2–9. Eight storeys, the door on floor 2. 24 m tall, 9.6 m across at the grass. */
	extern const FPlan Stately;
	/** The shipped tower. Ten floors, 10 m across at the grass, 6 m across at the cap. */
	extern const FPlan Imperious;
	bool IsFloor(const FPlan& Plan, const int32 Floor);
	/** Walking surface of a floor. Floor 0 is the ground. The last floor is the top room. */
	float FloorZCm(const FPlan& Plan, const int32 Floor);
	float InteriorRadiusCm(const FPlan& Plan, const float ZCm);
	/** Depth of the dark door slab, along local X. */
	inline constexpr float DoorSlabDepthCm = 40.f;
	/** How far the outer face stands past the ground course. */
	inline constexpr float DoorProudCm = 8.f;
	struct FWedge
	{
		float StartRad = 0.f;
		float SweepRad = 0.f;
		/** False when this floor has no such opening. */
		bool bOpen = false;
	};
	/** Arc of the flight that leaves this floor. The top floor has none. */
	float FlightSweepRad(const FPlan& Plan, const int32 Floor);
	/**
	 * Flat run along the stair centerline where a flight arrives, before the next flight.
	 * About two metres. The same length on every tower size.
	 */
	inline constexpr float LandingRunCm = 200.f;
	/** Stairs arrive on this floor, so it has a landing. The ground flight has none of its own. */
	bool HasLanding(const FPlan& Plan, const int32 Floor);
	/** Arc of the landing at this floor's walking surface. Zero when this floor has no landing. */
	float LandingSweepRad(const FPlan& Plan, const int32 Floor);
	/**
	 * Where this floor's flight starts.
	 * Each landing sits between the flight below and the next flight, so a flight above the ground
	 * starts at the far end of that landing.
	 */
	float FlightStartRad(const FPlan& Plan, const int32 Floor);
	FWedge FlightWedge(const FPlan& Plan, const int32 Floor);
	/** The hole for the flight that arrives from the floor below. The ground floor has none. The landing is not part of the hole. */
	FWedge GapWedge(const FPlan& Plan, const int32 Floor);
	/** Ground is floor 1. The plan's floor index starts at 0. */
	int32 FloorNumber(const int32 Floor);
	/** How far above the walking surface the floor number sits. */
	inline constexpr float FloorSignLiftCm = 150.f;
	/** How far inside the curved wall the number's plane stands, so the plate stays in the room. */
	inline constexpr float FloorSignInsetCm = 32.f;
	/** Flat arc in the stair band after the arriving flight, at this floor's walking surface. */
	FWedge LandingWedge(const FPlan& Plan, const int32 Floor);
	/**
	 * The number on the wall beside this floor's landing.
	 * The number is this tower's own floor. Ground is 1, so the first landing reads 2.
	 * Absent when this floor has no landing. The text faces the room.
	 */
	SolidStronghold::FSolidFloorSign FloorSignFor(const FPlan& Plan, const int32 Floor);
	float WrapTau(const float Angle);
	bool AngleInSweep(const float Angle, const float StartRad, const float SweepRad);
	/** Inner edge of the stair band at this height. The flight and the hole meet the floor here. */
	float RailInnerRadiusCm(const FPlan& Plan, const float ZCm);
	/**
	 * Inner edge of the flight that leaves this floor, rising with the treads.
	 * It stops where the flight stops, so it does not cross the landing above.
	 */
	void CollectFlightRail(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStronghold::FSolidHandrail::FPiece>& Out);
	/**
	 * Inner edge of the arrival hole, at this floor's walking surface.
	 * The radial edge away from the landing is closed. The landing end stays open.
	 */
	void CollectHoleRail(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStronghold::FSolidHandrail::FPiece>& Out);
	/** True when the hex center meets the annular sector of a flight or a hole. */
	bool CenterMeetsWedge(
		const FPlan& Plan,
		const FVector2D& Center,
		const float InteriorRadius,
		const FWedge& Wedge);
	/**
	 * How far this floor's hex grid sits from the tower center.
	 * At most 30 cm, and only when the shift keeps another whole cell.
	 * A plan that is not one of the five tower sizes stays centered.
	 * The cells stay pointy-top. A rotation would pull the yellow lines apart.
	 */
	FVector2D GridNudgeCm(const FPlan& Plan, const int32 Floor);
	FVector2D CellCenter(const int32 Col, const int32 Row, const FVector2D& Origin);
	FVector2D CellCenter(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	/** Inclusive column and row limit that covers the widest interior circle of this plan. */
	int32 HexSpan(const FPlan& Plan);
	/** Every vertex lies inside the interior circle at this floor's walking surface. */
	bool HexFitsFloor(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin);
	bool HexFitsFloor(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	bool HexOnStair(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin);
	bool HexOnStair(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	bool HexOnGap(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin);
	bool HexOnGap(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	/** The landing is floored, and it is not a hex. The ground floor has none. */
	bool HexOnLanding(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin);
	bool HexOnLanding(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	/** A floor cell: inside the circle, and neither the stair, the hole, nor the landing. */
	bool HexKept(
		const FPlan& Plan,
		const int32 Floor,
		const int32 Col,
		const int32 Row,
		const FVector2D& Origin);
	bool HexKept(const FPlan& Plan, const int32 Floor, const int32 Col, const int32 Row);
	/**
	 * Kept cells of one floor, row then column, on the grid whose origin is Origin.
	 * The stair, the hole, and the landing are absent.
	 * These are the cells that fit. An Imperious floor draws fewer hex tiles than this.
	 */
	void CollectKeptHexes(
		const FPlan& Plan,
		const int32 Floor,
		TArray<SolidStrongholdGrid::FHex>& OutHexes,
		const FVector2D& Origin);
	/** Kept cells on this floor's shifted grid. */
	void CollectKeptHexes(const FPlan& Plan, const int32 Floor, TArray<SolidStrongholdGrid::FHex>& OutHexes);
	/** How far past the proud door the captain stands on the grass. Clears the capsule. */
	inline constexpr float DoorStandClearCm = 120.f;
}
