#include "SolidGameInstance.h"
#include "SolidGameState.h"
#include "SolidSessionSave.h"
#include "SolidCore1.h"
#include "Company/SolidCompany.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"

static const TCHAR* SolidWorldTimeSessionSlot = TEXT("SolidWorldTime");

void USolidGameInstance::OnStart()
{
	Super::OnStart();
	BeginWorldSession();
}

USolidCompany* USolidGameInstance::GetCompany()
{
	if (!Company)
	{
		Company = NewObject<USolidCompany>(this, TEXT("Company"));
		Company->InitializeDefaultBattlePlans();
		Company->InitializeDefaultCharacterMeshes();
		Company->InitializeDefaultCompanions();
		Company->InitializeStartingStronghold();
	}
	return Company;
}

USolidGameState* USolidGameInstance::GetConfiguration()
{
	if (!GameState)
	{
		GameState = NewObject<USolidGameState>(this, TEXT("GameState"));
	}
	return GameState;
}

void USolidGameInstance::BeginWorldSession()
{
	int32 Completed = 0;
	if (const USolidSessionSave* Loaded = Cast<USolidSessionSave>(UGameplayStatics::LoadGameFromSlot(SolidWorldTimeSessionSlot, 0)))
	{
		Completed = Loaded->SessionNumber;
	}

	WorldTime.Begin(Completed, FPlatformTime::Seconds());

	USolidSessionSave* Save = NewObject<USolidSessionSave>(this);
	Save->SessionNumber = WorldTime.GetSessionNumber();
	if (!UGameplayStatics::SaveGameToSlot(Save, SolidWorldTimeSessionSlot, 0))
	{
		UE_LOG(LogSolid, Warning, TEXT("World time session %d was not saved."), WorldTime.GetSessionNumber());
	}
	else
	{
		UE_LOG(LogSolid, Warning, TEXT("World time session %d, %s."), WorldTime.GetSessionNumber(),
			*WorldTime.Format(WorldTime.Stamp(FPlatformTime::Seconds())));
	}
}

void USolidGameInstance::Capture(const FSolidTravelSession& InSession)
{
	Session = InSession;
	bHasSession = true;
}

void USolidGameInstance::StoreExplorationFog(const TArray<float>& Fog)
{
	ExplorationFog = Fog;
	bHasExplorationFog = Fog.Num() > 0;
}

void USolidGameInstance::ClearStrongholdExit()
{
	Session.bExitStronghold = false;
}

void USolidGameInstance::ClearLairMouthSpawn()
{
	Session.bReturnToLairMouth = false;
}
