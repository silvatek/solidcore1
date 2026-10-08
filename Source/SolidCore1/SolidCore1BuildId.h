#pragma once

#include "HAL/Platform.h"

/**
 * Bump SOLIDCORE1_BUILD_ID on every GitHub push so PIE screenshots can identify the build.
 * Format: SC1-NNNN (increment the number).
 * SOLIDCORE1_BUILD_NOTE is a short one-line summary of what this build changed.
 */
#ifndef SOLIDCORE1_BUILD_ID
#define SOLIDCORE1_BUILD_ID TEXT("SC1-0024")
#endif

#ifndef SOLIDCORE1_BUILD_NOTE
#define SOLIDCORE1_BUILD_NOTE TEXT("Quinn companion follows Manny")
#endif
