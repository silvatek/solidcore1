#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
	/**
	 * A rectangular fortress.
	 * Humble is the plan these fields describe. A later size stays unsupported until its mockup is drawn.
	 * The gate side of the plan is local north. Placement turns that side toward the grass.
	 */
	class FSolidFortress : public FSolidStronghold
	{
	public:
		FSolidFortress();

		static const FSolidFortress& Instance();

		/** Columns in the Keep. Humble is four, three rows, the medium room. */
		int32 KeepColumns = 4;
		/** Columns in the Annex. Humble is two, three rows, the small room. */
		int32 AnnexColumns = 2;
		int32 RoomRows = 3;
		/** Tower wall thickness. */
		float WallCm = 50.f;
		/** Open ground between the Annex's east outer face and the Keep's west outer face. */
		float GapCm = 200.f;
		/** Tower storey, walking surface to walking surface. */
		float StoreyCm = 300.f;
		/** One hex across, above the storey. */
		float ParapetCm = 120.f;
		/** Tower door width. The gate and each building doorway. */
		float OpeningCm = 140.f;
		/** Open ground from the buildings' north outer faces to the inner face of the gate wall. */
		float YardDepthCm = 300.f;

		virtual bool Supported(ESolidStrongholdSize InSize) const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize InSize,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		void CollectKeepHexes(
			ESolidStrongholdSize InSize,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const;
		void CollectAnnexHexes(
			ESolidStrongholdSize InSize,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const;

		/** Vertex span of Columns pointy-top columns. */
		float RoomWidthCm(int32 Columns) const;
		/** Vertex span of RoomRows pointy-top rows. */
		float RoomDepthCm() const;
		FVector2D AnnexInnerMinCm() const;
		FVector2D KeepInnerMinCm() const;
		float OuterWidthCm() const;
		float OuterDepthCm() const;
		/** Storey plus the parapet. */
		float CurtainHeightCm() const;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
