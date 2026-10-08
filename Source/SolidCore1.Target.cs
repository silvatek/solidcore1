using UnrealBuildTool;
using System.Collections.Generic;

public class SolidCore1Target : TargetRules
{
	public SolidCore1Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SolidCore1");
	}
}
