using UnrealBuildTool;
using System.Collections.Generic;

public class SolidCore1EditorTarget : TargetRules
{
	public SolidCore1EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SolidCore1");
	}
}
