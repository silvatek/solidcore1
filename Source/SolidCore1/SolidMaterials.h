#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;
class UObject;

/**
 * Shared FlatCol / DefaultColorway solid-color MID helpers.
 * Used by terrain grass fallback, fog opaque fallback, trees, and monolith.
 */
namespace SolidMaterials
{
	/**
	 * Load M_FlatCol (or MI_DefaultColorway), create a MID, set BaseColor + Roughness.
	 * @param Outer  Outer for the MID (usually the requesting actor).
	 * @param DebugName  Included in error logs when the parent material is missing.
	 */
	UMaterialInterface* CreateSolidColor(
		UObject* Outer,
		const FLinearColor& Color,
		const TCHAR* DebugName,
		float Roughness = 1.f);
}
