using UnrealBuildTool;

public class SCTD : ModuleRules
{
	public SCTD(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"PhysicsCore",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
