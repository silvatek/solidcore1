#include "SolidFabManifest.h"
#include "SolidCore1.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	int32 FindFrom(const FString& Json, const TCHAR* Needle, int32 From)
	{
		if (!Needle || !Needle[0] || From < 0 || From >= Json.Len())
		{
			return INDEX_NONE;
		}
		const int32 At = Json.Mid(From).Find(Needle, ESearchCase::CaseSensitive);
		return At == INDEX_NONE ? INDEX_NONE : At + From;
	}

	bool FindAssetSpan(const FString& Json, const TCHAR* AssetId, int32& OutStart, int32& OutEnd)
	{
		if (!AssetId || !AssetId[0])
		{
			return false;
		}

		const FString QuotedId = FString::Printf(TEXT("\"%s\""), AssetId);
		int32 SearchFrom = 0;
		while (SearchFrom < Json.Len())
		{
			const int32 IdKey = FindFrom(Json, TEXT("\"id\""), SearchFrom);
			if (IdKey == INDEX_NONE)
			{
				return false;
			}

			int32 Index = IdKey + 4;
			while (Index < Json.Len() && FChar::IsWhitespace(Json[Index]))
			{
				++Index;
			}
			if (Index >= Json.Len() || Json[Index] != TEXT(':'))
			{
				SearchFrom = IdKey + 4;
				continue;
			}
			++Index;
			while (Index < Json.Len() && FChar::IsWhitespace(Json[Index]))
			{
				++Index;
			}
			if (!Json.Mid(Index).StartsWith(QuotedId))
			{
				SearchFrom = IdKey + 4;
				continue;
			}

			int32 Brace = INDEX_NONE;
			for (int32 Cursor = IdKey; Cursor >= 0; --Cursor)
			{
				if (Json[Cursor] == TEXT('{'))
				{
					Brace = Cursor;
					break;
				}
			}
			if (Brace == INDEX_NONE)
			{
				return false;
			}

			int32 Depth = 0;
			bool bInString = false;
			bool bEscape = false;
			for (int32 Cursor = Brace; Cursor < Json.Len(); ++Cursor)
			{
				const TCHAR Char = Json[Cursor];
				if (bInString)
				{
					if (bEscape)
					{
						bEscape = false;
						continue;
					}
					if (Char == TEXT('\\'))
					{
						bEscape = true;
						continue;
					}
					if (Char == TEXT('"'))
					{
						bInString = false;
					}
					continue;
				}
				if (Char == TEXT('"'))
				{
					bInString = true;
					continue;
				}
				if (Char == TEXT('{'))
				{
					++Depth;
				}
				else if (Char == TEXT('}'))
				{
					--Depth;
					if (Depth == 0)
					{
						OutStart = Brace;
						OutEnd = Cursor;
						return true;
					}
				}
			}
			return false;
		}
		return false;
	}

	bool ReadStringAt(const FString& Json, int32 KeyAt, int32 Limit, FString& OutValue, int32& OutValueStart, int32& OutValueEnd)
	{
		const int32 KeyEnd = FindFrom(Json, TEXT("\""), KeyAt + 1);
		if (KeyEnd == INDEX_NONE || KeyEnd >= Limit)
		{
			return false;
		}

		int32 Index = KeyEnd + 1;
		while (Index < Limit && FChar::IsWhitespace(Json[Index]))
		{
			++Index;
		}
		if (Index >= Limit || Json[Index] != TEXT(':'))
		{
			return false;
		}
		++Index;
		while (Index < Limit && FChar::IsWhitespace(Json[Index]))
		{
			++Index;
		}
		if (Index >= Limit || Json[Index] != TEXT('"'))
		{
			return false;
		}
		++Index;
		OutValueStart = Index;
		OutValue.Reset();
		while (Index < Limit)
		{
			const TCHAR Char = Json[Index];
			if (Char == TEXT('"'))
			{
				OutValueEnd = Index;
				return true;
			}
			if (Char == TEXT('\\') && Index + 1 < Limit)
			{
				OutValue.AppendChar(Json[Index + 1]);
				Index += 2;
				continue;
			}
			OutValue.AppendChar(Char);
			++Index;
		}
		return false;
	}

	FString EscapeManifestValue(const FString& Value)
	{
		FString Escaped;
		Escaped.Reserve(Value.Len());
		for (const TCHAR Char : Value)
		{
			if (Char == TEXT('\\') || Char == TEXT('"'))
			{
				Escaped.AppendChar(TEXT('\\'));
			}
			Escaped.AppendChar(Char);
		}
		return Escaped;
	}

	FString NormalizeContentPath(const FString& Path)
	{
		FString Normal = Path;
		Normal.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Normal.StartsWith(TEXT("/")))
		{
			Normal.RightChopInline(1);
		}
		while (Normal.EndsWith(TEXT("/")))
		{
			Normal.LeftChopInline(1);
		}
		return Normal;
	}

	FString ParentContentFolder(const FString& ProjectRelativeFile)
	{
		int32 Slash = INDEX_NONE;
		if (!ProjectRelativeFile.FindLastChar(TEXT('/'), Slash))
		{
			return FString();
		}
		return ProjectRelativeFile.Left(Slash);
	}

	bool ReadFieldInSpan(
		const FString& Json,
		int32 Start,
		int32 End,
		const TCHAR* Field,
		FString& OutValue,
		int32& OutValueStart,
		int32& OutValueEnd)
	{
		const FString Key = FString::Printf(TEXT("\"%s\""), Field);
		int32 From = Start;
		while (From < End)
		{
			const int32 At = FindFrom(Json, *Key, From);
			if (At == INDEX_NONE || At >= End)
			{
				return false;
			}
			if (ReadStringAt(Json, At, End, OutValue, OutValueStart, OutValueEnd))
			{
				return true;
			}
			From = At + Key.Len();
		}
		return false;
	}

	void AssignField(FString& Json, const TCHAR* AssetId, const TCHAR* Field, const FString& Value, bool& bChanged)
	{
		const SolidFabManifest::EWriteResult Result =
			SolidFabManifest::WriteJsonStringField(Json, AssetId, Field, Value);
		if (Result == SolidFabManifest::EWriteResult::Updated)
		{
			bChanged = true;
			UE_LOG(LogSolid, Warning, TEXT("Recorded Fab %s %s -> %s"), AssetId, Field, *Value);
		}
		else if (Result == SolidFabManifest::EWriteResult::Missing)
		{
			UE_LOG(LogSolid, Warning, TEXT("Fab manifest has no %s field on %s."), Field, AssetId);
		}
	}

	void StoreIfChanged(const FString& Json, bool bChanged)
	{
		if (!bChanged)
		{
			return;
		}
#if WITH_EDITOR
		const FString Path = SolidFabManifest::ManifestPath();
		if (Path.IsEmpty())
		{
			return;
		}
		if (!FFileHelper::SaveStringToFile(Json, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogSolid, Warning, TEXT("Could not write Fab manifest %s"), *Path);
		}
#endif
	}
}

