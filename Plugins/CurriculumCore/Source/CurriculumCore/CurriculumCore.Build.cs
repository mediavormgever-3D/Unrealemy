using UnrealBuildTool;

public class CurriculumCore : ModuleRules
{
	public CurriculumCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Bestanden in Private/UI includen headers uit Private.
		PrivateIncludePaths.Add(System.IO.Path.Combine(ModuleDirectory, "Private"));

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"EditorSubsystem"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"BlueprintGraph",
			"InputCore",
			"Json",
			"LevelEditor",
			"MainFrame",
			"Projects",
			"PythonScriptPlugin",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"UnrealEd"
		});
	}
}
