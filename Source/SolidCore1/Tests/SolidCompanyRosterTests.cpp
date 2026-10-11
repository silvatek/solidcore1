#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include <initializer_list>
#include "Companion/SolidCompanionCharacter.h"
#include "Party/SolidCompany.h"
#include "Party/SolidParty.h"
#include "SolidClipLocomotion.h"
#include "SolidGameMode.h"
#include "UObject/SoftObjectPath.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	const TCHAR* ExpectedMeshName(const TCHAR* CompanionName)
	{
		if (FCString::Stricmp(CompanionName, TEXT("Alex")) == 0
			|| FCString::Stricmp(CompanionName, TEXT("Max")) == 0)
		{
			return TEXT("Blacksmith");
		}
		return TEXT("Assassin");
	}

	bool ReferencesMesh(const USolidCompany* Company, const FSolidCompanion& Companion, const TCHAR* MeshName)
	{
		if (!Company || !MeshName)
		{
			return false;
		}
		const FSolidCharacterMesh* CharacterMesh = Company->GetCharacterMesh(Companion.CharacterMeshIndex);
		return CharacterMesh && CharacterMesh->Name.Equals(MeshName, ESearchCase::IgnoreCase);
	}

	bool FieldedNamesAre(
		FAutomationTestBase& Test,
		USolidParty* Party,
		const int32 MaxCount,
		std::initializer_list<const TCHAR*> Expected)
	{
		TArray<FSolidCompanion> Fielded;
		Party->CollectFieldedCompanions(MaxCount, Fielded);
		if (Fielded.Num() != static_cast<int32>(Expected.size()))
		{
			Test.AddError(FString::Printf(
				TEXT("fielded %d, expected %d (max %d)"),
				Fielded.Num(), static_cast<int32>(Expected.size()), MaxCount));
			return false;
		}
		USolidCompany* Company = Party ? Party->GetCompany() : nullptr;
		int32 Index = 0;
		for (const TCHAR* Name : Expected)
		{
			const TCHAR* MeshName = ExpectedMeshName(Name);
			if (Fielded[Index].Name != Name || !ReferencesMesh(Company, Fielded[Index], MeshName))
			{
				Test.AddError(FString::Printf(
					TEXT("fielded[%d] is %s, expected %s on %s"),
					Index, *Fielded[Index].Name, Name, MeshName));
				return false;
			}
			++Index;
		}
		return true;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanyRosterTest,
	"SolidCore1.Company.Roster",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanyRosterTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	TestNotNull(TEXT("company"), Company);
	Company->InitializeDefaultCharacterMeshes();
	TestEqual(TEXT("three character meshes"), Company->GetCharacterMeshCount(), 3);
	Company->InitializeDefaultCharacterMeshes();
	TestEqual(TEXT("mesh catalog is idempotent"), Company->GetCharacterMeshCount(), 3);

	const FSolidCharacterMesh* Viking = Company->GetCharacterMesh(0);
	TestNotNull(TEXT("viking mesh"), Viking);
	if (Viking)
	{
		TestEqual(TEXT("mesh name"), Viking->Name, FString(TEXT("Viking")));
		TestTrue(TEXT("viking asset"),
			Viking->Mesh.ToSoftObjectPath() == FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));
		TestEqual(TEXT("viking token"), Viking->AssetToken, FString(TEXT("SK_Viking")));
	}
	const FSolidCharacterMesh* Assassin = Company->GetCharacterMesh(Company->FindCharacterMeshIndexByName(TEXT("Assassin")));
	const FSolidCharacterMesh* Blacksmith = Company->GetCharacterMesh(Company->FindCharacterMeshIndexByName(TEXT("Blacksmith")));
	TestNotNull(TEXT("assassin mesh"), Assassin);
	TestNotNull(TEXT("blacksmith mesh"), Blacksmith);
	if (Assassin)
	{
		TestTrue(TEXT("assassin mesh is found by token after Add to Project"), Assassin->Mesh.IsNull());
		TestEqual(TEXT("assassin token"), Assassin->AssetToken, FString(TEXT("Assassin")));
	}
	if (Blacksmith)
	{
		TestTrue(TEXT("blacksmith mesh is found by token after Add to Project"), Blacksmith->Mesh.IsNull());
		TestEqual(TEXT("blacksmith token"), Blacksmith->AssetToken, FString(TEXT("Blacksmith")));
	}
	TestEqual(TEXT("find viking"), Company->FindCharacterMeshIndexByName(TEXT("viking")), 0);
	TestEqual(TEXT("unknown mesh"), Company->FindCharacterMeshIndexByName(TEXT("Shield")), INDEX_NONE);
	TestNull(TEXT("mesh out of range"), Company->GetCharacterMesh(99));

	FSolidCharacterMesh Extra;
	Extra.Name = TEXT("Extra");
	Extra.Mesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Viking/Mesh/SK_Other.SK_Other")));
	const int32 ExtraIndex = Company->AddCharacterMesh(Extra);
	TestEqual(TEXT("appended mesh"), ExtraIndex, 3);
	TestEqual(TEXT("four meshes"), Company->GetCharacterMeshCount(), 4);

	Company->InitializeDefaultCompanions();
	TestEqual(TEXT("five companions"), Company->GetCompanionCount(), 5);
	Company->InitializeDefaultCompanions();
	TestEqual(TEXT("idempotent init"), Company->GetCompanionCount(), 5);
	TestEqual(TEXT("companion init does not reseed meshes"), Company->GetCharacterMeshCount(), 4);

	const TCHAR* Names[] = {
		TEXT("Sam"),
		TEXT("Alex"),
		TEXT("Max"),
		TEXT("Morgan"),
		TEXT("Quinn"),
	};
	const ESolidVocation Vocations[] = {
		ESolidVocation::Mage,
		ESolidVocation::Warrior,
		ESolidVocation::Cleric,
		ESolidVocation::Rogue,
		ESolidVocation::Warrior,
	};
	const TCHAR* Meshes[] = {
		TEXT("Assassin"),
		TEXT("Blacksmith"),
		TEXT("Blacksmith"),
		TEXT("Assassin"),
		TEXT("Assassin"),
	};
	static_assert(UE_ARRAY_COUNT(Names) == UE_ARRAY_COUNT(Vocations), "Vocation per companion");
	static_assert(UE_ARRAY_COUNT(Names) == UE_ARRAY_COUNT(Meshes), "Mesh per companion");
	bool bSeenWarrior = false;
	bool bSeenRogue = false;
	bool bSeenMage = false;
	bool bSeenCleric = false;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		const FSolidCompanion* Companion = Company->GetCompanion(Index);
		TestNotNull(*FString::Printf(TEXT("companion %d"), Index), Companion);
		if (!Companion)
		{
			continue;
		}
		TestEqual(
			*FString::Printf(TEXT("name %d"), Index),
			Companion->Name,
			FString(Names[Index]));
		TestEqual(
			*FString::Printf(TEXT("mesh %d"), Index),
			Companion->CharacterMeshIndex,
			Company->FindCharacterMeshIndexByName(Meshes[Index]));
		TestTrue(
			*FString::Printf(TEXT("references %s"), Meshes[Index]),
			ReferencesMesh(Company, *Companion, Meshes[Index]));
		TestFalse(
			*FString::Printf(TEXT("not the captain mesh %d"), Index),
			ReferencesMesh(Company, *Companion, TEXT("Viking")));
		TestTrue(
			*FString::Printf(TEXT("vocation %d"), Index),
			Companion->Vocation == Vocations[Index]);
		TestEqual(
			*FString::Printf(TEXT("find %s"), Names[Index]),
			Company->FindCompanionIndexByName(Names[Index]),
			Index);
		bSeenWarrior |= Companion->Vocation == ESolidVocation::Warrior;
		bSeenRogue |= Companion->Vocation == ESolidVocation::Rogue;
		bSeenMage |= Companion->Vocation == ESolidVocation::Mage;
		bSeenCleric |= Companion->Vocation == ESolidVocation::Cleric;
	}
	TestTrue(TEXT("a companion is a warrior"), bSeenWarrior);
	TestTrue(TEXT("a companion is a rogue"), bSeenRogue);
	TestTrue(TEXT("a companion is a mage"), bSeenMage);
	TestTrue(TEXT("a companion is a cleric"), bSeenCleric);

	TestEqual(TEXT("find ignores case"), Company->FindCompanionIndexByName(TEXT("morgan")), 3);
	TestEqual(TEXT("unknown name"), Company->FindCompanionIndexByName(TEXT("Nobody")), INDEX_NONE);
	TestNull(TEXT("out of range"), Company->GetCompanion(99));
	TestNull(TEXT("negative index"), Company->GetCompanion(-1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartyCompanionSelectionTest,
	"SolidCore1.Company.PartySelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartyCompanionSelectionTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidParty* Party = NewObject<USolidParty>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("party"), Party);

	USolidParty* Bare = NewObject<USolidParty>();
	TestNotNull(TEXT("unbound party"), Bare);
	TArray<int32> UnboundChoice;
	UnboundChoice.Add(0);
	TestFalse(TEXT("no company rejects a selection"), Bare->SetSelectedCompanions(UnboundChoice));
	TestTrue(TEXT("unbound party fields nobody"), FieldedNamesAre(*this, Bare, 2, {}));

	Party->InitializeFromCompany(Company);
	TestEqual(TEXT("Sam and Alex start selected"), Party->GetSelectedCompanionCount(), 2);
	const TArray<int32>& Starter = Party->GetSelectedCompanionIndices();
	TestEqual(TEXT("starter slot 0"), Starter.Num() > 0 ? Starter[0] : INDEX_NONE, 0);
	TestEqual(TEXT("starter slot 1"), Starter.Num() > 1 ? Starter[1] : INDEX_NONE, 1);
	TestTrue(TEXT("max 0 fields nobody"), FieldedNamesAre(*this, Party, 0, {}));
	TestTrue(TEXT("max 1 fields Sam"), FieldedNamesAre(*this, Party, 1, { TEXT("Sam") }));
	TestTrue(TEXT("max 2 fields Sam and Alex"), FieldedNamesAre(*this, Party, 2, { TEXT("Sam"), TEXT("Alex") }));
	TestTrue(TEXT("a higher max still fields only the selection"),
		FieldedNamesAre(*this, Party, 9, { TEXT("Sam"), TEXT("Alex") }));

	const int32 MaxIndex = Company->FindCompanionIndexByName(TEXT("Max"));
	const int32 QuinnIndex = Company->FindCompanionIndexByName(TEXT("Quinn"));
	TArray<int32> MaxAndQuinn;
	MaxAndQuinn.Add(MaxIndex);
	MaxAndQuinn.Add(99);
	MaxAndQuinn.Add(MaxIndex);
	MaxAndQuinn.Add(QuinnIndex);
	MaxAndQuinn.Add(-1);
	TestTrue(TEXT("select Max then Quinn"), Party->SetSelectedCompanions(MaxAndQuinn));
	TestEqual(TEXT("invalid index and duplicate skipped"), Party->GetSelectedCompanionCount(), 2);
	const TArray<int32>& Chosen = Party->GetSelectedCompanionIndices();
	TestEqual(TEXT("first chosen"), Chosen.Num() > 0 ? Chosen[0] : INDEX_NONE, MaxIndex);
	TestEqual(TEXT("second chosen"), Chosen.Num() > 1 ? Chosen[1] : INDEX_NONE, QuinnIndex);
	TestTrue(TEXT("max 2 fields Max and Quinn"), FieldedNamesAre(*this, Party, 2, { TEXT("Max"), TEXT("Quinn") }));
	TestTrue(TEXT("max 1 fields Max"), FieldedNamesAre(*this, Party, 1, { TEXT("Max") }));

	TestTrue(TEXT("empty selection is allowed"), Party->SetSelectedCompanions(TArray<int32>()));
	TestEqual(TEXT("nothing selected"), Party->GetSelectedCompanionCount(), 0);
	TestTrue(TEXT("empty selection fields nobody"), FieldedNamesAre(*this, Party, 2, {}));

	TestTrue(TEXT("select again before unbind"), Party->SetSelectedCompanions(MaxAndQuinn));
	Party->InitializeFromCompany(nullptr);
	TestEqual(TEXT("clearing the company drops the selection"), Party->GetSelectedCompanionCount(), 0);
	TestNull(TEXT("company unbound"), Party->GetCompany());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidPartySelectionSurvivesEnsureTest,
	"SolidCore1.Company.SelectionSurvivesEnsure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidPartySelectionSurvivesEnsureTest::RunTest(const FString& Parameters)
{
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>();
	TestNotNull(TEXT("gamemode"), GameMode);
	USolidCompany* Company = GameMode->GetCompany();
	USolidParty* Party = GameMode->GetParty();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("party"), Party);
	TestEqual(TEXT("catalog seeded"), Company->GetCompanionCount(), 5);
	TestTrue(TEXT("game starts with Sam and Alex selected"),
		FieldedNamesAre(*this, Party, 2, { TEXT("Sam"), TEXT("Alex") }));

	const int32 Morgan = Company->FindCompanionIndexByName(TEXT("Morgan"));
	const int32 Quinn = Company->FindCompanionIndexByName(TEXT("Quinn"));
	TArray<int32> MorganAndQuinn;
	MorganAndQuinn.Add(Morgan);
	MorganAndQuinn.Add(Quinn);
	TestTrue(TEXT("select Morgan and Quinn"), Party->SetSelectedCompanions(MorganAndQuinn));

	// Enter-town refresh calls EnsureCompanyAndParty on a timer. That must not
	// put Sam and Alex back.
	GameMode->EnsureCompanyAndParty();
	TestTrue(TEXT("selection stays Morgan and Quinn"),
		FieldedNamesAre(*this, Party, 2, { TEXT("Morgan"), TEXT("Quinn") }));
	TestEqual(TEXT("catalog stays five"), Company->GetCompanionCount(), 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionMeshAssignTest,
	"SolidCore1.Company.CompanionMesh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionMeshAssignTest::RunTest(const FString& Parameters)
{
	ASolidCompanionCharacter* Companion = NewObject<ASolidCompanionCharacter>();
	TestNotNull(TEXT("companion"), Companion);
	TestTrue(TEXT("constructor uses the Viking mesh"),
		Companion->GetCompanionMesh().ToSoftObjectPath() == FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));

	const FSoftObjectPath Other(TEXT("/Game/Viking/Mesh/SK_Other.SK_Other"));
	Companion->SetCompanionMesh(TSoftObjectPtr<USkeletalMesh>(Other));
	TestTrue(TEXT("mesh reference is stored before play"),
		Companion->GetCompanionMesh().ToSoftObjectPath() == Other);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
