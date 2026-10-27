#pragma once

#include "CoreMinimal.h"

/**
 * The file name of one Play In Editor picture.
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
}
