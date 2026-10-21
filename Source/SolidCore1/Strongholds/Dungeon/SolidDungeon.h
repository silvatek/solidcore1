#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

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
		virtual ESolidStrongholdTrip Trip() const override;
		virtual bool AcceptsEnter(
			const USolidGameState& State,
			const FSolidStronghold* Active,
			FIntPoint OpeningCell) const override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;
		virtual UClass* InteriorClass() const override;
		virtual void ConfigureInterior(
			AActor* Interior,
			const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const override;

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
		/**
		 * Local +Y shift that centers the stone on this cell's terrain points.
		 * Cell edges fall on those points. The east and north edge points belong
		 * to this cell, and the west and south edge points belong to the neighbor,
		 * so the points in the cell sit half a step (100 cm) east and north of
		 * the geometric center. Across the exit, that is this offset.
		 */
		float ShelfLateralCm(ESolidStrongholdOrientation InOrientation) const;
		/** Centimetres in front of the face where the captain stands on leaving. */
		float LeaveStandCm() const;
		/** World position in front of the mouth, outside the exit. */
		FVector2D LeaveStandXY(FVector2D CellCenterXY, ESolidStrongholdOrientation InOrientation) const;

		virtual float ExitReachCm() const override;
		virtual float ExitLateralCm() const override;

		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
