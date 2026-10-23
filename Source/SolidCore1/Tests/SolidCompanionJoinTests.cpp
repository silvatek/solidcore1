#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Company/SolidCompany.h"
#include "Company/SolidSquad.h"
#include "Events/SolidEvents.h"
#include "Inventory/SolidItemType.h"
#include "Journal/SolidJournal.h"
#include "SolidGameInstance.h"
#include "SolidGameMode.h"
#include "Towns/SolidTowns.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	struct FPlay
	{
		USolidGameInstance* Instance = nullptr;
		ASolidGameMode* GameMode = nullptr;
		USolidCompany* Company = nullptr;
		USolidSquad* Squad = nullptr;
		USolidGameState* State = nullptr;
		USolidCaptain* Captain = nullptr;

		bool Open()
		{
			Instance = NewObject<USolidGameInstance>();
			GameMode = Instance ? NewObject<ASolidGameMode>(Instance) : nullptr;
			Company = GameMode ? GameMode->GetCompany() : nullptr;
			Squad = GameMode ? GameMode->GetSquad() : nullptr;
			State = GameMode ? GameMode->GetConfiguration() : nullptr;
			Captain = Company ? Company->GetCaptain() : nullptr;
			return Instance && GameMode && Company && Squad && State && Captain;
		}
	};

	FString SelectedName(const USolidSquad* Squad)
	{
		if (!Squad || !Squad->GetCompany() || Squad->GetSelectedCompanionCount() != 1)
		{
			return FString();
		}
		const TArray<int32>& Selected = Squad->GetSelectedCompanionIndices();
		const FSolidCompanion* Companion = Selected.Num() > 0
			? Squad->GetCompany()->GetCompanion(Selected[0])
			: nullptr;
		return Companion ? Companion->Name : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSolidCompanionJoinTest,
	"SolidCore1.Events.AddCompanion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSolidCompanionJoinTest::RunTest(const FString& Parameters)
{
	const TArray<FSolidCompanion>& Catalog = SolidCompanions::AllCompanions();
	TestEqual(TEXT("the global list holds five"), Catalog.Num(), 5);
	TestEqual(TEXT("a second read stays five"), SolidCompanions::AllCompanions().Num(), 5);
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
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Names); ++Index)
	{
		if (!TestTrue(TEXT("catalog index"), Catalog.IsValidIndex(Index)))
		{
			continue;
		}
		const FSolidCompanion& Companion = Catalog[Index];
		TestEqual(*FString::Printf(TEXT("catalog name %d"), Index), Companion.Name, FString(Names[Index]));
		TestTrue(*FString::Printf(TEXT("catalog vocation %d"), Index), Companion.Vocation == Vocations[Index]);
		FLinearColor Torso;
		FLinearColor Legs;
		FLinearColor Skin;
		SolidCompanionColors::ColorsForName(Companion.Name, Torso, Legs, Skin);
		TestTrue(*FString::Printf(TEXT("catalog torso %d"), Index), Companion.TorsoColor.Equals(Torso));
		TestTrue(*FString::Printf(TEXT("catalog legs %d"), Index), Companion.LegColor.Equals(Legs));
		TestTrue(*FString::Printf(TEXT("catalog skin %d"), Index), Companion.SkinColor.Equals(Skin));
		TestEqual(
			*FString::Printf(TEXT("catalog spear %d"), Index),
			Companion.WeaponId,
			FString(SolidItemType::IdCrudeSpear));
		TestEqual(
			*FString::Printf(TEXT("catalog rags %d"), Index),
			Companion.ArmourId,
			FString(SolidItemType::IdRags));
	}

	USolidCompany* Bare = NewObject<USolidCompany>();
	Bare->InitializeDefaultCompanions();
	TestEqual(TEXT("a new company count is 0"), Bare->GetCompanionCount(), 0);
	Bare->InitializeDefaultCompanions();
	TestEqual(TEXT("init leaves the company empty"), Bare->GetCompanionCount(), 0);
	FSolidCompanion Extra;
	Extra.Name = TEXT("Kept");
	Bare->AddCompanion(Extra);
	Bare->InitializeDefaultCompanions();
	TestEqual(TEXT("init leaves a companion already there"), Bare->GetCompanionCount(), 1);

	FPlay Empty;
	if (!TestTrue(TEXT("a play opens"), Empty.Open()))
	{
		return false;
	}
	TestEqual(TEXT("the game starts with no companions"), Empty.Company->GetCompanionCount(), 0);
	TestEqual(TEXT("the squad starts empty"), Empty.Squad->GetSelectedCompanionCount(), 0);

	SolidEvents::FResult Result;
	SolidEvents::ApplyChange(*Empty.State, *Empty.Captain, SolidEvents::Change::AddCompanion(TEXT("")), Result);
	SolidEvents::ApplyChange(*Empty.State, *Empty.Captain, SolidEvents::Change::AddCompanion(TEXT("Nobody")), Result);
	TestEqual(TEXT("an empty or unknown name adds nobody"), Empty.Company->GetCompanionCount(), 0);
	TestEqual(TEXT("an empty or unknown name selects nobody"), Empty.Squad->GetSelectedCompanionCount(), 0);

	Empty.State->MaxSquadSize = 0;
	SolidEvents::ApplyChange(*Empty.State, *Empty.Captain, SolidEvents::Change::AddCompanion(TEXT("Sam")), Result);
	TestEqual(TEXT("no room still copies Sam"), Empty.Company->GetCompanionCount(), 1);
	TestEqual(TEXT("Sam is the copy"), Empty.Company->GetCompanion(0) ? Empty.Company->GetCompanion(0)->Name : FString(), FString(TEXT("Sam")));
	TestEqual(TEXT("no room leaves the squad empty"), Empty.Squad->GetSelectedCompanionCount(), 0);
	SolidEvents::ApplyChange(*Empty.State, *Empty.Captain, SolidEvents::Change::SetMaxSquadSize(1), Result);
	TestEqual(TEXT("raising the cap does not select Sam"), Empty.Squad->GetSelectedCompanionCount(), 0);
	TestEqual(TEXT("the cap is 1"), Empty.State->MaxSquadSize, 1);

	FPlay Town;
	if (!TestTrue(TEXT("a town play opens"), Town.Open()))
	{
		return false;
	}
	auto EnterTown = [](FPlay& Play, const int32 TownId)
	{
		const SolidEvents::FResult Entered = SolidEvents::NotifyOccupiedTown(
			*Play.State, *Play.Captain, TownId);
		if (Entered.bFired)
		{
			Play.Squad->SyncEnabledBattlePlans(*Play.State);
		}
	};
	EnterTown(Town, SolidTowns::Iglin);
	TestEqual(TEXT("Iglin adds nobody"), Town.Company->GetCompanionCount(), 0);
	TestEqual(TEXT("Iglin selects nobody"), Town.Squad->GetSelectedCompanionCount(), 0);
	EnterTown(Town, SolidTowns::Relion);
	TestEqual(TEXT("Relion's list is Sam"), Town.Company->GetCompanionCount(), 1);
	const FSolidCompanion* Sam = Town.Company->GetCompanion(Town.Company->FindCompanionIndexByName(TEXT("Sam")));
	TestNotNull(TEXT("Sam is on the company"), Sam);
	if (Sam)
	{
		TestTrue(TEXT("Sam is a Rogue"), Sam->Vocation == ESolidVocation::Rogue);
		TestEqual(TEXT("Sam wears the company's Viking mesh"), Sam->CharacterMeshIndex, Town.Company->FindCharacterMeshIndexByName(TEXT("Viking")));
		TestEqual(TEXT("Sam keeps the crude spear"), Sam->WeaponId, FString(SolidItemType::IdCrudeSpear));
		const FSolidCompanion& GlobalSam = SolidCompanions::AllCompanions()[0];
		TestTrue(TEXT("Sam keeps the global shirt"), Sam->TorsoColor.Equals(GlobalSam.TorsoColor));
		TestTrue(TEXT("Sam keeps the global trousers"), Sam->LegColor.Equals(GlobalSam.LegColor));
	}
	TestEqual(TEXT("Relion's cap is 1"), Town.State->MaxSquadSize, 1);
	TestEqual(TEXT("Relion selects Sam"), SelectedName(Town.Squad), FString(TEXT("Sam")));
	EnterTown(Town, SolidTowns::Relion);
	TestEqual(TEXT("a second Relion stays one Sam"), Town.Company->GetCompanionCount(), 1);
	TestEqual(TEXT("a second Relion keeps Sam selected"), SelectedName(Town.Squad), FString(TEXT("Sam")));

	const int32 RelionIndex = SolidEvents::FindIndexById(SolidEvents::IdEnterRelion);
	const SolidEvents::FEvent& Relion = SolidEvents::Definitions[RelionIndex];
	TestEqual(TEXT("Relion has seven changes"), Relion.ChangeCount, 7);
	bool bSamFollowsSize = false;
	for (int32 Index = 0; Index + 1 < Relion.ChangeCount; ++Index)
	{
		if (Relion.Changes[Index].Kind == SolidEvents::EChange::MaxSquadSize
			&& Relion.Changes[Index + 1].Kind == SolidEvents::EChange::AddCompanion
			&& Relion.Changes[Index + 1].CompanionName
			&& FCString::Strcmp(Relion.Changes[Index + 1].CompanionName, TEXT("Sam")) == 0)
		{
			bSamFollowsSize = true;
		}
	}
	TestTrue(TEXT("Sam is the change after the squad size"), bSamFollowsSize);
	const FString Detail = USolidJournal::Describe(Relion);
	const int32 SquadAt = Detail.Find(TEXT("Squad 1: Sam"));
	const int32 JoinsAt = Detail.Find(TEXT("Sam joins"));
	const int32 TitleAt = Detail.Find(TEXT("Title Vagrant"));
	TestTrue(TEXT("Relion names Sam after the squad line and before the title"),
		SquadAt != INDEX_NONE && JoinsAt != INDEX_NONE && TitleAt != INDEX_NONE
		&& SquadAt < JoinsAt && JoinsAt < TitleAt);
	TestFalse(TEXT("Relion does not name Alex"), Detail.Contains(TEXT("Alex")));

	FPlay Room;
	if (!TestTrue(TEXT("a room play opens"), Room.Open()))
	{
		return false;
	}
	SolidEvents::ApplyChange(*Room.State, *Room.Captain, SolidEvents::Change::SetMaxSquadSize(1), Result);
	SolidEvents::ApplyChange(*Room.State, *Room.Captain, SolidEvents::Change::AddCompanion(TEXT("sam")), Result);
	TestEqual(TEXT("Sam joins when there is room"), SelectedName(Room.Squad), FString(TEXT("Sam")));
	SolidEvents::ApplyChange(*Room.State, *Room.Captain, SolidEvents::Change::AddCompanion(TEXT("Alex")), Result);
	TestTrue(TEXT("Alex is on the company"), Room.Company->FindCompanionIndexByName(TEXT("Alex")) != INDEX_NONE);
	TestEqual(TEXT("a full squad stays Sam"), SelectedName(Room.Squad), FString(TEXT("Sam")));
	SolidEvents::ApplyChange(*Room.State, *Room.Captain, SolidEvents::Change::AddCompanion(TEXT("Sam")), Result);
	TestEqual(TEXT("Sam stays one entry"), Room.Company->GetCompanionCount(), 2);
	TestEqual(TEXT("Sam stays in his box"), SelectedName(Room.Squad), FString(TEXT("Sam")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
