#include "SolidCore1.h"
#include "SolidBuildId.h"
#include "SolidContentPaths.h"
#include "SolidGameMode.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Modules/ModuleManager.h"

class FSolidModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		// Error severity so it shows even when the Output Log is filtered to errors/warnings.
		UE_LOG(LogSolid, Error, TEXT("MODULE STARTUP - build %s"), SOLID_BUILD_ID);

		// Lvl_ThirdPerson (and similar template maps) bake BP_ThirdPersonGameMode into WorldSettings,
		// which spawns Quinn and skips SolidHUD. Force our GameMode before the world picks one.
		PreWorldInitHandle = FWorldDelegates::OnPreWorldInitialization.AddRaw(
			this, &FSolidModule::HandlePreWorldInitialization);
	}

	virtual void ShutdownModule() override
	{
		if (PreWorldInitHandle.IsValid())
		{
			FWorldDelegates::OnPreWorldInitialization.Remove(PreWorldInitHandle);
			PreWorldInitHandle.Reset();
		}
		FDefaultGameModuleImpl::ShutdownModule();
	}

private:
	void HandlePreWorldInitialization(UWorld* World, const UWorld::InitializationValues /*IVS*/)
	{
		if (!World || World->IsNetMode(NM_Client))
		{
			return;
		}

		// Editor preview worlds (not PIE/game) should keep their authored GameMode.
		if (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::EditorPreview)
		{
			return;
		}

		AWorldSettings* WorldSettings = World->GetWorldSettings();
		if (!WorldSettings)
		{
			return;
		}

		UClass* DesiredGameMode = SolidContentPaths::LoadFirstClass<AGameModeBase>(
			SolidContentPaths::GameModeBlueprintClasses());
		if (!DesiredGameMode)
		{
			DesiredGameMode = ASolidGameMode::StaticClass();
		}

		if (WorldSettings->DefaultGameMode == DesiredGameMode)
		{
			return;
		}

		UE_LOG(LogSolid, Warning,
			TEXT("Overriding WorldSettings GameMode %s -> %s (build %s)"),
			WorldSettings->DefaultGameMode ? *WorldSettings->DefaultGameMode->GetName() : TEXT("<none>"),
			*DesiredGameMode->GetName(),
			SOLID_BUILD_ID);

		WorldSettings->DefaultGameMode = DesiredGameMode;
	}

	FDelegateHandle PreWorldInitHandle;
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSolidModule, SolidCore1, "SolidCore1");

DEFINE_LOG_CATEGORY(LogSolid);

namespace
{
	void ApplyGameStateChange(FGameState& State, const SolidEvents::FChange& Change, SolidEvents::FResult& Result)
	{
		switch (Change.Kind)
		{
		case SolidEvents::EChange::EnableSight:
			if (Change.Sight == ESolidSight::Raven)
			{
				State.bRavenSightEnabled = true;
			}
			else
			{
				State.bTrueSightEnabled = true;
			}
			Result.bSetSight = true;
			Result.Sight = Change.Sight;
			break;
		case SolidEvents::EChange::MaxPartySize:
			State.MaxPartySize = FMath::Max(0, Change.PartySize);
			Result.MaxPartySize = State.MaxPartySize;
			break;
		case SolidEvents::EChange::CaptainTitle:
			if (Change.Title && Change.Title[0])
			{
				State.CaptainTitle = Change.Title;
				Result.bSetTitle = true;
				Result.CaptainTitle = Change.Title;
			}
			break;
		default:
			break;
		}
	}
}

namespace SolidEvents
{

int32 PartyCount(const FGameState& State)
{
	return FMath::Clamp(State.MaxPartySize, 0, RosterCount);
}

bool IsSightEnabled(const FGameState& State, const ESolidSight Sight)
{
	switch (Sight)
	{
	case ESolidSight::True:
		return State.bTrueSightEnabled;
	case ESolidSight::Raven:
		return State.bRavenSightEnabled;
	default:
		return false;
	}
}

void Remember(
	FGameState& State,
	const SolidWorldTime::FStamp& When,
	const SolidJournal::EJournalType Type,
	const FString& Text,
	const int32 EventIndex)
{
	if (State.Journal.Num() >= SolidJournal::MaxEntries)
	{
		State.Journal.RemoveAt(0);
	}

	SolidJournal::FJournalEntry Entry;
	Entry.Type = Type;
	Entry.Year = When.Year;
	Entry.Day = When.Day;
	Entry.MinuteOfSession = When.MinuteOfSession;
	Entry.EventIndex = EventIndex;
	Entry.Text = Text;
	State.Journal.Add(Entry);
}

void RememberStronghold(
	FGameState& State,
	const SolidWorldTime::FStamp& When,
	const bool bEnter)
{
	Remember(
		State,
		When,
		SolidJournal::EJournalType::Stronghold,
		bEnter ? FString(TEXT("Enter Stronghold")) : FString(TEXT("Exit Stronghold")));
}

FResult NotifyOccupiedTown(
	FGameState& State,
	const int32 TownId,
	const SolidWorldTime::FStamp& When)
{
	FResult Result;
	Result.MaxPartySize = State.MaxPartySize;
	Result.CaptainTitle = State.CaptainTitle;
	if (State.OccupiedTown == TownId)
	{
		return Result;
	}

	State.OccupiedTown = TownId;
	if (TownId == INDEX_NONE)
	{
		return Result;
	}

	for (int32 Index = 0; Index < DefinitionCount; ++Index)
	{
		if (State.bFired[Index])
		{
			continue;
		}

		const FEvent& Event = Definitions[Index];
		if (Event.Trigger != ETrigger::EnterTown || Event.Parameter != TownId)
		{
			continue;
		}

		State.bFired[Index] = true;
		if (State.FiredCount < MaxEvents)
		{
			State.FiredOrder[State.FiredCount] = Index;
			++State.FiredCount;
		}
		Remember(State, When, SolidJournal::EJournalType::Event, FString(), Index);
		Result.bFired = true;
		const int32 ChangeCount = FMath::Min(Event.ChangeCount, MaxChangesPerEvent);
		for (int32 ChangeIndex = 0; ChangeIndex < ChangeCount; ++ChangeIndex)
		{
			ApplyGameStateChange(State, Event.Changes[ChangeIndex], Result);
		}
	}

	Result.MaxPartySize = State.MaxPartySize;
	return Result;
}

}
