using UnrealBuildTool;

public class SolidCore1 : ModuleRules
{
	public SolidCore1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory + "/Terrain");
		PrivateIncludePaths.Add(ModuleDirectory + "/Companion");
		PrivateIncludePaths.Add(ModuleDirectory + "/Vegetation");
		PrivateIncludePaths.Add(ModuleDirectory + "/Tests");

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"MeshDescription",
			"StaticMeshDescription"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry"
		});
	}
}
