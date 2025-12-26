using UnrealBuildTool;

public class MagnusExtension : ModuleRules
{
    public MagnusExtension(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(new string[] {
            ModuleDirectory + "/Public",
            ModuleDirectory + "/Public/NodeCompiler"
        });

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
                    "StructViewer",
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
                }
            );
        }
    }
}