FString SolidFabManifest::ContentFileToObjectPath(const FString& ProjectRelativeUasset)
{
	const FString Normal = NormalizeContentPath(ProjectRelativeUasset);
	if (!Normal.StartsWith(TEXT("Content/")) || !Normal.EndsWith(TEXT(".uasset")))
	{
		return FString();
	}

	FString Rel = Normal.Mid(8);
	Rel.LeftChopInline(7);
	if (Rel.IsEmpty())
	{
		return FString();
	}

	int32 Slash = INDEX_NONE;
	const FString Name = Rel.FindLastChar(TEXT('/'), Slash) ? Rel.Mid(Slash + 1) : Rel;
	if (Name.IsEmpty())
	{
		return FString();
	}
	return FString::Printf(TEXT("/Game/%s.%s"), *Rel, *Name);
}

FString SolidFabManifest::ObjectPathToContentFile(const FString& ObjectPath)
{
	FString Path = ObjectPath;
	if (!Path.StartsWith(TEXT("/Game/")))
	{
		return FString();
	}

	int32 Dot = INDEX_NONE;
	if (Path.FindLastChar(TEXT('.'), Dot))
	{
		const FString ObjectName = Path.Mid(Dot + 1);
		const FString PackageName = Path.Left(Dot);
		if (FPaths::GetCleanFilename(PackageName).Equals(ObjectName, ESearchCase::IgnoreCase))
		{
			Path = PackageName;
		}
	}

	Path.RightChopInline(6);
	return FString(TEXT("Content/")) + Path + TEXT(".uasset");
}

FString SolidFabManifest::ContentFolderToPackagePath(const FString& ProjectRelativeFolder)
{
	const FString Normal = NormalizeContentPath(ProjectRelativeFolder);
	if (!Normal.StartsWith(TEXT("Content/")) || Normal.Len() <= 8)
	{
		return FString();
	}
	return FString(TEXT("/Game/")) + Normal.Mid(8);
}

FString SolidFabManifest::ManifestPath()
{
	if (FPaths::ProjectDir().IsEmpty())
	{
		return FString();
	}
	return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("tools/fab-assets.json"));
}

