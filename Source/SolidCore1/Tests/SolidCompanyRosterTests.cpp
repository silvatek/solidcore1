#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include <initializer_list>
#include "Companion/SolidCompanionCharacter.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"
#include "Inventory/SolidInventory.h"
#include "Inventory/SolidItemType.h"
#include "SolidClipLocomotion.h"
#include "SolidCompanionTestSeed.h"
#include "SolidGameInstance.h"
#include "SolidGameMode.h"
#include "UObject/SoftObjectPath.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	bool FieldedNamesAre(
		FAutomationTestBase& Test,
		USolidSquad* Squad,
		const int32 MaxCount,
		std::initializer_list<const TCHAR*> Expected)
	{
		TArray<FSolidCompanion> Fielded;
		Squad->CollectFieldedCompanions(MaxCount, Fielded);
		if (Fielded.Num() != static_cast<int32>(Expected.size()))
		{
			Test.AddError(FString::Printf(
				TEXT("fielded %d, expected %d (max %d)"),
				Fielded.Num(), static_cast<int32>(Expected.size()), MaxCount));
			return false;
		}
		int32 Index = 0;
		for (const TCHAR* Name : Expected)
		{
			if (Fielded[Index].Name != Name)
			{
				Test.AddError(FString::Printf(
					TEXT("fielded[%d] is %s, expected %s"),
					Index, *Fielded[Index].Name, Name));
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
	Company->InitializeDefaultCharacterMeshes();

	const FSolidCharacterMesh* Viking = Company->GetCharacterMesh(0);
	TestNotNull(TEXT("viking mesh"), Viking);
	if (Viking)
	{
		TestEqual(TEXT("mesh name"), Viking->Name, FString(TEXT("Viking")));
		TestTrue(TEXT("viking asset"),
			Viking->Mesh.ToSoftObjectPath() == FSoftObjectPath(SolidClipLocomotion::DefaultMeshPath));
		TestEqual(TEXT("viking token"), Viking->AssetToken, FString(TEXT("SK_Viking")));
	}
	TestEqual(TEXT("find viking"), Company->FindCharacterMeshIndexByName(TEXT("viking")), 0);
	TestEqual(TEXT("unknown mesh"), Company->FindCharacterMeshIndexByName(TEXT("Shield")), INDEX_NONE);
	TestNull(TEXT("mesh out of range"), Company->GetCharacterMesh(99));
	TestNull(TEXT("no fourth mesh"), Company->GetCharacterMesh(3));

	Company->InitializeDefaultCompanions();
	const int32 AfterInit = Company->GetCompanionCount();
	Company->InitializeDefaultCompanions();
	TestEqual(TEXT("idempotent init"), Company->GetCompanionCount(), AfterInit);
	TestEqual(TEXT("companion init does not reseed meshes"), Company->FindCharacterMeshIndexByName(TEXT("Viking")), 0);
	TestNull(TEXT("companion init does not append a mesh"), Company->GetCharacterMesh(3));

	SolidCompanionTest::SeedCompanyCompanions(Company);
	TestEqual(TEXT("the company holds the five"), Company->GetCompanionCount(), 5);

	const TCHAR* Names[] = {
		TEXT("Sam"),
		TEXT("Alex"),
		TEXT("Max"),
		TEXT("Morgan"),
		TEXT("Quinn"),
	};
	const ESolidVocation Vocations[] = {
		ESolidVocation::Rogue,
		ESolidVocation::Warrior,
		ESolidVocation::Cleric,
		ESolidVocation::Mage,
		ESolidVocation::Warrior,
	};
	static_assert(UE_ARRAY_COUNT(Names) == UE_ARRAY_COUNT(Vocations), "Vocation per companion");
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
		TestTrue(
			*FString::Printf(TEXT("vocation %d"), Index),
			Companion->Vocation == Vocations[Index]);
		FLinearColor ExpectedTorso;
		FLinearColor ExpectedLegs;
		FLinearColor ExpectedSkin;
		SolidCompanionColors::ColorsForName(Companion->Name, ExpectedTorso, ExpectedLegs, ExpectedSkin);
		TestFalse(
			*FString::Printf(TEXT("skin is painted %d"), Index),
			ExpectedSkin.Equals(FLinearColor::Black));
		TestTrue(
			*FString::Printf(TEXT("torso color %d"), Index),
			Companion->TorsoColor.Equals(ExpectedTorso));
		TestTrue(
			*FString::Printf(TEXT("leg color %d"), Index),
			Companion->LegColor.Equals(ExpectedLegs));
		TestFalse(
			*FString::Printf(TEXT("torso differs from legs %d"), Index),
			Companion->TorsoColor.Equals(Companion->LegColor));
		TestEqual(
			*FString::Printf(TEXT("find %s"), Names[Index]),
			Company->FindCompanionIndexByName(Names[Index]),
			Index);
		TestEqual(
			*FString::Printf(TEXT("%s keeps the crude spear"), Names[Index]),
			Companion->WeaponId,
			FString(SolidItemType::IdCrudeSpear));
		TestEqual(
			*FString::Printf(TEXT("%s keeps Rags"), Names[Index]),
			Companion->ArmourId,
			FString(SolidItemType::IdRags));
		bSeenWarrior |= Companion->Vocation == ESolidVocation::Warrior;
		bSeenRogue |= Companion->Vocation == ESolidVocation::Rogue;
		bSeenMage |= Companion->Vocation == ESolidVocation::Mage;
		bSeenCleric |= Companion->Vocation == ESolidVocation::Cleric;
	}
	const FSolidCompanion* Sam = Company->GetCompanion(0);
	const FSolidCompanion* AlexCompanion = Company->GetCompanion(1);
	if (Sam && AlexCompanion)
	{
		TestFalse(TEXT("Sam and Alex wear different shirts"), Sam->TorsoColor.Equals(AlexCompanion->TorsoColor));
	}
	TestTrue(TEXT("a companion is a warrior"), bSeenWarrior);
	TestTrue(TEXT("a companion is a rogue"), bSeenRogue);
	TestTrue(TEXT("a companion is a mage"), bSeenMage);
	TestTrue(TEXT("a companion is a cleric"), bSeenCleric);

	SolidInventory::FLoadout Captain = SolidInventory::StartingLoadout();
	const FString SamWeapon = Sam ? Sam->WeaponId : FString();
	const FString SamArmour = Sam ? Sam->ArmourId : FString();
	SolidInventory::PlacePack(Captain, 0, 0, SolidItemType::IdRustySword);
	SolidInventory::FSlotHit FromPack;
	FromPack.bHit = true;
	FromPack.bPack = true;
	FromPack.Index = SolidInventory::PackIndex(0, 0);
	SolidInventory::FSlotHit ToHand;
	ToHand.bHit = true;
	ToHand.Slot = SolidInventory::ESlot::MainHand;
	ToHand.Index = SolidInventory::WornIndex(SolidInventory::ESlot::MainHand);
	SolidStorage::FContainer Chest = SolidStorage::StartingCaveChest();
	TestTrue(TEXT("the captain can swap weapons"), SolidInventory::Move(&Chest, Captain, FromPack, ToHand));
	TestEqual(TEXT("Sam's spear stays"), Sam ? Sam->WeaponId : FString(), SamWeapon);
	TestEqual(TEXT("Sam's armour stays"), Sam ? Sam->ArmourId : FString(), SamArmour);
	SolidInventory::FPage Page;
	SolidInventory::Layout(Captain, Page);
	TestEqual(TEXT("the inventory page is the captain's"), FString(SolidInventory::Title), FString(TEXT("Inventory")));
	for (const SolidInventory::FBox& Box : Page.Boxes)
	{
		TestFalse(TEXT("the page does not name a companion"), Box.Item.Contains(TEXT("Sam")) || Box.Label.Contains(TEXT("Sam")));
	}

	TestEqual(TEXT("find ignores case"), Company->FindCompanionIndexByName(TEXT("morgan")), 3);
	TestEqual(TEXT("unknown name"), Company->FindCompanionIndexByName(TEXT("Nobody")), INDEX_NONE);
	TestNull(TEXT("out of range"), Company->GetCompanion(99));
	TestNull(TEXT("negative index"), Company->GetCompanion(-1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadCompanionSelectionTest,
	"SolidCore1.Company.SquadSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadCompanionSelectionTest::RunTest(const FString& Parameters)
{
	USolidCompany* Company = NewObject<USolidCompany>();
	USolidSquad* Squad = NewObject<USolidSquad>();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);

	USolidSquad* Bare = NewObject<USolidSquad>();
	TestNotNull(TEXT("unbound squad"), Bare);
	TArray<int32> UnboundChoice;
	UnboundChoice.Add(0);
	TestFalse(TEXT("no company rejects a selection"), Bare->SetSelectedCompanions(UnboundChoice));
	TestTrue(TEXT("unbound squad fields nobody"), FieldedNamesAre(*this, Bare, 2, {}));

	Squad->InitializeFromCompany(Company);
	SolidCompanionTest::SeedCompanyCompanions(Company);
	TestEqual(TEXT("the squad starts empty"), Squad->GetSelectedCompanionCount(), 0);
	const int32 Sam = Company->FindCompanionIndexByName(TEXT("Sam"));
	const int32 Alex = Company->FindCompanionIndexByName(TEXT("Alex"));
	TestTrue(TEXT("select Sam then Alex"), Squad->SetSelectedCompanions({ Sam, Alex }));
	const TArray<int32>& Starter = Squad->GetSelectedCompanionIndices();
	TestEqual(TEXT("slot 0 is Sam"), Starter.Num() > 0 ? Starter[0] : INDEX_NONE, Sam);
	TestEqual(TEXT("slot 1 is Alex"), Starter.Num() > 1 ? Starter[1] : INDEX_NONE, Alex);
	TestTrue(TEXT("max 0 fields nobody"), FieldedNamesAre(*this, Squad, 0, {}));
	TestTrue(TEXT("max 1 fields Sam"), FieldedNamesAre(*this, Squad, 1, { TEXT("Sam") }));
	TestTrue(TEXT("max 2 fields Sam and Alex"), FieldedNamesAre(*this, Squad, 2, { TEXT("Sam"), TEXT("Alex") }));
	TestTrue(TEXT("a higher max still fields only the selection"),
		FieldedNamesAre(*this, Squad, 9, { TEXT("Sam"), TEXT("Alex") }));

	const int32 MaxIndex = Company->FindCompanionIndexByName(TEXT("Max"));
	const int32 QuinnIndex = Company->FindCompanionIndexByName(TEXT("Quinn"));
	TArray<int32> MaxAndQuinn;
	MaxAndQuinn.Add(MaxIndex);
	MaxAndQuinn.Add(99);
	MaxAndQuinn.Add(MaxIndex);
	MaxAndQuinn.Add(QuinnIndex);
	MaxAndQuinn.Add(-1);
	TestTrue(TEXT("select Max then Quinn"), Squad->SetSelectedCompanions(MaxAndQuinn));
	TestEqual(TEXT("invalid index and duplicate skipped"), Squad->GetSelectedCompanionCount(), 2);
	const TArray<int32>& Chosen = Squad->GetSelectedCompanionIndices();
	TestEqual(TEXT("first chosen"), Chosen.Num() > 0 ? Chosen[0] : INDEX_NONE, MaxIndex);
	TestEqual(TEXT("second chosen"), Chosen.Num() > 1 ? Chosen[1] : INDEX_NONE, QuinnIndex);
	TestTrue(TEXT("max 2 fields Max and Quinn"), FieldedNamesAre(*this, Squad, 2, { TEXT("Max"), TEXT("Quinn") }));
	TestTrue(TEXT("max 1 fields Max"), FieldedNamesAre(*this, Squad, 1, { TEXT("Max") }));

	TestTrue(TEXT("empty selection is allowed"), Squad->SetSelectedCompanions(TArray<int32>()));
	TestEqual(TEXT("nothing selected"), Squad->GetSelectedCompanionCount(), 0);
	TestTrue(TEXT("empty selection fields nobody"), FieldedNamesAre(*this, Squad, 2, {}));

	TestTrue(TEXT("select again before unbind"), Squad->SetSelectedCompanions(MaxAndQuinn));
	Squad->InitializeFromCompany(nullptr);
	TestEqual(TEXT("clearing the company drops the selection"), Squad->GetSelectedCompanionCount(), 0);
	TestNull(TEXT("company unbound"), Squad->GetCompany());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidSquadSelectionSurvivesEnsureTest,
	"SolidCore1.Company.SelectionSurvivesEnsure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidSquadSelectionSurvivesEnsureTest::RunTest(const FString& Parameters)
{
	USolidGameInstance* Instance = NewObject<USolidGameInstance>();
	ASolidGameMode* GameMode = NewObject<ASolidGameMode>(Instance);
	TestNotNull(TEXT("gamemode"), GameMode);
	USolidCompany* Company = GameMode->GetCompany();
	USolidSquad* Squad = GameMode->GetSquad();
	TestNotNull(TEXT("company"), Company);
	TestNotNull(TEXT("squad"), Squad);
	TestEqual(TEXT("the squad boxes start empty"), Squad->GetSelectedCompanionCount(), 0);
	TestTrue(TEXT("game starts with nobody selected"),
		FieldedNamesAre(*this, Squad, 2, {}));

	const int32 Before = Company->GetCompanionCount();
	GameMode->EnsureCompanyAndSquad();
	TestEqual(TEXT("ensure leaves the company"), Company->GetCompanionCount(), Before);

	SolidCompanionTest::SeedCompanyCompanions(Company);
	const int32 Held = Company->GetCompanionCount();
	const int32 Morgan = Company->FindCompanionIndexByName(TEXT("Morgan"));
	const int32 Quinn = Company->FindCompanionIndexByName(TEXT("Quinn"));
	TArray<int32> MorganAndQuinn;
	MorganAndQuinn.Add(Morgan);
	MorganAndQuinn.Add(Quinn);
	TestTrue(TEXT("select Morgan and Quinn"), Squad->SetSelectedCompanions(MorganAndQuinn));

	// Enter-town refresh calls EnsureCompanyAndSquad on a timer. That must not
	// put Sam and Alex back.
	GameMode->EnsureCompanyAndSquad();
	TestTrue(TEXT("selection stays Morgan and Quinn"),
		FieldedNamesAre(*this, Squad, 2, { TEXT("Morgan"), TEXT("Quinn") }));
	TestEqual(TEXT("ensure leaves the catalog"), Company->GetCompanionCount(), Held);
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
