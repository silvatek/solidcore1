#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
	/** Which building this is. Humble has an Annex and a Keep. */
	enum class ESolidFortressBuilding : uint8
	{
		Annex,
		Keep,
	};

	/**
	 * One building in the yard.
	 * Its outer face may be the curtain. InnerMinCm is the south-west corner of the inner face.
	 */
	class FSolidFortressBuilding
	{
	public:
		ESolidFortressBuilding Kind = ESolidFortressBuilding::Annex;
		int32 Columns = 0;
		int32 Rows = 0;
		FVector2D InnerMinCm = FVector2D::ZeroVector;

		/** Vertex span of these columns. */
		float WidthCm() const;
		/** Vertex span of these rows. */
		float DepthCm() const;
		void CollectHexes(TArray<SolidStrongholdGrid::FHex>& OutHexes) const;
	};

	/**
	 * A rectangular fortress.
	 * Buildings run west to east along the south curtain. Humble is that list.
	 * A later size stays unsupported until its mockup is drawn.
	 * The gate side of the plan is local north. Placement turns that side toward the grass.
	 */
	class FSolidFortress : public FSolidStronghold
	{
	public:
		FSolidFortress();

		static const FSolidFortress& Instance();

		/** Tower wall thickness. */
		float WallCm = 50.f;
		/** Open ground between one building's east outer face and the next building's west outer face. */
		float GapCm = 200.f;
		/** Tower storey, walking surface to walking surface. */
		float StoreyCm = 300.f;
		/** One hex across, above the storey. */
		float ParapetCm = 120.f;
		/** Tower door width. The gate and each building doorway. */
		float OpeningCm = 140.f;
		/** Open ground from the buildings' north outer faces to the inner face of the gate wall. */
		float YardDepthCm = 300.f;
		/** West to east. Humble is the Annex, then the Keep. */
		TArray<FSolidFortressBuilding> Buildings;

		virtual bool Supported(ESolidStrongholdSize InSize) const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize InSize,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		const FSolidFortressBuilding* Building(ESolidFortressBuilding Kind) const;
		float OuterWidthCm() const;
		float OuterDepthCm() const;
		/** Storey plus the parapet. */
		float CurtainHeightCm() const;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
