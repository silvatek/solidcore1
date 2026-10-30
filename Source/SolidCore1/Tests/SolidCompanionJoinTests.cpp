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

	FString SelectedNames(const USolidSquad* Squad)
	{
		if (!Squad || !Squad->GetCompany())
		{
			return FString();
		}
		FString Names;
		for (const int32 Index : Squad->GetSelectedCompanionIndices())
		{
			const FSolidCompanion* Companion = Squad->GetCompany()->GetCompanion(Index);
			if (!Companion)
			{
				continue;
			}
			if (!Names.IsEmpty())
			{
				Names += TEXT(", ");
			}
			Names += Companion->Name;
		}
		return Names;
	}

	FString CompanyNames(const USolidCompany* Company)
	{
		if (!Company)
		{
			return FString();
		}
		FString Names;
		for (int32 Index = 0; Index < Company->GetCompanionCount(); ++Index)
		{
			const FSolidCompanion* Companion = Company->GetCompanion(Index);
			if (!Companion)
			{
				continue;
			}
			if (!Names.IsEmpty())
			{
				Names += TEXT(", ");
			}
			Names += Companion->Name;
		}
		return Names;
	}

	int32 OccurrenceCount(const FString& Text, const TCHAR* Sentence)
	{
		int32 Count = 0;
		int32 At = 0;
		while (At >= 0 && At < Text.Len())
		{
			const int32 Found = Text.Find(Sentence, ESearchCase::CaseSensitive, ESearchDir::FromStart, At);
			if (Found == INDEX_NONE)
			{
				break;
			}
			++Count;
			At = Found + 1;
		}
		return Count;
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
	TestTrue(TEXT("the catalogue is the same list"), &SolidCompanions::AllCompanions() == &Catalog);
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

	const TCHAR* PlanSentence = TEXT(
		"Starter disabled. Attack disabled. Defend disabled. Line enabled. Column enabled. Tight mob enabled. Loose mob enabled");

	const SolidEvents::FEvent& Kanfold = SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdEnterKanfold)];
	TestEqual(TEXT("Kanfold has twelve changes"), Kanfold.ChangeCount, 12);
	bool bAlexFollowsSize = false;
	for (int32 Index = 0; Index + 1 < Kanfold.ChangeCount; ++Index)
	{
		if (Kanfold.Changes[Index].Kind == SolidEvents::EChange::MaxSquadSize
			&& Kanfold.Changes[Index + 1].Kind == SolidEvents::EChange::AddCompanion
			&& Kanfold.Changes[Index + 1].CompanionName
			&& FCString::Strcmp(Kanfold.Changes[Index + 1].CompanionName, TEXT("Alex")) == 0)
		{
			bAlexFollowsSize = true;
		}
	}
	TestTrue(TEXT("Alex is the change after the squad size"), bAlexFollowsSize);
	const FString KanfoldDetail = USolidJournal::Describe(Kanfold);
	const int32 KanfoldSquadAt = KanfoldDetail.Find(TEXT("Squad 2: Sam, Alex"));
	const int32 AlexJoinsAt = KanfoldDetail.Find(TEXT("Alex joins"));
	const int32 RangerAt = KanfoldDetail.Find(TEXT("Title Ranger"));
	TestTrue(TEXT("Kanfold names Alex after the squad line and before the title"),
		KanfoldSquadAt != INDEX_NONE && AlexJoinsAt != INDEX_NONE && RangerAt != INDEX_NONE
		&& KanfoldSquadAt < AlexJoinsAt && AlexJoinsAt < RangerAt);
	TestEqual(TEXT("Kanfold's battle plans stay one run"), OccurrenceCount(KanfoldDetail, PlanSentence), 1);

	const SolidEvents::FEvent& Dragongate = SolidEvents::Definitions[SolidEvents::FindIndexById(SolidEvents::IdEnterDragongate)];
	TestEqual(TEXT("Dragongate has twenty-two changes"), Dragongate.ChangeCount, 22);
	int32 GateSizeAt = INDEX_NONE;
	for (int32 Index = 0; Index < Dragongate.ChangeCount; ++Index)
	{
		if (Dragongate.Changes[Index].Kind == SolidEvents::EChange::MaxSquadSize)
		{
			GateSizeAt = Index;
			break;
		}
	}
	TestTrue(TEXT("Dragongate's size change has room for the three adds"), GateSizeAt >= 3);
	if (GateSizeAt >= 3)
	{
		const TCHAR* GateNames[] = { TEXT("Max"), TEXT("Morgan"), TEXT("Quinn") };
		for (int32 Step = 0; Step < 3; ++Step)
		{
			const SolidEvents::FChange& Added = Dragongate.Changes[GateSizeAt - 3 + Step];
			TestTrue(*FString::Printf(TEXT("%s is added just before the squad size"), GateNames[Step]),
				Added.Kind == SolidEvents::EChange::AddCompanion
				&& Added.CompanionName
				&& FCString::Strcmp(Added.CompanionName, GateNames[Step]) == 0);
		}
	}
	const FString GateDetail = USolidJournal::Describe(Dragongate);
	const int32 MaxJoinsAt = GateDetail.Find(TEXT("Max joins"));
	const int32 MorganJoinsAt = GateDetail.Find(TEXT("Morgan joins"));
	const int32 QuinnJoinsAt = GateDetail.Find(TEXT("Quinn joins"));
	const int32 GateSquadAt = GateDetail.Find(TEXT("Squad 5: Sam, Alex, Max, Morgan, Quinn"));
	const int32 CaptainAt = GateDetail.Find(TEXT("Title Captain"));
	TestTrue(TEXT("Dragongate names the three before the squad line and the title"),
		MaxJoinsAt != INDEX_NONE && MorganJoinsAt != INDEX_NONE && QuinnJoinsAt != INDEX_NONE
		&& GateSquadAt != INDEX_NONE && CaptainAt != INDEX_NONE
		&& MaxJoinsAt < MorganJoinsAt && MorganJoinsAt < QuinnJoinsAt
		&& QuinnJoinsAt < GateSquadAt && GateSquadAt < CaptainAt);
	TestEqual(TEXT("Dragongate's battle plans stay one run"), OccurrenceCount(GateDetail, PlanSentence), 1);

	FPlay KanfoldOnly;
	if (!TestTrue(TEXT("a Kanfold play opens"), KanfoldOnly.Open()))
	{
		return false;
	}
	EnterTown(KanfoldOnly, SolidTowns::Kanfold);
	TestEqual(TEXT("Kanfold alone lists Alex"), CompanyNames(KanfoldOnly.Company), FString(TEXT("Alex")));
	TestEqual(TEXT("Kanfold alone selects Alex"), SelectedNames(KanfoldOnly.Squad), FString(TEXT("Alex")));
	TestEqual(TEXT("Kanfold's cap is 2"), KanfoldOnly.State->MaxSquadSize, 2);
	const FSolidCompanion* Alex = KanfoldOnly.Company->GetCompanion(0);
	TestNotNull(TEXT("Alex is on the company"), Alex);
	if (Alex)
	{
		TestTrue(TEXT("Alex is a Warrior"), Alex->Vocation == ESolidVocation::Warrior);
		TestEqual(TEXT("Alex wears the company's Viking mesh"), Alex->CharacterMeshIndex, KanfoldOnly.Company->FindCharacterMeshIndexByName(TEXT("Viking")));
	}
	TestTrue(TEXT("Sam stays on the global list"), KanfoldOnly.Company->FindCompanionIndexByName(TEXT("Sam")) == INDEX_NONE);
	EnterTown(KanfoldOnly, SolidTowns::Kanfold);
	TestEqual(TEXT("a second Kanfold stays Alex"), CompanyNames(KanfoldOnly.Company), FString(TEXT("Alex")));
	TestEqual(TEXT("a second Kanfold keeps Alex selected"), SelectedNames(KanfoldOnly.Squad), FString(TEXT("Alex")));

	FPlay Both;
	if (!TestTrue(TEXT("a Relion and Kanfold play opens"), Both.Open()))
	{
		return false;
	}
	EnterTown(Both, SolidTowns::Iglin);
	EnterTown(Both, SolidTowns::Relion);
	EnterTown(Both, SolidTowns::Kanfold);
	TestEqual(TEXT("Relion then Kanfold lists Sam then Alex"), CompanyNames(Both.Company), FString(TEXT("Sam, Alex")));
	TestEqual(TEXT("Relion then Kanfold selects both"), SelectedNames(Both.Squad), FString(TEXT("Sam, Alex")));
	EnterTown(Both, SolidTowns::Dragongate);
	TestEqual(
		TEXT("Dragongate adds the other three"),
		CompanyNames(Both.Company),
		FString(TEXT("Sam, Alex, Max, Morgan, Quinn")));
	TestEqual(TEXT("Dragongate leaves Sam and Alex selected"), SelectedNames(Both.Squad), FString(TEXT("Sam, Alex")));
	TestEqual(TEXT("Dragongate's cap is the roster"), Both.State->MaxSquadSize, SolidEvents::RosterCount);
	EnterTown(Both, SolidTowns::Dragongate);
	TestEqual(TEXT("a second Dragongate keeps the five"), CompanyNames(Both.Company), FString(TEXT("Sam, Alex, Max, Morgan, Quinn")));
	TestEqual(TEXT("a second Dragongate keeps the squad"), SelectedNames(Both.Squad), FString(TEXT("Sam, Alex")));

	FPlay Parade;
	if (!TestTrue(TEXT("a parade play opens"), Parade.Open()))
	{
		return false;
	}
	EnterTown(Parade, SolidTowns::Iglin);
	EnterTown(Parade, SolidTowns::Relion);
	EnterTown(Parade, SolidTowns::Dragongate);
	TestEqual(TEXT("Parade lists Sam then the three"), CompanyNames(Parade.Company), FString(TEXT("Sam, Max, Morgan, Quinn")));
	TestEqual(TEXT("Parade keeps Sam selected"), SelectedNames(Parade.Squad), FString(TEXT("Sam")));
	TestTrue(TEXT("Parade leaves Alex on the global list"), Parade.Company->FindCompanionIndexByName(TEXT("Alex")) == INDEX_NONE);

	FPlay OpenSlot;
	if (!TestTrue(TEXT("an open-slot play opens"), OpenSlot.Open()))
	{
		return false;
	}
	EnterTown(OpenSlot, SolidTowns::Kanfold);
	EnterTown(OpenSlot, SolidTowns::Dragongate);
	TestEqual(
		TEXT("Kanfold then Dragongate lists Alex then the three"),
		CompanyNames(OpenSlot.Company),
		FString(TEXT("Alex, Max, Morgan, Quinn")));
	TestEqual(TEXT("the open slot stays empty"), SelectedNames(OpenSlot.Squad), FString(TEXT("Alex")));
	TestEqual(TEXT("the cap is the roster"), OpenSlot.State->MaxSquadSize, SolidEvents::RosterCount);

	FPlay GateOnly;
	if (!TestTrue(TEXT("a Dragongate play opens"), GateOnly.Open()))
	{
		return false;
	}
	EnterTown(GateOnly, SolidTowns::Dragongate);
	TestEqual(TEXT("Dragongate alone lists the three"), CompanyNames(GateOnly.Company), FString(TEXT("Max, Morgan, Quinn")));
	TestEqual(TEXT("Dragongate alone selects nobody"), SelectedNames(GateOnly.Squad), FString());
	TestEqual(TEXT("Dragongate alone sets the roster cap"), GateOnly.State->MaxSquadSize, SolidEvents::RosterCount);

	FPlay Early;
	if (!TestTrue(TEXT("an early play opens"), Early.Open()))
	{
		return false;
	}
	EnterTown(Early, SolidTowns::Iglin);
	const SolidEvents::FResult Bridge = SolidEvents::NotifyReachBrokenBridge(*Early.State, *Early.Captain);
	TestTrue(TEXT("the bridge fires"), Bridge.bFired);
	const SolidEvents::FResult Killed = SolidEvents::NotifyCaptainKilled(*Early.State, *Early.Captain);
	TestTrue(TEXT("the death fires"), Killed.bFired);
	SolidEvents::NotifyOccupiedTown(*Early.State, *Early.Captain, INDEX_NONE);
	EnterTown(Early, SolidTowns::Iglin);
	const int32 DrivenIndex = SolidEvents::FindIndexById(SolidEvents::IdDrivenFromVillage);
	TestTrue(TEXT("driven from the village fires"), DrivenIndex != INDEX_NONE && Early.State->bFired[DrivenIndex]);
	TestEqual(TEXT("the early events add nobody"), CompanyNames(Early.Company), FString());
	TestEqual(TEXT("the early events select nobody"), SelectedNames(Early.Squad), FString());

	FPlay AfterSam;
	if (!TestTrue(TEXT("a Sam play opens"), AfterSam.Open()))
	{
		return false;
	}
	EnterTown(AfterSam, SolidTowns::Iglin);
	EnterTown(AfterSam, SolidTowns::Relion);
	const SolidEvents::FResult BridgeAfter = SolidEvents::NotifyReachBrokenBridge(*AfterSam.State, *AfterSam.Captain);
	TestTrue(TEXT("the bridge still fires"), BridgeAfter.bFired);
	const SolidEvents::FResult KilledAfter = SolidEvents::NotifyCaptainKilled(*AfterSam.State, *AfterSam.Captain);
	TestTrue(TEXT("the death still fires"), KilledAfter.bFired);
	EnterTown(AfterSam, SolidTowns::Iglin);
	TestTrue(TEXT("driven from the village still fires"), DrivenIndex != INDEX_NONE && AfterSam.State->bFired[DrivenIndex]);
	TestEqual(TEXT("the early events leave Sam on the company"), CompanyNames(AfterSam.Company), FString(TEXT("Sam")));
	TestEqual(TEXT("the early events leave Sam selected"), SelectedNames(AfterSam.Squad), FString(TEXT("Sam")));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
