#pragma once

#include "CoreMinimal.h"
#include "Strongholds/SolidStronghold.h"

namespace SolidStronghold
{
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
		virtual ESolidStrongholdTrip Trip() const override;
		virtual void CollectGroundHexes(
			ESolidStrongholdSize Size,
			TArray<SolidStrongholdGrid::FHex>& OutHexes) const override;

		virtual TUniquePtr<FSolidStronghold> Clone() const override;
	};
}
