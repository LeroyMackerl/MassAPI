/*
* Magnus Uncooked
* Author: Ember, All Rights Reserved.
*/

using UnrealBuildTool;

public class MagnusUncooked : ModuleRules
{
	public MagnusUncooked(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
			}
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"BlueprintGraph",
					"GraphEditor",
					"Kismet",
					"KismetWidgets",
					"KismetCompiler",
					"ToolMenus",
					"ToolWidgets",
					"EditorStyle",
					"PropertyEditor",
					"Magnus",
				}
			);
		}
	}
}
