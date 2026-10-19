#include "SolidTower.h"
#include "SolidGameState.h"
#include "SolidTowerExterior.h"
#include "SolidTowerInterior.h"
#include "SolidTowerStronghold.h"
#include "Strongholds/SolidStrongholds.h"

namespace SolidStronghold
{
	const FSolidTower& FSolidTower::Instance()
	{
		static const FSolidTower Tower;
		return Tower;
	}

	bool FSolidTower::Supported(const ESolidStrongholdSize InSize) const
	{
		return SolidTowerStronghold::IsDefinedSize(InSize);
	}

	ESolidStrongholdTrip FSolidTower::Trip() const
	{
		return ESolidStrongholdTrip::Tower;
	}

	bool FSolidTower::AcceptsEnter(const FGameState& State, const FIntPoint OpeningCell) const
	{
		const FSolidStronghold* Active = State.ActiveStronghold.Get();
		return Active
			&& Active->Type == ESolidStrongholdType::Tower
			&& Active->Cell == OpeningCell;
	}

	UClass* FSolidTower::ExteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidTowerExterior::StaticClass();
	}

	void FSolidTower::ConfigureExterior(AActor* Exterior) const
	{
		ASolidTowerExterior* Shell = Cast<ASolidTowerExterior>(Exterior);
		if (!Shell)
		{
			return;
		}
		SolidTowerStronghold::FPlan Plan;
		if (!SolidTowerStronghold::PlanFor(*this, Plan))
		{
			return;
		}
		Shell->SetPlan(Plan);
		Shell->SetStrongholdCell(Cell);
		Shell->BuildVisuals();
	}

	UClass* FSolidTower::InteriorClass() const
	{
		if (!Supported(Size))
		{
			return nullptr;
		}
		return ASolidTowerInterior::StaticClass();
	}

	void FSolidTower::ConfigureInterior(
		AActor* Interior,
		const TArray<SolidStrongholdFurniture::FUsed>& Furnishings) const
	{
		ASolidTowerInterior* Room = Cast<ASolidTowerInterior>(Interior);
		if (!Room)
		{
			return;
		}
		SolidTowerStronghold::FPlan Plan;
		if (!SolidTowerStronghold::PlanFor(*this, Plan))
		{
			Plan = SolidTowerStronghold::Imperious;
		}
		Room->SetStrongholdCell(Cell);
		Room->SetPlan(Plan);
		Room->BuildVisuals();
		Room->PlaceFurnishings(Furnishings);
	}

	float FSolidTower::ExteriorYawDeg(const USolidWorldMap* Map) const
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		return SolidTowerStronghold::ExteriorYawDeg(GrassYaw);
	}

	float FSolidTower::ExitReachCm() const
	{
		SolidTowerStronghold::FPlan Plan;
		if (!SolidTowerStronghold::PlanFor(*this, Plan))
		{
			return SolidTowerStronghold::DoorStandClearCm;
		}
		return Plan.GroundExteriorRadiusCm
			+ SolidTowerStronghold::DoorProudCm
			+ SolidTowerStronghold::DoorStandClearCm;
	}

	void FSolidTower::RememberExit(const USolidWorldMap* Map)
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		Orientation = OrientationFromYaw(GrassYaw);
	}

	void FSolidTower::CollectGroundHexes(
		const ESolidStrongholdSize InSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		SolidTowerStronghold::FPlan Plan;
		const FSolidStronghold Stronghold{ ESolidStrongholdType::Tower, InSize };
		if (!SolidTowerStronghold::PlanFor(Stronghold, Plan))
		{
			return;
		}
		SolidTowerStronghold::CollectKeptHexes(Plan, 0, OutHexes);
	}

	TUniquePtr<FSolidStronghold> FSolidTower::Clone() const
	{
		return MakeUnique<FSolidTower>(*this);
	}
}
