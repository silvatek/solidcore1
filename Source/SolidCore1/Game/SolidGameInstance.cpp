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

	SessionNumber = SolidWorldTime::NextSessionNumber(Completed);
	SessionStartSeconds = FPlatformTime::Seconds();

	USolidSessionSave* Save = NewObject<USolidSessionSave>(this);
	Save->SessionNumber = SessionNumber;
	if (!UGameplayStatics::SaveGameToSlot(Save, SolidWorldTimeSessionSlot, 0))
	{
		UE_LOG(LogSolid, Warning, TEXT("World time session %d was not saved."), SessionNumber);
	}
	else
	{
		UE_LOG(LogSolid, Warning, TEXT("World time session %d, %s."), SessionNumber,
			*SolidWorldTime::Format(WorldStamp()));
	}
}

SolidWorldTime::FStamp USolidGameInstance::WorldStamp() const
{
	if (SessionStartSeconds <= 0.0)
	{
		return SolidWorldTime::FromSession(SessionNumber, 0.0);
	}
	const double Elapsed = FPlatformTime::Seconds() - SessionStartSeconds;
	return SolidWorldTime::FromSession(SessionNumber, Elapsed);
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
