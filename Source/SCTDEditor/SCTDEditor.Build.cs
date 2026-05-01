using UnrealBuildTool;

public class SCTDEditor : ModuleRules
{
	public SCTDEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SCTD"
		});

		PublicIncludePaths.AddRange(new string[]
		{
			"SCTD"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraph",
			"AnimGraphRuntime",
			"BlueprintGraph",
			"Kismet",
			"UnrealEd"
		});
	}
}
