using UnrealBuildTool;

public class SolidCore1Editor : ModuleRules
{
	public SolidCore1Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Slate",
			"SlateCore",
			"InputCore",
			"SolidCore1"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EditorFramework",
			"Json",
			"LevelEditor"
		});

		// The chart headers include siblings by the short names the game module uses.
		string Game = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "../SolidCore1"));
		PrivateIncludePaths.Add(Game);
		foreach (string Folder in new string[]
		{
			"Character", "Companion", "Content", "Crafting", "Events", "Fog", "Game",
			"HUD", "Inventory", "Journal", "Lairs", "Menus", "Player", "Sight", "Squad",
			"Strongholds", "Terrain", "Towns", "Vegetation"
		})
		{
			PrivateIncludePaths.Add(System.IO.Path.Combine(Game, Folder));
		}
	}
}
