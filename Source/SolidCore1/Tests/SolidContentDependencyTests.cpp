#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Animation/AnimSequence.h"
#include "UObject/SoftObjectPath.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace SolidContentTestPrivate
{
	static bool SoftObjectResolves(const TCHAR* Path)
	{
		const FSoftObjectPath Soft(Path);
		if (!Soft.IsValid())
		{
			return false;
		}
		return Soft.TryLoad() != nullptr;
	}

	static bool SoftClassResolves(const TCHAR* Path)
	{
		return LoadClass<UObject>(nullptr, Path) != nullptr;
	}

	/** True if at least one path in the null-terminated list resolves. */
	static bool AnyObjectResolves(const TCHAR* const* Paths)
	{
		for (int32 Index = 0; Paths[Index] != nullptr; ++Index)
		{
			if (SoftObjectResolves(Paths[Index]))
			{
				return true;
			}
		}
		return false;
	}

	static bool AnyClassResolves(const TCHAR* const* Paths)
	{
		for (int32 Index = 0; Paths[Index] != nullptr; ++Index)
		{
			if (SoftClassResolves(Paths[Index]))
			{
				return true;
			}
		}
		return false;
	}

	static bool FabGrassMaterialPresent()
	{
		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.SearchAllAssets(true);

		FARFilter Filter;
		Filter.PackagePaths.Add(FName(TEXT("/Game/Fab")));
		Filter.bRecursivePaths = true;
		Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> Assets;
		AssetRegistry.GetAssets(Filter, Assets);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Equals(TEXT("Mat_025_grass"), ESearchCase::IgnoreCase)
				&& !Asset.PackageName.ToString().Contains(TEXT("/StaticMeshes/")))
			{
				return Asset.GetAsset() != nullptr;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentEngineBasicShapesTest,
	"SolidCore1.Content.EngineBasicShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentEngineBasicShapesTest::RunTest(const FString& Parameters)
{
	// SolidTree / SolidMonolith
	TestTrue(TEXT("Engine Cube (monolith)"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Engine/BasicShapes/Cube.Cube")));
	TestTrue(TEXT("Engine Cylinder (tree trunk)"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	TestTrue(TEXT("Engine Cone (tree canopy)"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Engine/BasicShapes/Cone.Cone")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentSolidColorMaterialsTest,
	"SolidCore1.Content.SolidColorMaterials",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentSolidColorMaterialsTest::RunTest(const FString& Parameters)
{
	// Fog / tree / monolith / grass fallback — need at least one FlatCol parent.
	static const TCHAR* Parents[] = {
		TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"),
		TEXT("/Game/LevelPrototyping/Materials/MI_DefaultColorway.MI_DefaultColorway"),
		nullptr
	};
	TestTrue(TEXT("FlatCol or DefaultColorway material present"),
		SolidContentTestPrivate::AnyObjectResolves(Parents));

	// Prefer the primary parent the code tries first.
	if (!SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol")))
	{
		AddWarning(TEXT("M_FlatCol missing; code will use MI_DefaultColorway fallback."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentVikingRequiredTest,
	"SolidCore1.Content.VikingRequired",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentVikingRequiredTest::RunTest(const FString& Parameters)
{
	// Default player + companion path (bUseVikingVisuals=true).
	TestTrue(TEXT("SK_Viking mesh"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")));
	TestTrue(TEXT("Anim_Viking_idle1"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/Viking/Animations/Anim_Viking_idle1.Anim_Viking_idle1")));
	TestTrue(TEXT("Anim_Viking_walk"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/Viking/Animations/Anim_Viking_walk.Anim_Viking_walk")));
	TestTrue(TEXT("Anim_Viking_run"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/Viking/Animations/Anim_Viking_run.Anim_Viking_run")));
	TestTrue(TEXT("Anim_Viking_jump"),
		SolidContentTestPrivate::SoftObjectResolves(TEXT("/Game/Viking/Animations/Anim_Viking_jump.Anim_Viking_jump")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentTerrainGrassTest,
	"SolidCore1.Content.TerrainGrass",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentTerrainGrassTest::RunTest(const FString& Parameters)
{
	// Preferred: Fab Mat_025_grass. Fallback: FlatCol solid color (covered elsewhere).
	const bool bFab = SolidContentTestPrivate::FabGrassMaterialPresent();
	const bool bFlatCol = SolidContentTestPrivate::SoftObjectResolves(
		TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol"))
		|| SolidContentTestPrivate::SoftObjectResolves(
			TEXT("/Game/LevelPrototyping/Materials/MI_DefaultColorway.MI_DefaultColorway"));

	TestTrue(TEXT("Fab grass or FlatCol fallback available for terrain"), bFab || bFlatCol);
	if (!bFab)
	{
		AddWarning(TEXT("Fab Mat_025_grass not found under /Game/Fab; terrain will use FlatCol grass."));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentPawnBlueprintFallbackTest,
	"SolidCore1.Content.PawnBlueprintOrCpp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentPawnBlueprintFallbackTest::RunTest(const FString& Parameters)
{
	// GameMode tries these BP classes, then falls back to ASolidCharacter C++.
	static const TCHAR* PawnBPs[] = {
		TEXT("/Game/Characters/BP_SolidCharacter.BP_SolidCharacter_C"),
		TEXT("/Game/Characters/BP_SolidCore1Character.BP_SolidCore1Character_C"),
		TEXT("/Game/Blueprints/BP_SolidCharacter.BP_SolidCharacter_C"),
		TEXT("/Game/Blueprints/BP_SolidCore1Character.BP_SolidCore1Character_C"),
		TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C"),
		nullptr
	};

	const bool bHasBp = SolidContentTestPrivate::AnyClassResolves(PawnBPs);
	if (!bHasBp)
	{
		AddWarning(TEXT("No pawn Blueprint found; GameMode will use C++ ASolidCharacter (supported)."));
	}
	else
	{
		AddInfo(TEXT("At least one pawn Blueprint is available."));
	}

	UClass* SolidCharacterClass = LoadClass<APawn>(nullptr, TEXT("/Script/SolidCore1.SolidCharacter"));
	TestNotNull(TEXT("ASolidCharacter C++ class loadable"), SolidCharacterClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentGameModeBlueprintOrCppTest,
	"SolidCore1.Content.GameModeBlueprintOrCpp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentGameModeBlueprintOrCppTest::RunTest(const FString& Parameters)
{
	static const TCHAR* GameModeBPs[] = {
		TEXT("/Game/Characters/BP_SolidGameMode.BP_SolidGameMode_C"),
		TEXT("/Game/Characters/BP_SolidCore1GameMode.BP_SolidCore1GameMode_C"),
		nullptr
	};

	const bool bHasBp = SolidContentTestPrivate::AnyClassResolves(GameModeBPs);
	if (!bHasBp)
	{
		AddWarning(TEXT("No Solid GameMode Blueprint found; module will use C++ ASolidGameMode (supported)."));
	}

	UClass* SolidGameModeClass = LoadClass<AGameModeBase>(nullptr, TEXT("/Script/SolidCore1.SolidGameMode"));
	TestNotNull(TEXT("ASolidGameMode loadable"), SolidGameModeClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidContentMannequinFallbackTest,
	"SolidCore1.Content.MannequinFallbackOptional",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidContentMannequinFallbackTest::RunTest(const FString& Parameters)
{
	// Only needed if Viking fails / bUseVikingVisuals=false. Warn if entirely absent.
	static const TCHAR* Meshes[] = {
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn.SKM_Quinn"),
		nullptr
	};
	static const TCHAR* AnimBPs[] = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
		nullptr
	};

	const bool bMesh = SolidContentTestPrivate::AnyObjectResolves(Meshes);
	const bool bAnim = SolidContentTestPrivate::AnyClassResolves(AnimBPs);
	if (!bMesh || !bAnim)
	{
		AddWarning(TEXT("Epic mannequin fallback mesh/AnimBP missing (OK while Viking assets are present)."));
	}
	// Always pass — this is an optional dependency check with warnings only.
	TestTrue(TEXT("mannequin check completed"), true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
