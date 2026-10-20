#include "Modules/ModuleManager.h"
#include "EditorModeRegistry.h"
#include "SolidWorldChartEdMode.h"

class FSolidCore1EditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FEditorModeRegistry::Get().RegisterMode<FSolidWorldChartEdMode>(
			FSolidWorldChartEdMode::Id,
			NSLOCTEXT("SolidWorldChart", "ModeName", "World chart"),
			FSlateIcon(),
			true);
	}

	virtual void ShutdownModule() override
	{
		FEditorModeRegistry::Get().UnregisterMode(FSolidWorldChartEdMode::Id);
	}
};

IMPLEMENT_MODULE(FSolidCore1EditorModule, SolidCore1Editor)
