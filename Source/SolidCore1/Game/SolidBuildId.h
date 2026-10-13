#pragma once

#include "HAL/Platform.h"

/**
 * Bump SOLID_BUILD_ID on every GitHub push so PIE screenshots can identify the build.
 * Format: SC1-NNNN (increment the number).
 * SOLID_BUILD_NOTE is a short one-line summary of what this build changed.
 */
#ifndef SOLID_BUILD_ID
#define SOLID_BUILD_ID TEXT("SC1-0231")
#endif

#ifndef SOLID_BUILD_NOTE
#define SOLID_BUILD_NOTE TEXT("Companions have a rounder body, matte torso and leg colors, and a face.")
#endif
