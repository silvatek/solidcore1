#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SolidFabManifest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFabLookupOrderTest,
	"SolidCore1.Content.FabLookupOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFabLookupOrderTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("recorded file first"),
		SolidFabManifest::ChooseFabLookup(true, true) == ESolidFabLookup::RecordedFile);
	TestTrue(
		TEXT("recorded folder when the file is missing"),
		SolidFabManifest::ChooseFabLookup(false, true) == ESolidFabLookup::RecordedFolder);
	TestTrue(
		TEXT("search when nothing is recorded"),
		SolidFabManifest::ChooseFabLookup(false, false) == ESolidFabLookup::Search);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFabPathTest,
	"SolidCore1.Content.FabPath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFabPathTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("content file to object"),
		SolidFabManifest::ContentFileToObjectPath(TEXT("Content/Viking/Mesh/SK_Viking.uasset")),
		FString(TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")));
	TestEqual(
		TEXT("object to content file"),
		SolidFabManifest::ObjectPathToContentFile(TEXT("/Game/Viking/Mesh/SK_Viking.SK_Viking")),
		FString(TEXT("Content/Viking/Mesh/SK_Viking.uasset")));
	TestEqual(
		TEXT("package path to content file"),
		SolidFabManifest::ObjectPathToContentFile(TEXT("/Game/Fab/Pack/SK_Body")),
		FString(TEXT("Content/Fab/Pack/SK_Body.uasset")));
	TestTrue(
		TEXT("engine path is not content"),
		SolidFabManifest::ObjectPathToContentFile(TEXT("/Engine/BasicShapes/Cube.Cube")).IsEmpty());
	TestEqual(
		TEXT("folder to package"),
		SolidFabManifest::ContentFolderToPackagePath(TEXT("Content/Viking")),
		FString(TEXT("/Game/Viking")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidFabManifestRecordTest,
	"SolidCore1.Content.FabManifestRecord",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidFabManifestRecordTest::RunTest(const FString& Parameters)
{
	const FString Manifest = SolidFabManifest::LoadManifest();
	TestTrue(TEXT("manifest loads"), Manifest.Contains(TEXT("\"id\": \"viking\"")));
	TestTrue(TEXT("grass is listed"), Manifest.Contains(TEXT("\"id\": \"grass\"")));

	FString VikingMesh;
	TestTrue(
		TEXT("viking mesh field"),
		SolidFabManifest::ReadJsonStringField(Manifest, TEXT("viking"), TEXT("SK_Viking.uasset"), VikingMesh));
	TestEqual(
		TEXT("viking mesh recorded at the repo path"),
		VikingMesh,
		FString(TEXT("Content/Viking/Mesh/SK_Viking.uasset")));

	FString GrassFolder;
	TestTrue(
		TEXT("grass folder is present"),
		SolidFabManifest::ReadJsonStringField(Manifest, TEXT("grass"), TEXT("resolvedFolder"), GrassFolder));
	TestFalse(TEXT("grass folder is recorded"), GrassFolder.IsEmpty());

	FString Edited = Manifest;
	const SolidFabManifest::EWriteResult Wrote = SolidFabManifest::WriteJsonStringField(
		Edited, TEXT("grass"), TEXT("resolvedFolder"), TEXT("Content/Fab/GrassRecorded"));
	TestTrue(TEXT("write updates"), Wrote == SolidFabManifest::EWriteResult::Updated);

	FString Folder;
	TestTrue(
		TEXT("read back folder"),
		SolidFabManifest::ReadJsonStringField(Edited, TEXT("grass"), TEXT("resolvedFolder"), Folder));
	TestEqual(
		TEXT("folder value"),
		Folder,
		FString(TEXT("Content/Fab/GrassRecorded")));

	FString VikingAfter;
	SolidFabManifest::ReadJsonStringField(Edited, TEXT("viking"), TEXT("SK_Viking.uasset"), VikingAfter);
	TestEqual(TEXT("viking mesh is untouched"), VikingAfter, VikingMesh);
	TestTrue(TEXT("other text stays"), Edited.Contains(TEXT("NoblesseOblige-No.1")));
	TestTrue(
		TEXT("second write is unchanged"),
		SolidFabManifest::WriteJsonStringField(
			Edited, TEXT("grass"), TEXT("resolvedFolder"), TEXT("Content/Fab/GrassRecorded"))
			== SolidFabManifest::EWriteResult::Unchanged);
	TestTrue(
		TEXT("missing asset"),
		SolidFabManifest::WriteJsonStringField(Edited, TEXT("nobody"), TEXT("resolvedPath"), TEXT("Content/X.uasset"))
			== SolidFabManifest::EWriteResult::Missing);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
