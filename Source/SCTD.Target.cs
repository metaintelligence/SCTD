using UnrealBuildTool;
using System.Collections.Generic;

public class SCTDTarget : TargetRules
{
	public SCTDTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SCTD");
	}
}
