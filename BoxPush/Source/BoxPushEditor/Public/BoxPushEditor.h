#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"

class FBoxLevelEditor;

class FBoxPushEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OpenLevelEditor();

private:
	void RegisterMenus();
	TSharedRef<SDockTab> SpawnLevelEditorTab(const FSpawnTabArgs& Args);

	TSharedPtr<FBoxLevelEditor> LevelEditor;
};
