#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidContentLookup.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionMeshFolderTest,
	"SolidCore1.Content.CompanionMeshFolder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionMeshFolderTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Assassin folder"),
		FString(SolidContentLookup::CompanionMeshFolder(TEXT("Assassin"))),
		FString(SolidContentLookup::AssassinContentFolder));
	TestEqual(
		TEXT("Blacksmith folder"),
		FString(SolidContentLookup::CompanionMeshFolder(TEXT("Blacksmith"))),
		FString(SolidContentLookup::BlacksmithContentFolder));
	TestEqual(
		TEXT("folder names"),
		FString(SolidContentLookup::AssassinContentFolder),
		FString(TEXT("Medieval_Assassin_Girl")));
	TestEqual(
		TEXT("blacksmith folder name"),
		FString(SolidContentLookup::BlacksmithContentFolder),
		FString(TEXT("Elf_Blacksmith")));
	TestTrue(
		TEXT("Viking token has no pack folder"),
		SolidContentLookup::CompanionMeshFolder(TEXT("SK_Viking"))[0] == TEXT('\0'));

	TestTrue(
		TEXT("nested assassin folder"),
		SolidContentLookup::PackageContainsFolder(
			TEXT("/Game/Fab/Medieval_Assassin_Girl/Mesh/SK_Body"),
			TEXT("Medieval_Assassin_Girl")));
	TestTrue(
		TEXT("assassin folder at Content root"),
		SolidContentLookup::PackageContainsFolder(
			TEXT("/Game/Medieval_Assassin_Girl"),
			TEXT("Medieval_Assassin_Girl")));
	TestTrue(
		TEXT("blacksmith folder"),
		SolidContentLookup::PackageContainsFolder(
			TEXT("/Game/Characters/Elf_Blacksmith/SK_Elf"),
			TEXT("Elf_Blacksmith")));
	TestFalse(
		TEXT("longer folder name is not the pack"),
		SolidContentLookup::PackageContainsFolder(
			TEXT("/Game/Medieval_Assassin_GirlExtra/SK_Body"),
			TEXT("Medieval_Assassin_Girl")));
	TestFalse(
		TEXT("token substring is not the folder"),
		SolidContentLookup::PackageContainsFolder(
			TEXT("/Game/Fab/SuperAssassin/SK_Body"),
			TEXT("Medieval_Assassin_Girl")));

	TestTrue(
		TEXT("SK_ beats a name that contains the token"),
		SolidContentLookup::CompanionMeshRank(TEXT("SK_Body"), TEXT("Assassin"))
			> SolidContentLookup::CompanionMeshRank(TEXT("SM_Assassin"), TEXT("Assassin")));
	TestTrue(
		TEXT("token in the name beats a plain SK_"),
		SolidContentLookup::CompanionMeshRank(TEXT("SK_Assassin"), TEXT("Assassin"))
			> SolidContentLookup::CompanionMeshRank(TEXT("SK_Body"), TEXT("Assassin")));
	TestTrue(
		TEXT("shorter SK_ wins when the token is absent"),
		SolidContentLookup::IsBetterCompanionMesh(TEXT("SK_A"), TEXT("SK_Body"), TEXT("Assassin")));
	TestFalse(
		TEXT("equal rank keeps the earlier name"),
		SolidContentLookup::IsBetterCompanionMesh(TEXT("SK_B"), TEXT("SK_A"), TEXT("Assassin")));

	TestEqual(
		TEXT("body file under the pack"),
		SolidContentLookup::CompanionBodyFile(TEXT("Content/Fab/Medieval_Assassin_Girl")),
		FString(TEXT("Content/Fab/Medieval_Assassin_Girl/Meshes/base_mesh/SKM_base_mesh_ue.uasset")));
	TestEqual(
		TEXT("blacksmith body file"),
		SolidContentLookup::CompanionBodyFile(TEXT("Content/Elf_Blacksmith")),
		FString(TEXT("Content/Elf_Blacksmith/Mesh/SK_Elf_Blacksmith.uasset")));
	TestFalse(
		TEXT("assassin body path is not the blacksmith body"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/X/Elf_Blacksmith/Meshes/base_mesh/SKM_base_mesh_ue.uasset")));
	TestTrue(
		TEXT("blacksmith body path"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/Elf_Blacksmith/Mesh/SK_Elf_Blacksmith.uasset")));
	TestFalse(
		TEXT("blacksmith skeleton is not the body"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/Elf_Blacksmith/Mesh/SK_Elf_Blacksmith_Skeleton.uasset")));
	TestFalse(
		TEXT("separated blacksmith part is not the body"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/Elf_Blacksmith/Mesh/SeparatedMesh/Body/SK_body.uasset")));
	TestTrue(
		TEXT("object body path"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("/Game/X/Medieval_Assassin_Girl/Meshes/base_mesh/SKM_base_mesh_ue.SKM_base_mesh_ue")));
	TestFalse(
		TEXT("SK_Mannequin is the skeleton"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/X/Medieval_Assassin_Girl/Meshes/base_mesh/SK_Mannequin.uasset")));
	TestEqual(
		TEXT("assassin idle"),
		FString(SolidContentLookup::AssassinIdleName),
		FString(TEXT("anim_assassin_idle_2")));
	TestEqual(
		TEXT("assassin walk"),
		FString(SolidContentLookup::AssassinWalkName),
		FString(TEXT("anim_assassin_Walk")));
	TestEqual(
		TEXT("assassin run"),
		FString(SolidContentLookup::AssassinRunName),
		FString(TEXT("anim_assassin_run")));
	TestEqual(
		TEXT("assassin jump"),
		FString(SolidContentLookup::AssassinJumpName),
		FString(TEXT("anim_assassin_Jump")));

	TestFalse(
		TEXT("a shorter SKM_ is not the body"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("/Game/X/Medieval_Assassin_Girl/Meshes/mech_separate/SKM_bag.SKM_bag")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
