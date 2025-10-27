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
                "MassEntity",
                "MassAPI",
            }
        );

        // Editor-only modules must be placed in this conditional block
        if (Target.bBuildEditor == true)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "ToolMenus",
                    "BlueprintGraph",
                    "KismetCompiler"
                }
            );
        }
    }
}
