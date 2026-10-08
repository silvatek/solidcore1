#pragma once

#include "HAL/Platform.h"

/**
 * Bump SOLIDCORE1_BUILD_ID on every GitHub push so PIE screenshots can identify the build.
 * Format: SC1-NNNN (increment the number).
 */
#ifndef SOLIDCORE1_BUILD_ID
#define SOLIDCORE1_BUILD_ID TEXT("SC1-0017")
#endif
