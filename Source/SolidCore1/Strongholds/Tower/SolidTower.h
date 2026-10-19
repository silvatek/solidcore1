#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
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
		virtual ESolidStrongholdTrip Trip() const override;
		virtual bool AcceptsEnter(const FGameState& State, FIntPoint OpeningCell) const override;
		virtual UClass* ExteriorClass() const override;
		virtual void ConfigureExterior(AActor* Exterior) const override;
		virtual UClass* InteriorClass() const override;
		virtual void ConfigureInterior(
			AActor* Interior,
			const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const override;
		virtual float ExteriorYawDeg(const USolidWorldMap* Map) const override;
		virtual float ExitReachCm() const override;
		virtual void RememberExit(const USolidWorldMap* Map) override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
