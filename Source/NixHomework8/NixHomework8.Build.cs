// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NixHomework8 : ModuleRules
{
	public NixHomework8(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"Niagara",
            "NiagaraCore"
        });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"NixHomework8",
			"NixHomework8/Variant_Platforming",
			"NixHomework8/Variant_Platforming/Animation",
			"NixHomework8/Variant_Combat",
			"NixHomework8/Variant_Combat/AI",
			"NixHomework8/Variant_Combat/Animation",
			"NixHomework8/Variant_Combat/Gameplay",
			"NixHomework8/Variant_Combat/Interfaces",
			"NixHomework8/Variant_Combat/UI",
			"NixHomework8/Variant_SideScrolling",
			"NixHomework8/Variant_SideScrolling/AI",
			"NixHomework8/Variant_SideScrolling/Gameplay",
			"NixHomework8/Variant_SideScrolling/Interfaces",
			"NixHomework8/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
