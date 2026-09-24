using UnrealBuildTool;

public class BoxPushEditor : ModuleRules
{
	public BoxPushEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"BoxPush"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"EditorFramework",
			"EditorStyle",
			"EditorWidgets",
			"ToolMenus",
			"AssetRegistry",
		"AssetTools",
		"WorkspaceMenuStructure",
		"AdvancedPreviewScene",
		"PropertyEditor",
		"GameplayTags"
		});
	}
}
