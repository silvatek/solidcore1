#include "Modules/ModuleManager.h"
#include "Containers/Ticker.h"
#include "Editor.h"
#include "EditorModeManager.h"
#include "EditorModeRegistry.h"
#include "ILevelEditor.h"
#include "LevelEditor.h"
#include "Misc/CommandLine.h"
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

		// tools\editor.bat passes this. The mode is switched on once the open-world level is up.
		if (FParse::Param(FCommandLine::Get(), TEXT("SolidWorldChart")))
		{
			FLevelEditorModule& LevelEditor = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
			if (LevelEditor.GetLevelEditorInstance().IsValid())
			{
				WaitForChartLevel();
			}
			else
			{
				LevelEditor.OnLevelEditorCreated().AddRaw(this, &FSolidCore1EditorModule::WaitForChartLevel);
			}
		}
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
		{
			FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor")
				.OnLevelEditorCreated()
				.RemoveAll(this);
		}
		if (ChartTicker.IsValid())
		{
			FTSTicker::RemoveTicker(ChartTicker);
			ChartTicker.Reset();
		}
		FEditorModeRegistry::Get().UnregisterMode(FSolidWorldChartEdMode::Id);
	}

private:
	FTSTicker::FDelegateHandle ChartTicker;

	void WaitForChartLevel(TSharedPtr<ILevelEditor> /*LevelEditor*/ = nullptr)
	{
		if (ChartTicker.IsValid())
		{
			return;
		}
		ChartTicker = FTSTicker::GetCoreTicker().AddTicker(
			TEXT("SolidWorldChart"),
			0.f,
			[this](float) { return OpenWorldChart(); });
	}

	bool OpenWorldChart()
	{
		if (!GEditor)
		{
			return true;
		}
		const UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!World || World->WorldType != EWorldType::Editor)
		{
			return true;
		}
		if (World->GetOutermost()->GetName() != FSolidWorldChartEdMode::OpenWorldLevel)
		{
			return false;
		}
		if (!GLevelEditorModeTools().IsModeActive(FSolidWorldChartEdMode::Id))
		{
			GLevelEditorModeTools().ActivateMode(FSolidWorldChartEdMode::Id);
		}
		return false;
	}
};

IMPLEMENT_MODULE(FSolidCore1EditorModule, SolidCore1Editor)
