#pragma once

#include "CoreMinimal.h"

/**
 * The file name of one Play In Editor picture, and the HUD lines stored in that PNG.
 * The name is the build id, an underscore, and the local time.
 */
namespace SolidPlayshot
{
	/**
	 * Builds that file name from the build id, the time, and the names already in the folder.
	 * NamesForBuild receives the existing names that belong to BuildId.
	 */
	FString FileName(
		const FString& BuildId,
		const FDateTime& Time,
		const TArray<FString>& ExistingNames,
		TArray<FString>& NamesForBuild);

	/**
	 * Writes the HUD lines into a PNG as one iTXt chunk named SolidHUD.
	 * The picture bytes stay. False when the buffer is not a PNG.
	 */
	bool StampHud(TArray<uint8>& Png, const FString& Text);

	/** The SolidHUD text in that PNG. Empty when the chunk is absent. */
	FString ReadHud(const TArray<uint8>& Png);

	/** Loads that PNG, stamps the HUD lines, and writes it back. */
	bool StampHudFile(const FString& Path, const FString& Text);
}
