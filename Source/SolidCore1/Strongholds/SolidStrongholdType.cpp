#include "SolidStrongholdType.h"
#include "SolidDungeonExterior.h"
#include "SolidStrongholds.h"
#include "SolidTowerExterior.h"
#include "SolidTowerStronghold.h"

TUniquePtr<FSolidStronghold> FSolidStronghold::Clone() const
{
	return MakeUnique<FSolidStronghold>(*this);
}

float FSolidStronghold::ExteriorYawDeg(const USolidWorldMap*) const
{
	return SolidStronghold::OrientationYawDeg(Orientation);
}

namespace SolidStronghold
{
	const FSolidDungeon& FSolidDungeon::Instance()
	{
		static const FSolidDungeon Dungeon;
		return Dungeon;
	}

	bool FSolidDungeon::Supported(const ESolidStrongholdSize InSize) const
	{
		return InSize == ESolidStrongholdSize::Meagre;
	}

	bool FSolidDungeon::UsesCliffMouth() const
	{
		return false;
	}

	ESolidStrongholdTrip FSolidDungeon::Trip() const
	{
		return ESolidStrongholdTrip::Cave;
	}

	UClass* FSolidDungeon::ExteriorClass() const
	{
		if (!Supported(Size) || UsesCliffMouth())
		{
			return nullptr;
		}
		return ASolidDungeonExterior::StaticClass();
	}

	void FSolidDungeon::ConfigureExterior(AActor* Exterior) const
	{
		ASolidDungeonExterior* Rock = Cast<ASolidDungeonExterior>(Exterior);
		if (!Rock)
		{
			return;
		}
		Rock->SetFace(FaceWidthCm(), FaceHeightCm(), SlopeRunCm());
		Rock->BuildVisuals();
	}

	float FSolidDungeon::FaceWidthCm() const
	{
		return SolidStrongholds::OpeningWidthCm + 2.f * SolidStrongholds::FrameThicknessCm;
	}

	float FSolidDungeon::FaceHeightCm() const
	{
		return SolidStrongholds::OpeningHeightCm + SolidStrongholds::FrameThicknessCm;
	}

	float FSolidDungeon::SlopeRunCm() const
	{
		return 2.f * FaceHeightCm();
	}

	float FSolidDungeon::LeaveStandCm() const
	{
		return 250.f;
	}

	FVector2D FSolidDungeon::LeaveStandXY(
		const FVector2D CellCenterXY,
		const ESolidStrongholdOrientation InOrientation) const
	{
		const float YawRad = FMath::DegreesToRadians(SolidStronghold::OrientationYawDeg(InOrientation));
		return CellCenterXY + FVector2D(FMath::Cos(YawRad), FMath::Sin(YawRad)) * LeaveStandCm();
	}

	void FSolidDungeon::CollectGroundHexes(
		const ESolidStrongholdSize InSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(InSize))
		{
			return;
		}
		SolidStrongholdGrid::CollectMeagreHexes(OutHexes);
	}

	TUniquePtr<FSolidStronghold> FSolidDungeon::Clone() const
	{
		return MakeUnique<FSolidDungeon>(*this);
	}

	const FSolidFortress& FSolidFortress::Instance()
	{
		static const FSolidFortress Fortress;
		return Fortress;
	}

	bool FSolidFortress::Supported(ESolidStrongholdSize) const
	{
		return false;
	}

	bool FSolidFortress::UsesCliffMouth() const
	{
		return false;
	}

	ESolidStrongholdTrip FSolidFortress::Trip() const
	{
		return ESolidStrongholdTrip::None;
	}

	void FSolidFortress::CollectGroundHexes(
		ESolidStrongholdSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
	}

	TUniquePtr<FSolidStronghold> FSolidFortress::Clone() const
	{
		return MakeUnique<FSolidFortress>(*this);
	}

	const FSolidTower& FSolidTower::Instance()
	{
		static const FSolidTower Tower;
		return Tower;
	}

	bool FSolidTower::Supported(const ESolidStrongholdSize InSize) const
	{
		return SolidTowerStronghold::IsDefinedSize(InSize);
	}

	bool FSolidTower::UsesCliffMouth() const
	{
		return false;
	}

	ESolidStrongholdTrip FSolidTower::Trip() const
	{
		return ESolidStrongholdTrip::Tower;
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
		Shell->SetTowerCell(Cell);
		Shell->BuildVisuals();
	}

	float FSolidTower::ExteriorYawDeg(const USolidWorldMap* Map) const
	{
		SolidStrongholds::FGrassFace Face;
		const float GrassYaw = (Map && SolidStrongholds::FindGrassFace(Map, Cell, Face))
			? Face.FacingYawDeg
			: 0.f;
		return SolidTowerStronghold::ExteriorYawDeg(GrassYaw);
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

	TUniquePtr<FSolidStronghold> Make(
		const ESolidStrongholdType Type,
		const ESolidStrongholdSize Size,
		const FIntPoint Cell)
	{
		TUniquePtr<FSolidStronghold> Stronghold = TypeOf(Type).Clone();
		Stronghold->Type = Type;
		Stronghold->Size = Size;
		Stronghold->Cell = Cell;
		return Stronghold;
	}

	const FSolidStronghold& TypeOf(const ESolidStrongholdType Type)
	{
		switch (Type)
		{
		case ESolidStrongholdType::Dungeon:
			return FSolidDungeon::Instance();
		case ESolidStrongholdType::Fortress:
			return FSolidFortress::Instance();
		case ESolidStrongholdType::Tower:
			return FSolidTower::Instance();
		default:
			checkNoEntry();
			return FSolidDungeon::Instance();
		}
	}

	bool UsesCaveMouth(const FSolidStronghold& Stronghold)
	{
		return TypeOf(Stronghold.Type).UsesCliffMouth();
	}
}
