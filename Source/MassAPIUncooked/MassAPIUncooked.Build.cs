/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/

using UnrealBuildTool;

public class MassAPIUncooked : ModuleRules
{
    public MassAPIUncooked(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[] { ModuleDirectory });

        PrivateIncludePaths.AddRange(new string[] { });

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "StructViewer", "InputCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "MassEntity",
                "MassAPI",
                "AppFramework",
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
                    "EditorStyle",
                    "PropertyEditor",
                    "MagnusExtension",
                    "MagnusUtilities",
                    "MagnusNodes",
                }
            );
        }
    }
}
