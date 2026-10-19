#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"

/**
 * Dungeon, Fortress, and Tower.
 * TypeOf returns the one instance of the subclass for an enum value.
 */
namespace SolidStronghold
{
	/** Meagre is the fourteen-hex cave. Larger dungeons are not built yet. */
	class FSolidDungeon : public FSolidStronghold
	{
	public:
		FSolidDungeon()
		{
			Type = ESolidStrongholdType::Dungeon;
		}

		static const FSolidDungeon& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;

		/** Front face width. The opening plus the stone on each side. */
		float FaceWidthCm() const;
		/** Front face height. The opening plus the stone above it. The rock is no taller. */
		float FaceHeightCm() const;
		/** How far the slope runs back from the front face to the ground. Twice the face height. */
		float SlopeRunCm() const;
		/**
		 * How far the front face sits in front of the cell center, along the exit.
		 * The face stays just inside the next cell. North and south use the taller cell.
		 * East and west use the shorter one. The actor yaw turns that into a world direction.
		 */
		float FaceForwardCm(ESolidStrongholdOrientation InOrientation) const;
		/** Centimetres in front of the face where the captain stands on leaving. */
		float LeaveStandCm() const;
		/** World position in front of the cell center, outside the exit. */
		FVector2D LeaveStandXY(FVector2D CellCenterXY, ESolidStrongholdOrientation InOrientation) const;

		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};

	/** No size is built yet. There is no visit and no ground floor. */
	class FSolidFortress : public FSolidStronghold
	{
	public:
		FSolidFortress()
		{
			Type = ESolidStrongholdType::Fortress;
		}

		static const FSolidFortress& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};

	/** Average through Imperious. The ground floor is the hexes the tower plan keeps. */
	class FSolidTower : public FSolidStronghold
	{
	public:
		FSolidTower()
		{
			Type = ESolidStrongholdType::Tower;
		}

		static const FSolidTower& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;
		virtual float ExteriorYawDeg(const USolidWorldMap* Map) const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};

	/** The subclass for this enum. The same instance on every call. */
	const FSolidStronghold& TypeOf(ESolidStrongholdType Type);
}
