/*
* MassAPI
* Created: 2025
* Author: Leroy Works, All Rights Reserved.
*/
using UnrealBuildTool;

public class MassAPI : ModuleRules
{
    public MassAPI(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                ModuleDirectory
            }
        );

        PrivateIncludePaths.AddRange(
            new string[] {

            }
            );

        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "MassEntity",
                "MassCommon",
                "MassSpawner"
                // "BlueprintGraph" was moved below
            }
        );

        if (Target.bBuildEditor == true)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "BlueprintGraph"
                }
            );
        }

        PrivateDependencyModuleNames.AddRange(
            new string[] {

            }
        );
    }
}
