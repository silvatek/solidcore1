#pragma once

#include "CoreMinimal.h"

class UAnimSequence;
class USkeletalMesh;

/**
 * Resolve Fab-sourced Content without a manual folder move after Add to Project.
 * Viking and grass are found by asset name under /Game/Viking or /Game/Fab.
 * Companion packs are the folders Medieval_Assassin_Girl and Elf_Blacksmith
 * anywhere under /Game. Recorded paths in tools/fab-assets.json are tried first.
 */
namespace SolidContentLookup
{
	inline constexpr const TCHAR* VikingMeshName = TEXT("SK_Viking");
	inline constexpr const TCHAR* VikingIdleName = TEXT("Anim_Viking_idle1");
	inline constexpr const TCHAR* VikingWalkName = TEXT("Anim_Viking_walk");
	inline constexpr const TCHAR* VikingRunName = TEXT("Anim_Viking_run");
	inline constexpr const TCHAR* VikingJumpName = TEXT("Anim_Viking_jump");

	/** Content folder for the Assassin catalog token. */
	inline constexpr const TCHAR* AssassinContentFolder = TEXT("Medieval_Assassin_Girl");
	/** Content folder for the Blacksmith catalog token. */
	inline constexpr const TCHAR* BlacksmithContentFolder = TEXT("Elf_Blacksmith");

	/**
	 * Character body inside an Assassin or Blacksmith pack.
	 * SK_Mannequin.uasset beside it is the skeleton, not the mesh.
	 */
	inline constexpr const TCHAR* CompanionBodyRelativePath = TEXT("Meshes/base_mesh/SKM_base_mesh_ue.uasset");

	/** Unarmed locomotion in Medieval_Assassin_Girl/animation/assassin. */
	inline constexpr const TCHAR* AssassinIdleName = TEXT("anim_assassin_idle_1");
	inline constexpr const TCHAR* AssassinWalkName = TEXT("anim_assassin_Walk");
	inline constexpr const TCHAR* AssassinRunName = TEXT("anim_assassin_run");
	inline constexpr const TCHAR* AssassinJumpName = TEXT("anim_assassin_Jump");

	/** PackFolder/Meshes/base_mesh/SKM_base_mesh_ue.uasset */
	inline FString CompanionBodyFile(const FString& PackFolder)
	{
		FString Folder = PackFolder;
		Folder.ReplaceInline(TEXT("\\"), TEXT("/"));
		while (Folder.EndsWith(TEXT("/")))
		{
			Folder.LeftChopInline(1);
		}
		if (Folder.IsEmpty())
		{
			return FString();
		}
		return Folder + TEXT("/") + CompanionBodyRelativePath;
	}

	/** True for the body mesh path, as a Content file or a /Game object path. */
	inline bool IsCompanionBodyPath(const FString& Path)
	{
		FString Normal = Path;
		Normal.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Normal.EndsWith(TEXT("Meshes/base_mesh/SKM_base_mesh_ue.uasset"), ESearchCase::IgnoreCase)
			|| Normal.EndsWith(TEXT("Meshes/base_mesh/SKM_base_mesh_ue.SKM_base_mesh_ue"), ESearchCase::IgnoreCase)
			|| Normal.EndsWith(TEXT("/Meshes/base_mesh/SKM_base_mesh_ue"), ESearchCase::IgnoreCase);
	}

	/**
	 * Prefer /Game/Viking (repo layout), then /Game/Fab (Launcher Add to Project).
	 * Returns nullptr if neither copy is present.
	 */
	USkeletalMesh* FindVikingMesh();
	UAnimSequence* FindVikingClip(const TCHAR* AssetName);

	/**
	 * Folder that holds the pack for a companion mesh token.
	 * Assassin → Medieval_Assassin_Girl, Blacksmith → Elf_Blacksmith.
	 * Empty when the token is not one of those packs.
	 */
	inline const TCHAR* CompanionMeshFolder(const TCHAR* Token)
	{
		if (!Token || !Token[0])
		{
			return TEXT("");
		}
		if (FCString::Stricmp(Token, TEXT("Assassin")) == 0)
		{
			return AssassinContentFolder;
		}
		if (FCString::Stricmp(Token, TEXT("Blacksmith")) == 0)
		{
			return BlacksmithContentFolder;
		}
		return TEXT("");
	}

	/** True when PackageName has Folder as its own path segment. */
	inline bool PackageContainsFolder(const FString& PackageName, const TCHAR* Folder)
	{
		if (!Folder || !Folder[0] || PackageName.IsEmpty())
		{
			return false;
		}

		const FString Segment = FString(TEXT("/")) + Folder;
		int32 SearchFrom = 0;
		while (SearchFrom < PackageName.Len())
		{
			const int32 At = PackageName.Find(
				Segment, ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchFrom);
			if (At == INDEX_NONE)
			{
				return false;
			}
			const int32 After = At + Segment.Len();
			if (After >= PackageName.Len() || PackageName[After] == TEXT('/'))
			{
				return true;
			}
			SearchFrom = At + 1;
		}
		return false;
	}

	/**
	 * Higher is a better skeletal mesh inside a companion pack.
	 * SK_ wins, then a name that contains the token, then a shorter name.
	 */
	inline int32 CompanionMeshRank(const FString& AssetName, const TCHAR* Token)
	{
		int32 Rank = 0;
		if (AssetName.StartsWith(TEXT("SK_"), ESearchCase::IgnoreCase))
		{
			Rank += 100000;
		}
		if (Token && Token[0] && AssetName.Contains(Token, ESearchCase::IgnoreCase))
		{
			Rank += 10000;
		}
		Rank -= AssetName.Len();
		return Rank;
	}

	/** True when AssetName should replace BestName inside a companion pack. */
	inline bool IsBetterCompanionMesh(
		const FString& AssetName, const FString& BestName, const TCHAR* Token)
	{
		const int32 Rank = CompanionMeshRank(AssetName, Token);
		const int32 BestRank = CompanionMeshRank(BestName, Token);
		if (Rank != BestRank)
		{
			return Rank > BestRank;
		}
		return AssetName.Compare(BestName, ESearchCase::IgnoreCase) < 0;
	}

	/**
	 * Skeletal mesh for a companion catalog token.
	 * The body is Meshes/base_mesh/SKM_base_mesh_ue inside Medieval_Assassin_Girl or
	 * Elf_Blacksmith. A recorded file is used when it is that body. Otherwise
	 * the body is found and written back, then a shorter SK_ is the fallback.
	 */
	USkeletalMesh* FindSkeletalMeshByToken(const TCHAR* Token);

	/** Anim sequence with this name whose package sits inside PackFolder. */
	UAnimSequence* FindPackClip(const TCHAR* PackFolder, const TCHAR* AssetName);

	UAnimSequence* FindVikingIdle();
	UAnimSequence* FindVikingWalk();
	UAnimSequence* FindVikingRun();
	UAnimSequence* FindVikingJump();
}
