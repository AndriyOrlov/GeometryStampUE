using UnrealBuildTool;

public class GeometryStamp : ModuleRules
{
    public GeometryStamp(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "GeometryCore",
                "GeometryFramework"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "AssetRegistry",
                "PlacementMode",
                "PropertyEditor",
                "SlateCore",
                "UnrealEd"
            });
    }
}
