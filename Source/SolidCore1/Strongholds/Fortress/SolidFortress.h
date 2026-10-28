#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
	/**
	 * One building in the yard.
	 * Its outer face may be the curtain. InnerMinCm is the south-west corner of the inner face.
	 * One building in the list is the keep. The others are places for furniture.
	 */
	class FSolidFortressBuilding
	{
	public:
		bool bKeep = false;
		int32 Columns = 0;
		int32 Rows = 0;
		FVector2D InnerMinCm = FVector2D::ZeroVector;
		/** Zero means the vertex span. A larger value is spare floor around the grid. */
		float InnerWidthCm = 0.f;
		/** Zero means the vertex span. A larger value is spare floor around the grid. */
		float InnerDepthCm = 0.f;

		/** Inner face across. The vertex span, or wider when there is spare floor. */
		float WidthCm() const;
		/** Inner face deep. The vertex span, or deeper when there is spare floor. */
		float DepthCm() const;
		/** Vertex span of these columns. */
		float SpanWidthCm() const;
		/** Vertex span of these rows. */
		float SpanDepthCm() const;
		void CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const;
	};

	/**
	 * A rectangular fortress.
	 * Humble's rooms run west to east along the south curtain.
	 * Average's rooms form an L. Average stays unsupported.
	 * The gate side of the plan is local north. Placement turns that side toward the grass.
	 */
	class FSolidFortress : public FSolidStronghold
	{
	public:
		FSolidFortress();

		static const FSolidFortress& Instance();

		/** Fills the rooms for this size. Humble is the row. Average is the L. */
		void Place(ESolidStrongholdSize InSize);

		/** Open ground between one building's east outer face and the next building's west outer face. */
		float GapCm = 0.f;
		/** One hex across, above the storey. */
		float ParapetCm = 120.f;
		/** Open ground from the buildings' north outer faces to the inner face of the gate wall. One storey. */
		float YardDepthCm = FSolidStronghold::StoreyCm;
		/** One of them is the keep. */
		TArray<FSolidFortressBuilding> Buildings;

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

		const FSolidFortressBuilding* Keep() const;
		float OuterWidthCm() const;
		float OuterDepthCm() const;
		/** One hex outside the gate, on the grass side. */
		FVector2D OutsideGateXY(const USolidWorldChart* Map) const;
		/** Storey plus the parapet. */
		float CurtainHeightCm() const;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
