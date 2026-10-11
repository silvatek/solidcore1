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
		FString(TEXT("Content/Fab/Medieval_Assassin_Girl/Meshes/base_mesh/SK_Mannequin.uasset")));
	TestTrue(
		TEXT("content body path"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("Content/X/Elf_Blacksmith/Meshes/base_mesh/SK_Mannequin.uasset")));
	TestTrue(
		TEXT("object body path"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("/Game/X/Medieval_Assassin_Girl/Meshes/base_mesh/SK_Mannequin.SK_Mannequin")));
	TestFalse(
		TEXT("a shorter SK_ is not the body"),
		SolidContentLookup::IsCompanionBodyPath(
			TEXT("/Game/X/Medieval_Assassin_Girl/Props/SK_Dagger.SK_Dagger")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
