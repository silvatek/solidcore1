#include "SolidCore1.h"
#include "SolidBuildId.h"
#include "SolidGameMode.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Modules/ModuleManager.h"
#include "UObject/SoftObjectPath.h"

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

		// Prefer Solid* BP names; legacy BP_SolidCore1GameMode remains supported.
		UClass* DesiredGameMode = LoadClass<AGameModeBase>(
			nullptr, TEXT("/Game/Characters/BP_SolidGameMode.BP_SolidGameMode_C"));
		if (!DesiredGameMode)
		{
			DesiredGameMode = LoadClass<AGameModeBase>(
				nullptr, TEXT("/Game/Characters/BP_SolidCore1GameMode.BP_SolidCore1GameMode_C"));
		}
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
