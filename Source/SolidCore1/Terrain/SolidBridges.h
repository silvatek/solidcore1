#pragma once

#include "SolidBridge.h"

/**
 * The shipped bridges. The world chart stores each marker's terrain point.
 * Chart coordinates are no longer used.
 */
namespace SolidBridge
{
	inline const FBridgeMarker Bridges[] = {
		FBridgeMarker(TEXT("Iglin Bridge"), 1800.f, EOrientation::EastWest, EInitialState::Broken),
		FBridgeMarker(TEXT("Jethan Bridge"), 1800.f, EOrientation::NorthSouth, EInitialState::Intact),
		FBridgeMarker(TEXT("Rathen Bridge"), 1800.f, EOrientation::NorthSouth, EInitialState::Intact),
	};

	inline constexpr int32 DefinitionCount = UE_ARRAY_COUNT(Bridges);
}
