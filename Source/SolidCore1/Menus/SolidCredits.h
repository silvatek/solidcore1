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

	inline constexpr const TCHAR* VikingCreator = TEXT("Art.Hiraeth");
	inline constexpr const TCHAR* VikingLicense = TEXT("Fab Standard (Personal / Professional)");
	inline constexpr const TCHAR* VikingListing =
		TEXT("https://www.fab.com/listings/ca4ba583-8d90-4069-b51f-50e694530b2f");

	inline constexpr const TCHAR* GrassCreator = TEXT("NoblesseOblige-No.1");
	inline constexpr const TCHAR* GrassLicense = TEXT("CC-BY");
	inline constexpr const TCHAR* GrassListing =
		TEXT("https://www.fab.com/listings/94bfee39-8d7d-409c-89c9-40433550ee3a");

	void CollectLines(TArray<FString>& OutLines);
}