FString SolidFabManifest::LoadManifest()
{
	const FString Path = ManifestPath();
	FString Json;
	if (Path.IsEmpty() || !FFileHelper::LoadFileToString(Json, *Path))
	{
		return FString();
	}
	return Json;
}

bool SolidFabManifest::ReadJsonStringField(
	const FString& Json, const TCHAR* AssetId, const TCHAR* Field, FString& OutValue)
{
	OutValue.Reset();
	if (!Field || !Field[0])
	{
		return false;
	}

	int32 Start = 0;
	int32 End = 0;
	if (!FindAssetSpan(Json, AssetId, Start, End))
	{
		return false;
	}

	int32 ValueStart = 0;
	int32 ValueEnd = 0;
	return ReadFieldInSpan(Json, Start, End, Field, OutValue, ValueStart, ValueEnd);
}

SolidFabManifest::EWriteResult SolidFabManifest::WriteJsonStringField(
	FString& Json, const TCHAR* AssetId, const TCHAR* Field, const FString& Value)
{
	if (!Field || !Field[0])
	{
		return EWriteResult::Missing;
	}

	int32 Start = 0;
	int32 End = 0;
	if (!FindAssetSpan(Json, AssetId, Start, End))
	{
		return EWriteResult::Missing;
	}

	FString Current;
	int32 ValueStart = 0;
	int32 ValueEnd = 0;
	if (!ReadFieldInSpan(Json, Start, End, Field, Current, ValueStart, ValueEnd))
	{
		return EWriteResult::Missing;
	}

	const FString Normal = NormalizeContentPath(Value);
	if (Current.Equals(Normal, ESearchCase::CaseSensitive))
	{
		return EWriteResult::Unchanged;
	}

	Json = Json.Left(ValueStart) + EscapeManifestValue(Normal) + Json.Mid(ValueEnd);
	return EWriteResult::Updated;
}

SolidFabManifest::EWriteResult SolidFabManifest::EnsureResolvedPath(
	FString& Json, const TCHAR* AssetId, const TCHAR* Field, const FString& Value)
{
	if (!AssetId || !AssetId[0] || !Field || !Field[0])
	{
		return EWriteResult::Missing;
	}

	const EWriteResult Existing = WriteJsonStringField(Json, AssetId, Field, Value);
	if (Existing != EWriteResult::Missing)
	{
		return Existing;
	}

	int32 Start = 0;
	int32 End = 0;
	if (!FindAssetSpan(Json, AssetId, Start, End))
	{
		return EWriteResult::Missing;
	}

	const FString Normal = NormalizeContentPath(Value);
	const FString Pair = FString::Printf(TEXT("\"%s\": \"%s\""), Field, *EscapeManifestValue(Normal));
	const int32 KeyAt = FindFrom(Json, TEXT("\"resolvedPaths\""), Start);
	if (KeyAt == INDEX_NONE || KeyAt >= End)
	{
		const FString Insert = FString::Printf(TEXT(",\n      \"resolvedPaths\": { %s }"), *Pair);
		Json = Json.Left(End) + Insert + Json.Mid(End);
		return EWriteResult::Updated;
	}

	int32 Brace = KeyAt;
	while (Brace < End && Json[Brace] != TEXT('{'))
	{
		++Brace;
	}
	if (Brace >= End || Json[Brace] != TEXT('{'))
	{
		return EWriteResult::Missing;
	}

	int32 Depth = 0;
	bool bInString = false;
	bool bEscape = false;
	int32 Close = INDEX_NONE;
	for (int32 Cursor = Brace; Cursor <= End; ++Cursor)
	{
		const TCHAR Char = Json[Cursor];
		if (bInString)
		{
			if (bEscape)
			{
				bEscape = false;
				continue;
			}
			if (Char == TEXT('\\'))
			{
				bEscape = true;
				continue;
			}
			if (Char == TEXT('"'))
			{
				bInString = false;
			}
			continue;
		}
		if (Char == TEXT('"'))
		{
			bInString = true;
			continue;
		}
		if (Char == TEXT('{'))
		{
			++Depth;
		}
		else if (Char == TEXT('}'))
		{
			--Depth;
			if (Depth == 0)
			{
				Close = Cursor;
				break;
			}
		}
	}
	if (Close == INDEX_NONE)
	{
		return EWriteResult::Missing;
	}

	bool bEmpty = true;
	for (int32 Cursor = Brace + 1; Cursor < Close; ++Cursor)
	{
		if (!FChar::IsWhitespace(Json[Cursor]))
		{
			bEmpty = false;
			break;
		}
	}

	const FString Insert = bEmpty
		? FString::Printf(TEXT(" %s "), *Pair)
		: FString::Printf(TEXT(", %s "), *Pair);
	Json = Json.Left(Close) + Insert + Json.Mid(Close);
	return EWriteResult::Updated;
}

