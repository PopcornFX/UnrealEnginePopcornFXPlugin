//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

using System;

namespace UnrealBuildTool.Rules
{
	public class PopcornFXEditor : ModuleRules
	{
		bool					IAmDeveloping = false;

#if WITH_FORWARDED_MODULE_RULES_CTOR
		public PopcornFXEditor(ReadOnlyTargetRules Target) : base(Target)
#else
		public PopcornFXEditor(TargetInfo Target)
#endif // WITH_FORWARDED_MODULE_RULES_CTOR
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			string		sdkFullRoot = Environment.GetEnvironmentVariable("PK_SDK_ROOT");
			if (!String.IsNullOrEmpty(sdkFullRoot))
			{
				// assume that
				IAmDeveloping = true;
			}
			if (IAmDeveloping)
			{
				// maybe not faster, but we want to make sure there is no missing includes
#if UE_4_24_OR_LATER
				bUseUnity = false;
#else
				bFasterWithoutUnity = true;
#endif
				PrivatePCHHeaderFile = "Private/EmptyPCH.h";
			}

			PrivateIncludePaths.AddRange(
				new string[] {
					"PopcornFXEditor/Private",
					"PopcornFX/Private",
				});
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"PopcornFX",
				});

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Engine",
					"UnrealEd",
					"Slate",
					"SlateCore",
					"GraphEditor",
					"BlueprintGraph",
					"KismetCompiler",
					"CoreUObject",
				});
		}
	}
}
