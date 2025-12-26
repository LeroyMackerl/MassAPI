using UnrealBuildTool;

public class MagnusNodes : ModuleRules
{
    public MagnusNodes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
                "SlateCore"
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
                    "MagnusExtension",
                    "MagnusUtilities",
                }
            );
        }
    }
}