using UnrealBuildTool;

public class SolidCore1 : ModuleRules
{
	public SolidCore1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory + "/Player");
		PrivateIncludePaths.Add(ModuleDirectory + "/Character");
		PrivateIncludePaths.Add(ModuleDirectory + "/Game");
		PrivateIncludePaths.Add(ModuleDirectory + "/Content");
		PrivateIncludePaths.Add(ModuleDirectory + "/Terrain");
		PrivateIncludePaths.Add(ModuleDirectory + "/Fog");
		PrivateIncludePaths.Add(ModuleDirectory + "/Companion");
		PrivateIncludePaths.Add(ModuleDirectory + "/Company");
		PrivateIncludePaths.Add(ModuleDirectory + "/BattlePlans");
		PrivateIncludePaths.Add(ModuleDirectory + "/Battle");
		PrivateIncludePaths.Add(ModuleDirectory + "/Vegetation");
		PrivateIncludePaths.Add(ModuleDirectory + "/Towns");
		PrivateIncludePaths.Add(ModuleDirectory + "/Strongholds");
		PrivateIncludePaths.Add(ModuleDirectory + "/Lairs");
		PrivateIncludePaths.Add(ModuleDirectory + "/Monster");
		PrivateIncludePaths.Add(ModuleDirectory + "/HUD");
		PrivateIncludePaths.Add(ModuleDirectory + "/Menus");
		PrivateIncludePaths.Add(ModuleDirectory + "/Playthrough");
		PrivateIncludePaths.Add(ModuleDirectory + "/Journal");
		PrivateIncludePaths.Add(ModuleDirectory + "/Inventory");
		PrivateIncludePaths.Add(ModuleDirectory + "/Crafting");
		PrivateIncludePaths.Add(ModuleDirectory + "/Events");
		PrivateIncludePaths.Add(ModuleDirectory + "/Tests");

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"MeshDescription",
			"StaticMeshDescription",
			"SkeletalMeshDescription"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"RenderCore",
			"Slate",
			"SlateCore"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}