void SolidFabManifest::CollectContentFiles(const FString& Json, const TCHAR* AssetId, TArray<FString>& OutFiles)
{
	OutFiles.Reset();
	int32 Start = 0;
	int32 End = 0;
	if (!FindAssetSpan(Json, AssetId, Start, End))
	{
		return;
	}

	bool bInString = false;
	bool bEscape = false;
	FString Current;
	for (int32 Cursor = Start; Cursor <= End; ++Cursor)
	{
		const TCHAR Char = Json[Cursor];
		if (bInString)
		{
			if (bEscape)
			{
				bEscape = false;
				Current.AppendChar(Char);
				continue;
			}
			if (Char == TEXT('\\'))
			{
				bEscape = true;
				continue;
			}
			if (Char == TEXT('"'))
			{
				bInString = false;
				if (Current.StartsWith(TEXT("Content/")) && Current.EndsWith(TEXT(".uasset")))
				{
					OutFiles.Add(Current);
				}
				Current.Reset();
				continue;
			}
			Current.AppendChar(Char);
			continue;
		}
		if (Char == TEXT('"'))
		{
			bInString = true;
			Current.Reset();
		}
	}
}

bool SolidFabManifest::ContentFileExists(const FString& ProjectRelativeUasset)
{
	if (ProjectRelativeUasset.IsEmpty() || FPaths::ProjectDir().IsEmpty())
	{
		return false;
	}
	const FString Full = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectRelativeUasset);
	return FPaths::FileExists(Full);
}

bool SolidFabManifest::ContentDirectoryExists(const FString& ProjectRelativeFolder)
{
	if (ProjectRelativeFolder.IsEmpty() || FPaths::ProjectDir().IsEmpty())
	{
		return false;
	}
	const FString Full = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / ProjectRelativeFolder);
	return FPaths::DirectoryExists(Full);
}

UObject* SolidFabManifest::TryLoadContentFile(const FString& ProjectRelativeUasset, UClass* Class)
{
	if (!Class || !ContentFileExists(ProjectRelativeUasset))
	{
		return nullptr;
	}
	const FString ObjectPath = ContentFileToObjectPath(ProjectRelativeUasset);
	if (ObjectPath.IsEmpty())
	{
		return nullptr;
	}
	return StaticLoadObject(Class, nullptr, *ObjectPath);
}

void SolidFabManifest::RememberDiscoveredFile(const TCHAR* AssetId, const UObject* Asset)
{
	if (!AssetId || !AssetId[0] || !Asset)
	{
		return;
	}
	const FString File = ObjectPathToContentFile(Asset->GetPathName());
	if (File.IsEmpty())
	{
		return;
	}

	FString Json = LoadManifest();
	bool bChanged = false;
	AssignField(Json, AssetId, TEXT("resolvedPath"), File, bChanged);
	const FString Folder = ParentContentFolder(File);
	if (!Folder.IsEmpty())
	{
		AssignField(Json, AssetId, TEXT("resolvedFolder"), Folder, bChanged);
	}
	StoreIfChanged(Json, bChanged);
}

void SolidFabManifest::RememberNamedFile(const TCHAR* AssetId, const TCHAR* Field, const UObject* Asset)
{
	if (!AssetId || !AssetId[0] || !Field || !Field[0] || !Asset)
	{
		return;
	}
	const FString File = ObjectPathToContentFile(Asset->GetPathName());
	if (File.IsEmpty())
	{
		return;
	}

	FString Json = LoadManifest();
	bool bChanged = false;
	AssignField(Json, AssetId, Field, File, bChanged);
	StoreIfChanged(Json, bChanged);
}

void SolidFabManifest::RememberResolvedPath(const TCHAR* AssetId, const UObject* Asset)
{
	if (!AssetId || !AssetId[0] || !Asset)
	{
		return;
	}
	const FString File = ObjectPathToContentFile(Asset->GetPathName());
	if (File.IsEmpty())
	{
		return;
	}

	const FString Field = FPaths::GetCleanFilename(File);
	FString Json = LoadManifest();
	const EWriteResult Result = EnsureResolvedPath(Json, AssetId, *Field, File);
	if (Result == EWriteResult::Updated)
	{
		UE_LOG(LogSolid, Warning, TEXT("Recorded Fab %s %s -> %s"), AssetId, *Field, *File);
	}
	StoreIfChanged(Json, Result == EWriteResult::Updated);
}
