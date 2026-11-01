#pragma once

#include "HAL/Platform.h"

/**
 * Bump SOLID_BUILD_ID when the product changes, so PIE screenshots can identify the build.
 * A documentation change or a test change does not bump it.
 * Format: SC1-NNNN (increment the number).
 * SOLID_BUILD_NOTE is a short one-line summary of what this build changed.
 */
#ifndef SOLID_BUILD_ID
#define SOLID_BUILD_ID TEXT("SC1-0867")
#endif

#ifndef SOLID_BUILD_NOTE
#define SOLID_BUILD_NOTE TEXT("Playshot text skips the key hints and stays with the panel hidden.")
#endif
