// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BoxPush : ModuleRules
{
	public BoxPush(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"AssetRegistry",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"DeveloperSettings",
			"UMG",
			"Slate",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ImageWrapper",
			"ImageCore",
			"GMP"
		});
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
