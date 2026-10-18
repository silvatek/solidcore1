#pragma once

#include "CoreMinimal.h"
#include "SolidStronghold.h"
#include "SolidStrongholdGrid.h"

/** The closed visit. Cave is the dungeon trip. Tower is the tower trip. None does not travel. */
enum class ESolidStrongholdTrip : uint8
{
	None,
	Cave,
	Tower,
};

/**
 * One stronghold type.
 * Dungeon, Fortress, and Tower are the subclasses.
 * These are plain C++ classes, not Unreal objects and not Blueprints.
 * TypeOf returns the one instance of the subclass for an enum value.
 */
namespace SolidStronghold
{
	class FSolidStrongholdType
	{
	public:
		virtual ~FSolidStrongholdType() = default;

		FSolidStrongholdType(const FSolidStrongholdType&) = delete;
		FSolidStrongholdType& operator=(const FSolidStrongholdType&) = delete;

		/** True when the current code can build this type at Size. */
		virtual bool Supported(ESolidStrongholdSize Size) const = 0;

		/** The cliff mouth. A tower and a fortress place their own exterior. */
		virtual bool UsesCliffMouth() const = 0;

		/** Which trip this type takes. */
		virtual ESolidStrongholdTrip Trip() const = 0;

		/**
		 * Ground-floor hexes this type keeps at Size.
		 * Empty when that size has no floor. A move onto an empty floor is refused.
		 */
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const = 0;

	protected:
		FSolidStrongholdType() = default;
	};

	/** Meagre is the fourteen-hex cave. Larger dungeons are not built yet. */
	class FSolidDungeonType : public FSolidStrongholdType
	{
	public:
		static const FSolidDungeonType& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;
	};

	/** No size is built yet. There is no visit and no ground floor. */
	class FSolidFortressType : public FSolidStrongholdType
	{
	public:
		static const FSolidFortressType& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;
	};

	/** Average through Imperious. The ground floor is the hexes the tower plan keeps. */
	class FSolidTowerType : public FSolidStrongholdType
	{
	public:
		static const FSolidTowerType& Instance();

		virtual bool Supported(ESolidStrongholdSize Size) const override;
		virtual bool UsesCliffMouth() const override;
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;
	};

	/** The subclass for this enum. The same instance on every call. */
	const FSolidStrongholdType& TypeOf(ESolidStrongholdType Type);
}
