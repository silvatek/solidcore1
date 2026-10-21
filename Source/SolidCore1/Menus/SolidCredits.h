#pragma once

#include "CoreMinimal.h"

/**
 * Credits copy shown from the main menu. F10 closes the page.
 * Keep Fab attribution in sync with tools/fab-assets.json.
 */
namespace SolidCredits
{
	inline constexpr const TCHAR* ProjectAuthor = TEXT("Silvatek");
	inline constexpr const TCHAR* BuiltWith = TEXT("Cursor + Grok");
	inline constexpr const TCHAR* ToggleHint = TEXT("F10 to close");

	// Unreal Engine EULA section 7(a). The product has credits, so both notices are required.
	inline constexpr const TCHAR* EngineNotice = TEXT("SolidCore1 uses Unreal® Engine.");
	// One sentence, split so the credits box stays on screen. Read in order.
	inline constexpr const TCHAR* EngineTrademark =
		TEXT("Unreal® is a trademark or registered trademark of Epic Games, Inc.");
	inline constexpr const TCHAR* EngineTrademarkPlace =
		TEXT("in the United States of America and elsewhere.");
	inline constexpr const TCHAR* EngineCopyright =
		TEXT("Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.");
	// EULA section 4(c): a distributed product disclaims Epic's warranties and liability for the engine.
	inline constexpr const TCHAR* EngineDisclaimer =
		TEXT("Epic disclaims all representations, warranties, conditions, and liabilities for Unreal Engine.");

	inline constexpr const TCHAR* CcByLicense = TEXT("https://creativecommons.org/licenses/by/4.0/");

	inline constexpr const TCHAR* VikingCreator = TEXT("Art.Hiraeth");
	inline constexpr const TCHAR* VikingLicense = TEXT("Fab Standard (Personal / Professional)");
	inline constexpr const TCHAR* VikingListing =
		TEXT("https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f");

	inline constexpr const TCHAR* GrassCreator = TEXT("NoblesseOblige-No.1");
	inline constexpr const TCHAR* GrassLicense = TEXT("CC-BY");
	inline constexpr const TCHAR* GrassListing =
		TEXT("https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a");
	// The Fab listing is a GLB. The project uses that art as a Unreal material.
	inline constexpr const TCHAR* GrassChanges =
		TEXT("Changed: Unreal material made from the Fab GLB.");

	void CollectLines(TArray<FString>& OutLines);
}
