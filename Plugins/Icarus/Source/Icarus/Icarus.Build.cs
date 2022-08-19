// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class Icarus : ModuleRules
{

    private string ModulePath
    {
        get { return ModuleDirectory; }
    }
    private string ThirdPartyPath
    {
        get { return Path.GetFullPath(Path.Combine(ModulePath, "../../ThirdParty/")); }
    }

	public Icarus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        //https://forums.unrealengine.com/t/how-do-i-resolve-the-error-message-unable-to-load-module-because-the-file-couldnt-be-loaded-by-the-os/283435/3

        bUseRTTI = true;
        bEnableExceptions = true;
        ShadowVariableWarningLevel = WarningLevel.Off;
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "CGAL"));
		bEnableUndefinedIdentifierWarnings = false;
        PublicDefinitions.Add("WIN64");
        PublicDefinitions.Add("_WINDOWS");
        PublicDefinitions.Add("_CRT_SECURE_NO_DEPRECATE");
        PublicDefinitions.Add("_SCL_SECURE_NO_DEPRECATE");
        PublicDefinitions.Add("_CRT_SECURE_NO_WARNINGS");
        PublicDefinitions.Add("_SCL_SECURE_NO_WARNINGS");
        PublicDefinitions.Add("CGAL_USE_MPFR");
        PublicDefinitions.Add("CGAL_USE_GMP");

		string CGALPath = Path.Combine(ThirdPartyPath, "CGAL");

        string LibPath = "";
        bool isdebug = Target.Configuration == UnrealTargetConfiguration.Debug && Target.bDebugBuildsActuallyUseDebugCRT;
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            LibPath = Path.Combine(CGALPath, "libraries", "Win64");



            PublicAdditionalLibraries.Add(Path.Combine(LibPath, "mpfr.lib"));

            if (!isdebug)
            {
                //Add Static Libraries
                PublicAdditionalLibraries.Add(Path.Combine(LibPath,"gmp.lib"));
            }
            else
            {
                //Add Static Libraries (Debug Version)
                PublicAdditionalLibraries.Add(Path.Combine(LibPath,"gmpd.lib"));
            }



            string BinPath = Path.Combine(CGALPath, "bin", "Win64");

            RuntimeDependencies.Add("$(BinaryOutputDir)/gmp.dll", Path.Combine(BinPath, "gmp.dll"));
            RuntimeDependencies.Add("$(BinaryOutputDir)/mpfr-6.dll", Path.Combine(BinPath, "mpfr-6.dll"));


        }

        PublicDependencyModuleNames.AddRange(
		new string[]
		{
			"Core",
			// ... add other public dependencies that you statically link with here ...
		}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore"
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}
