#pragma once

#include "CoreMinimal.h"

class UClass;
class UObject;

/**
 * Recorded Fab locations live in tools/fab-assets.json.
 * A lookup tries the recorded file, then the recorded folder, then a search.
 * A search that finds the asset writes the location back.
 */
enum class ESolidFabLookup : uint8
{
	RecordedFile,
	RecordedFolder,
	Search
};

namespace SolidFabManifest
{
	/** Recorded file wins when it is on disk. Otherwise the recorded folder, then a search. */
	inline ESolidFabLookup ChooseFabLookup(bool bRecordedFileExists, bool bRecordedFolderExists)
	{
		if (bRecordedFileExists)
		{
			return ESolidFabLookup::RecordedFile;
		}
		if (bRecordedFolderExists)
		{
			return ESolidFabLookup::RecordedFolder;
		}
		return ESolidFabLookup::Search;
	}

	/** Content/Viking/Mesh/SK_Viking.uasset -> /Game/Viking/Mesh/SK_Viking.SK_Viking */
	FString ContentFileToObjectPath(const FString& ProjectRelativeUasset);

	/** /Game/Viking/Mesh/SK_Viking.SK_Viking -> Content/Viking/Mesh/SK_Viking.uasset */
	FString ObjectPathToContentFile(const FString& ObjectPath);

	/** Content/Fab/Pack -> /Game/Fab/Pack */
	FString ContentFolderToPackagePath(const FString& ProjectRelativeFolder);

	FString ManifestPath();
	FString LoadManifest();

	/** False when the asset or the field is absent. An empty string is present. */
	bool ReadJsonStringField(
		const FString& Json, const TCHAR* AssetId, const TCHAR* Field, FString& OutValue);

	enum class EWriteResult : uint8
	{
		Unchanged,
		Updated,
		Missing
	};

	/** Replace one string field on an asset object. Does not touch other assets. */
	EWriteResult WriteJsonStringField(
		FString& Json, const TCHAR* AssetId, const TCHAR* Field, const FString& Value);

	bool ContentFileExists(const FString& ProjectRelativeUasset);
	bool ContentDirectoryExists(const FString& ProjectRelativeFolder);

	UObject* TryLoadContentFile(const FString& ProjectRelativeUasset, UClass* Class);

	/** Write resolvedPath and resolvedFolder (the file's parent) when they differ. */
	void RememberDiscoveredFile(const TCHAR* AssetId, const UObject* Asset);

	/** Write one field from an object's Content path. Used for the Viking file map. */
	void RememberNamedFile(const TCHAR* AssetId, const TCHAR* Field, const UObject* Asset);
}
