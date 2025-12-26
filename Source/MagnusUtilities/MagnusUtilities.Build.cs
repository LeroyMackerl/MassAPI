using UnrealBuildTool;

public class MagnusUtilities : ModuleRules
{
    public MagnusUtilities(ReadOnlyTargetRules Target) : base(Target)
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
                }
            );
        }
    }
}