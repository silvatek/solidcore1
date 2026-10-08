#include "SolidCore1.h"
#include "SolidCore1BuildId.h"
#include "Modules/ModuleManager.h"

class FSolidCore1Module : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();
		// Error severity so it shows even when the Output Log is filtered to errors/warnings.
		UE_LOG(LogTemp, Error, TEXT("[SolidCore1] MODULE STARTUP - build %s"), SOLIDCORE1_BUILD_ID);
		UE_LOG(LogSolidCore1, Error, TEXT("MODULE STARTUP - build %s"), SOLIDCORE1_BUILD_ID);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FSolidCore1Module, SolidCore1, "SolidCore1");

DEFINE_LOG_CATEGORY(LogSolidCore1);
