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
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};

	/** The subclass for this enum. The same instance on every call. */
	const FSolidStronghold& TypeOf(ESolidStrongholdType Type);
}
