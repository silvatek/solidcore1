#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

#include <initializer_list>

namespace SolidStronghold
{
	/**
	 * One building in the yard.
	 * Its outer face may be the curtain. InnerMinCm is the south-west corner of the inner face.
	 * Floors start at the ground. Each floor is one room: small, medium, or large.
	 */
	class FSolidFortressBuilding
	{
	public:
		/** Ground floor first. Each entry is that floor's room. */
		TArray<ESolidStrongholdRoom> Floors;
		/** The doorway faces this way. */
		ESolidStrongholdOrientation Facing = ESolidStrongholdOrientation::NorthFace;
		FVector2D InnerMinCm = FVector2D::ZeroVector;
		/** Inner face across, in centimetres. */
		float InnerWidthCm = 0.f;
		/** Inner face deep, in centimetres. */
		float InnerDepthCm = 0.f;

		/** Inner face across. The footprint, or the hex grid when that is wider. */
		float WidthCm() const;
		/** Inner face deep. The footprint, or the hex grid when that is deeper. */
		float DepthCm() const;
		/** Storeys, plus the parapet on the top floor. */
		float HeightCm(float ParapetCm) const;
		/** Vertex span of the ground floor's columns. */
		float SpanWidthCm() const;
		/** Vertex span of the ground floor's rows. */
		float SpanDepthCm() const;
		/** Ground-floor columns. A small room is 2. A medium room is 4. */
		int32 Columns() const;
		/** Ground-floor rows. A small or medium room is 3. */
		int32 Rows() const;
		/** A second floor keeps a stair along the door wall. */
		bool HasStair() const;
		/**
		 * From the inner start of the door wall to the doorway centre.
		 * Zero centres the doorway on that face. A stair centres it on the landing.
		 */
		float DoorFromInnerCm() const;
		/** The ground floor. */
		void CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const;
		/** One floor of this building. Floor 0 is the ground. */
		void CollectFloorHexes(int32 FloorIndex, TArray<SolidStrongholdGrid::FHex>& OutHexes) const;
	};

	/**
	 * A rectangular fortress.
	 * AddBuildings lists the buildings for one size. Meagre stays unsupported.
	 * The gate side of the plan is local north. Placement turns that side toward the grass.
	 */
	class FSolidFortress : public FSolidStronghold
	{
	public:
		FSolidFortress();

		static const FSolidFortress& Instance();

		/** Fills the buildings for this size. A size with no calls has none. */
		void AddBuildings(ESolidStrongholdSize InSize);

		/**
		 * One building.
		 * PositionCm is the inner south-west corner. FootprintCm is the room block.
		 * Facing is the door wall. Floors start at the ground, and each entry is that floor's room.
		 * A second floor adds the stair along the door wall.
		 */
		void AddBuilding(
			FVector2D PositionCm,
			FVector2D FootprintCm,
			ESolidStrongholdOrientation Facing,
			std::initializer_list<ESolidStrongholdRoom> Floors);

		/** Open ground between one building's east outer face and the next building's west outer face. */
		float GapCm = 0.f;
		/** One hex across, above the storey. */
		float ParapetCm = 120.f;
		/** Open ground from the buildings' north outer faces to the inner face of the gate wall. */
		float YardDepthCm = FSolidStronghold::StoreyCm;
		/**
		 * Open ground from the buildings' east outer faces to the inner face of the east wall.
		 * Zero means the building's east wall is that curtain.
		 */
		float YardWidthCm = 0.f;
		/** The buildings for this size. */
		TArray<FSolidFortressBuilding> Buildings;

		virtual void BindPlan() override;
		virtual bool Supported(ESolidStrongholdSize InSize) const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual bool AcceptsEnter(
			const USolidGameState& State,
			const FSolidStronghold* Active,
			FIntPoint OpeningCell) const override;
		virtual float ExitReachCm() const override;
		virtual void RememberExit(const USolidWorldChart* Map) override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;
		virtual UClass* InteriorClass() const override;
		virtual void ConfigureInterior(AActor* Interior) const override;
		virtual float ExteriorYawDeg(const USolidWorldChart* Map) const override;
		/** Beside the gate, on the grass side of the curtain. */
		virtual FSolidNameSignPlace NameSign() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize InSize,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		float OuterWidthCm() const;
		float OuterDepthCm() const;
		/** One hex outside the gate, on the grass side. */
		FVector2D OutsideGateXY(const USolidWorldChart* Map) const;
		/** One storey plus the parapet. The gate stops under the parapet. */
		float CurtainHeightCm() const;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;

	private:
		/** Open ground north of the buildings, and east of them. A zero width keeps the east wall as curtain. */
		void AdditionalCourtyard(float DepthCm, float WidthCm);
		/** Inner south-west corner of the building immediately east of this one. */
		FVector2D EastOf(int32 BuildingIndex) const;
		/**
		 * A medium room block with more than one floor.
		 * The footprint fits a medium room. The stair is added along the door wall.
		 * Floors start at the ground, and each entry is that floor's room.
		 */
		void AddMediumHigh(
			FVector2D PositionCm,
			ESolidStrongholdOrientation Facing,
			std::initializer_list<ESolidStrongholdRoom> Floors);
		/**
		 * A large room block with more than one floor.
		 * The footprint fits a large room. The stair is added along the door wall.
		 * Floors start at the ground, and each entry is that floor's room.
		 */
		void AddLargeHigh(
			FVector2D PositionCm,
			ESolidStrongholdOrientation Facing,
			std::initializer_list<ESolidStrongholdRoom> Floors);
		/**
		 * A medium room on one floor. The size is the room, so there is no room list, and no stair.
		 */
		void AddMediumLow(FVector2D PositionCm, ESolidStrongholdOrientation Facing);
		/**
		 * A small room on one floor. The size is the room, so there is no room list, and no stair.
		 */
		void AddSmallLow(FVector2D PositionCm, ESolidStrongholdOrientation Facing);
	};
}
