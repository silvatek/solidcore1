#include "SolidStrongholdType.h"
#include "SolidTowerStronghold.h"

namespace SolidStronghold
{
	const FSolidDungeonType& FSolidDungeonType::Instance()
	{
		static const FSolidDungeonType Dungeon;
		return Dungeon;
	}

	bool FSolidDungeonType::Supported(const ESolidStrongholdSize Size) const
	{
		return Size == ESolidStrongholdSize::Meagre;
	}

	bool FSolidDungeonType::UsesCliffMouth() const
	{
		return true;
	}

	ESolidStrongholdTrip FSolidDungeonType::Trip() const
	{
		return ESolidStrongholdTrip::Cave;
	}

	void FSolidDungeonType::CollectGroundHexes(
		const ESolidStrongholdSize Size,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		if (!Supported(Size))
		{
			return;
		}
		SolidStrongholdGrid::CollectMeagreHexes(OutHexes);
	}

	const FSolidFortressType& FSolidFortressType::Instance()
	{
		static const FSolidFortressType Fortress;
		return Fortress;
	}

	bool FSolidFortressType::Supported(ESolidStrongholdSize) const
	{
		return false;
	}

	bool FSolidFortressType::UsesCliffMouth() const
	{
		return false;
	}

	ESolidStrongholdTrip FSolidFortressType::Trip() const
	{
		return ESolidStrongholdTrip::None;
	}

	void FSolidFortressType::CollectGroundHexes(
		ESolidStrongholdSize,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
	}

	const FSolidTowerType& FSolidTowerType::Instance()
	{
		static const FSolidTowerType Tower;
		return Tower;
	}

	bool FSolidTowerType::Supported(const ESolidStrongholdSize Size) const
	{
		return SolidTowerStronghold::IsDefinedSize(Size);
	}

	bool FSolidTowerType::UsesCliffMouth() const
	{
		return false;
	}

	ESolidStrongholdTrip FSolidTowerType::Trip() const
	{
		return ESolidStrongholdTrip::Tower;
	}

	void FSolidTowerType::CollectGroundHexes(
		const ESolidStrongholdSize Size,
		TArray<SolidStrongholdGrid::FHex>& OutHexes) const
	{
		OutHexes.Reset();
		SolidTowerStronghold::FPlan Plan;
		const FSolidStronghold Stronghold{ ESolidStrongholdType::Tower, Size };
		if (!SolidTowerStronghold::PlanFor(Stronghold, Plan))
		{
			return;
		}
		SolidTowerStronghold::CollectKeptHexes(Plan, 0, OutHexes);
	}

	const FSolidStrongholdType& TypeOf(const ESolidStrongholdType Type)
	{
		switch (Type)
		{
		case ESolidStrongholdType::Dungeon:
			return FSolidDungeonType::Instance();
		case ESolidStrongholdType::Fortress:
			return FSolidFortressType::Instance();
		case ESolidStrongholdType::Tower:
			return FSolidTowerType::Instance();
		default:
			checkNoEntry();
			return FSolidDungeonType::Instance();
		}
	}

	bool UsesCaveMouth(const FSolidStronghold& Stronghold)
	{
		return TypeOf(Stronghold.Type).UsesCliffMouth();
	}
}
