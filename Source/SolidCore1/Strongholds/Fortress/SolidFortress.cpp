#include "SolidFortress.h"

namespace SolidStronghold
{
	const FSolidFortress& FSolidFortress::Instance()
	{
		static const FSolidFortress Fortress;
		return Fortress;
	}

	bool FSolidFortress::Supported(ESolidStrongholdSize) const
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
}
