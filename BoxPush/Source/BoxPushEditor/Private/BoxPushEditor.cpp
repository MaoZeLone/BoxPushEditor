#include "BoxPushEditor.h"

#include "BoxLevelEditor.h"
#include "Styling/AppStyle.h"
#include "Textures/SlateIcon.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "BoxPushEditor"

static const FName BoxLevelEditorTabName("BoxLevelEditor");

void FBoxPushEditorModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		BoxLevelEditorTabName,
		FOnSpawnTab::CreateRaw(this, &FBoxPushEditorModule::SpawnLevelEditorTab))
		.SetDisplayName(LOCTEXT("TabTitle", "BoxPush 关卡编辑器"))
		.SetTooltipText(LOCTEXT("TabTip", "编辑 BoxPush 关卡"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"));

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FBoxPushEditorModule::RegisterMenus));
}

void FBoxPushEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BoxLevelEditorTabName);
	LevelEditor.Reset();
}

void FBoxPushEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	FToolMenuSection& Section = Menu->FindOrAddSection("BoxPush");
	Section.AddMenuEntry(
		"OpenBoxLevelEditor",
		LOCTEXT("Open", "BoxPush 关卡编辑器"),
		LOCTEXT("OpenTip", "打开 Slate 关卡编辑器"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Viewports"),
		FUIAction(FExecuteAction::CreateRaw(this, &FBoxPushEditorModule::OpenLevelEditor)));
}

void FBoxPushEditorModule::OpenLevelEditor()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(BoxLevelEditorTabName));
}

TSharedRef<SDockTab> FBoxPushEditorModule::SpawnLevelEditorTab(const FSpawnTabArgs& Args)
{
	if (!LevelEditor)
	{
		LevelEditor = MakeShared<FBoxLevelEditor>();
		LevelEditor->Initialize();
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("TabLabel", "BoxPush 关卡编辑器"))
		[
			SNew(SBoxLevelEditor, LevelEditor.ToSharedRef())
		];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FBoxPushEditorModule, BoxPushEditor)
