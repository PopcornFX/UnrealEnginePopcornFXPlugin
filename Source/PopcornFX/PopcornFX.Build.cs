//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.IO;
using System.Diagnostics;

namespace UnrealBuildTool.Rules
{
	public class PopcornFX : ModuleRules
	{
		bool					IAmDeveloping = false;

		static string			DefaultClientName = "UnrealEngine";
		private static char[]	DirSeparators = {'/', '\\'};
		private string[]		PkLibs = new string[] {
			"PK-Runtime",
			"PK-RenderHelpers",
			"PK-ParticlesToolbox", // PopcornFX::ParticleToolbox::PredictCurrentVelocity/TransformDeferred
			"PK-Plugin_CompilerBackend_CPU_VM",
		};

		private string[]		PkLibsEditorOnly = new string[] {
			"PK-AssetBakerLib",
		};

		enum SDK
		{
			No,
			Embed,
			Full,
		}
		SDK						CurrentSDK = SDK.No;

		private void			Log(string message)
		{
			Console.WriteLine("PopcornFX - " + message);
		}

		private void			LogError(string message)
		{
			Console.WriteLine("PopcornFX - ERROR - " + message);
		}

		private void			Add01DefinitionFromValue(string definition, bool condition)
		{
			if (condition)
				AddDefinition(definition + "=1");
			else
				AddDefinition(definition + "=0");
		}

		private void	AddDefinition(string def)
		{
			PublicDefinitions.Add(def);
		}

		private void _CopyBackwardCompatFileIFN(string targetFile, string srcFile)
		{
			// Avoid constant writes.. might introduce checkouts in perforce or other
			if (File.Exists(targetFile))
			{
				byte[] fileToCopy = File.ReadAllBytes(srcFile);
				byte[] fileToWrite = File.ReadAllBytes(targetFile);

				bool writeContent = fileToCopy.Length != fileToWrite.Length;
				if (!writeContent)
				{
					for (int iB = 0; iB < fileToCopy.Length; ++iB)
					{
						if (fileToCopy[iB] != fileToWrite[iB])
						{
							writeContent = true;
							break;
						}
					}
				}
				if (!writeContent)
					return;
				File.SetAttributes(targetFile, FileAttributes.Normal);
				File.Delete(targetFile);
			}
			File.Copy(srcFile, targetFile, true);
			File.SetAttributes(targetFile, FileAttributes.ReadOnly);
		}

		private void SetupBackwardCompatFiles()
		{
			string moduleSourceDir = ModuleDirectory + "/";
			string backwardCompatPrivateSourceDir = moduleSourceDir + "Private/";
			string backwardCompatDir = moduleSourceDir + "../../BackwardCompat/";

			// _CopyBackwardCompatFileIFN(seqTemplateTargetFile, seqTemplateSrcFile);
		}

#if WITH_FORWARDED_MODULE_RULES_CTOR
		private bool			SetupPopcornFX(ReadOnlyTargetRules Target)
#else
		private bool			SetupPopcornFX(TargetInfo Target)
#endif // WITH_FORWARDED_MODULE_RULES_CTOR
		{
			CurrentSDK = SDK.No;

			string		sdkRoot = null;

			string		pluginRoot = ModuleDirectory + "/../..";
			pluginRoot = Path.GetFullPath(pluginRoot);
			pluginRoot = Utils.CleanDirectorySeparators(pluginRoot, '/').TrimEnd(DirSeparators) + "/";

			string		sdkEmbedRoot = pluginRoot + "PopcornFX_Runtime_SDK/";
			if (System.IO.Directory.Exists(sdkEmbedRoot + "source_tree"))
			{
				CurrentSDK = SDK.Embed;
				sdkRoot = sdkEmbedRoot;
			}
			else
			{
				string		sdkFullRoot = Environment.GetEnvironmentVariable("PK_SDK_ROOT");
				if (!String.IsNullOrEmpty(sdkFullRoot))
				{
					sdkFullRoot = Utils.CleanDirectorySeparators(sdkFullRoot, '/').TrimEnd(DirSeparators) + "/";
					if (System.IO.Directory.Exists(sdkFullRoot + "source_tree"))
					{
						CurrentSDK = SDK.Full;
						sdkRoot = sdkFullRoot;
						// assume that
						IAmDeveloping = true;
					}
					else
					{
						LogError("Found PK_SDK_ROOT but does not seem valid !");
					}
				}
			}

			if (CurrentSDK == SDK.No || string.IsNullOrEmpty(sdkRoot))
			{
				Log("SDK not found, continuing without.");
				return true;
			}

			if (IAmDeveloping)
			{
				// maybe not faster, but we want to make sure there is no missing includes
				bUseUnity = false;
				PrivatePCHHeaderFile = "Private/EmptyPCH.h";
			}

			string		clientName = DefaultClientName;
			string		binDir = sdkRoot + "source_tree/Runtime/bin/";
			if (!System.IO.Directory.Exists(binDir + clientName))
			{
				string[]	clients = System.IO.Directory.GetDirectories(binDir);
				if (clients == null || clients.Length == 0)
				{
					LogError("Invalid PopcornFX SDK, nothing found in " + binDir);
					return false;
				}
				clientName = System.IO.Path.GetFileName(clients[0].TrimEnd(DirSeparators));
			}

			string		sourceDir = sdkRoot + "source_tree/Runtime/";
			string		clientLibDir = binDir + clientName + "/";

			PublicIncludePaths.AddRange(
				new string[] {
					sourceDir,
					sourceDir + "include/",
					sourceDir + "include/license/" + clientName + "/",
					//sourceDir + "../HellHeaven-SDK/Samples/IntegrationUnrealEngine/",
				}
			);

			string		configShort = null;
			switch (Target.Configuration)
			{
				case UnrealTargetConfiguration.Unknown:
					Debug.Assert(false, "Error Unknown Target Configuration");
					break;
				case UnrealTargetConfiguration.Debug:
					configShort = "d";
					AddDefinition("PK_DEBUG");
					break;
				case UnrealTargetConfiguration.DebugGame:
					configShort = "d";
					AddDefinition("PK_DEBUG");
					break;
				case UnrealTargetConfiguration.Development:
					configShort = "r";
					AddDefinition("PK_RELEASE");
					break;
				case UnrealTargetConfiguration.Shipping:
					configShort = "s";
					AddDefinition("PK_RETAIL");
					break;
				case UnrealTargetConfiguration.Test:
					configShort = "s";
					AddDefinition("PK_RETAIL");
					break;
			}
			Debug.Assert(!String.IsNullOrEmpty(configShort), "Error: invalid configuration");

			bool	isUNKNOWNCompatible = false;
			bool	isWinUNKNOWN = false;
			bool	isXboxOneUNKNOWN = false;
			bool	isUNKNOWN = false;
			bool	isUNKNOWN2Compatible = false;
			bool	isUNKNOWN2 = false;

			// Makes sure the plugin is compatible with Vanilla UnrealEngine
			isUNKNOWNCompatible = UnrealTargetPlatform.IsValidName("WinUNKNOWN") || UnrealTargetPlatform.IsValidName("XboxOneUNKNOWN") || UnrealTargetPlatform.IsValidName("UNKNOWN");

			isWinUNKNOWN = isUNKNOWNCompatible && (Target.Platform.ToString() == "WinUNKNOWN");
			isXboxOneUNKNOWN = isUNKNOWNCompatible && (Target.Platform.ToString() == "XboxOneUNKNOWN");
			isUNKNOWN = isUNKNOWNCompatible && (Target.Platform.ToString() == "UNKNOWN");
			isUNKNOWN2Compatible = UnrealTargetPlatform.IsValidName("UNKNOWN2");
			isUNKNOWN2 = isUNKNOWN2Compatible && (Target.Platform.ToString() == "UNKNOWN2");

			if (isUNKNOWNCompatible)
				Log("Detected UNKNOWN compatible Unreal Engine");
			if (isUNKNOWN2Compatible)
				Log("Detected UNKNOWN2 compatible Unreal Engine");

			string		libPrefix = "";
			string		libExt = "";
			bool		processLibs = true;
#if !UE_5_0_OR_LATER // Support dropped with UE5
			if (Target.Platform == UnrealTargetPlatform.Win32)
			{
				libPrefix = clientLibDir + "vs2017_Win32/";
				libExt = ".lib";
			}
			else
#endif // !UE_5_0_OR_LATER // Support dropped with UE5
			if (Target.Platform == UnrealTargetPlatform.Win64 ||
				isWinUNKNOWN) // Win32 UNKNOWN (WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP), just link with the same libs as Win64
			{
				libPrefix = clientLibDir + "vs2017_x64/";
				libExt = ".lib";
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				libPrefix = clientLibDir + "gmake_macosx_x64/lib";
				libExt = ".a";
			}
#if !UE_5_0_OR_LATER // Support dropped with UE5
			else if (Target.Platform == UnrealTargetPlatform.XboxOne)
			{
				libPrefix = clientLibDir + "vs2017_Durango/";
				libExt = ".lib";
			}
#endif // !UE_5_0_OR_LATER
			else if (isXboxOneUNKNOWN)
			{
				libPrefix = clientLibDir + "vs2017_UNKNOWN.x64/";
				libExt = ".lib";
			}
			else if (isUNKNOWN)
			{
				libPrefix = clientLibDir + "vs2017_UNKNOWN.x64/";
				libExt = ".lib";
			}
#if !UE_5_0_OR_LATER // Support dropped with UE5
			else if (Target.Platform == UnrealTargetPlatform.PS4)
			{
				libPrefix = clientLibDir + "vs2017_ORBIS/";
				// "vs" + WindowsPlatform.GetVisualStudioCompilerVersionName(); // error (exception) on >= 4.16
				libExt = ".a";
			}
#endif // !UE_5_0_OR_LATER
			else if (isUNKNOWN2)
			{
				libPrefix = clientLibDir + "vs2017_UNKNOWN2/";
				libExt = ".a";
			}
			else if (Target.Platform == UnrealTargetPlatform.IOS)
			{
				// UE4 starts dropping armv7 from UE4.16, only link with arm64
				libPrefix = clientLibDir + "gmake_ios64/lib";
				libExt = ".a";
			}
#if !UE_5_0_OR_LATER // Support dropped with UE5
			else if (Target.Platform == UnrealTargetPlatform.Lumin)
			{
				libPrefix = clientLibDir + "gmake_android64/lib";
				libExt = ".a";
			}
#endif // !UE_5_0_OR_LATER
			else if (Target.Platform == UnrealTargetPlatform.Android)
			{
				processLibs = false;
				foreach (string lib in PkLibs)
				{
#if !UE_5_0_OR_LATER // armv7 dropped with UE5
					// Multiple Architectures ! (armeabi, arm64)
					// Using PublicLibraryPaths so ld will "skipping incompatible" libraries
					PublicAdditionalLibraries.Add(clientLibDir + "gmake_android/lib" + lib + "_" + configShort + ".a");     // armv7 (armeabi)
#endif
					PublicAdditionalLibraries.Add(clientLibDir + "gmake_android64/lib" + lib + "_" + configShort + ".a");   // armv8 (arm64)
				}
				libPrefix = "";
				libExt = "";
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				libPrefix = clientLibDir + "gmake_linux_x64/lib";
				libExt = ".a";
			}
#if !UE_5_0_OR_LATER // Support dropped with UE5
			else if (Target.Platform == UnrealTargetPlatform.Switch)
			{
				libPrefix = clientLibDir + "vs2017_NX64/";
				libExt = ".a";
			}
#endif // !UE_5_0_OR_LATER
			else
			{
				LogError("Target Platform " + Target.Platform.ToString() + " not supported by PopcornFX");
				Debug.Assert(false, "Platform not supported");
			}

			Log("SDK " + CurrentSDK + " - " +  String.Join(", ", PublicAdditionalLibraries) + "" + libPrefix + " ... _" + configShort + libExt);
			if (processLibs)
			{
				foreach (string lib in PkLibs)
				{
					PublicAdditionalLibraries.Add(libPrefix + lib + "_" + configShort + libExt);
				}
			}

			if (Target.bBuildEditor == true)
			{
				foreach (string lib in PkLibsEditorOnly)
				{
					PublicAdditionalLibraries.Add(libPrefix + lib + "_" + configShort + libExt);
				}

				string		toolsSourceDir = sdkRoot + "source_tree/Tools/";
				PublicIncludePaths.AddRange(
					new string[] {
					toolsSourceDir,
					}
				);
			}
			// Depth based collisions tests: To remove for 2.8 release
			PublicIncludePaths.Add("Runtime/Renderer/Private");

			bool	d3d12Compat = false;
			bool	d3d11Compat = false;
			if (
#if !UE_5_0_OR_LATER // Support dropped with UE5
				Target.Platform == UnrealTargetPlatform.Win32 ||
#endif // !UE_5_0_OR_LATER // Support dropped with UE5
				Target.Platform == UnrealTargetPlatform.Win64 || isWinUNKNOWN)
			{
				if (!isWinUNKNOWN)
					d3d11Compat = true;
				d3d12Compat = true;
				PublicSystemLibraries.AddRange(
					new string[] {
						"Version.lib",
						"Psapi.lib",
					});
			}
			else if (
#if !UE_5_0_OR_LATER // Support dropped with UE5
					Target.Platform == UnrealTargetPlatform.XboxOne ||
#endif // !UE_5_0_OR_LATER
					isXboxOneUNKNOWN || isUNKNOWN)
			{
				if (isUNKNOWN)
				{
					// Missing definitions while PopcornFX SDK includes those headers. To remove once patched in the SDK
					AddDefinition("DECLARE_UMD_EXPORTS_D3D12X=0");
					AddDefinition("D3DCOMPILE_NO_DEBUG=0");
				}
				// d3d12Compat = true; GPU sim is disabled for now
				PublicSystemLibraries.AddRange(
					new string[] {
						"Version.lib",
						"Psapi.lib",
					});
			}
			if (d3d11Compat)
				Log("D3D11 GPU sim is enabled");
			if (d3d12Compat)
				Log("D3D12 GPU sim is enabled");

			bool	hasGPU = d3d11Compat || d3d12Compat;

			// Can we bake the GPU sim for those platforms?
			bool	hasBackend_GPU = false;
			bool	hasBackend_D3D = false;
			bool	hasBackend_UNKNOWN2 = false;
			if (Target.bBuildEditor)
			{
				string		backendPath_D3D = libPrefix + "PK-Plugin_CompilerBackend_GPU_D3D" + "_" + configShort + libExt;

				hasBackend_D3D = System.IO.File.Exists(backendPath_D3D);
				hasBackend_GPU = hasBackend_D3D || hasBackend_UNKNOWN2;
				if (hasBackend_D3D)
				{
					Log("D3D GPU backend enabled for baking");

					PublicAdditionalLibraries.Add(backendPath_D3D); // Only necessary to link with GPU backends for baking

					// TODO: Is this still required ?
					// Required when linking with PK-AssetBakerLib
					PublicSystemLibraries.AddRange(new string[]
						{
							"d3dcompiler.lib",
							"dxguid.lib"
						});
				}
			}
			// Disabled until properly tested on all console platforms
			const bool	enableConsoleGPUSim = false;

			Add01DefinitionFromValue("PK_COMPILE_GPU", hasBackend_GPU);
			Add01DefinitionFromValue("PK_COMPILE_GPU_PC", hasBackend_D3D);
			Add01DefinitionFromValue("PK_COMPILE_GPU_XBOX_ONE", hasBackend_D3D && isUNKNOWNCompatible && enableConsoleGPUSim);
			Add01DefinitionFromValue("PK_COMPILE_GPU_UNKNOWN", hasBackend_D3D && isUNKNOWNCompatible && enableConsoleGPUSim);
			Add01DefinitionFromValue("PK_COMPILE_GPU_D3D", hasBackend_D3D);
			Add01DefinitionFromValue("PK_COMPILE_GPU_UNKNOWN2", hasBackend_UNKNOWN2 && isUNKNOWN2Compatible && enableConsoleGPUSim);

			Add01DefinitionFromValue("PK_GPU_D3D11", d3d11Compat);
			Add01DefinitionFromValue("PK_GPU_D3D12", d3d12Compat);
			if (d3d11Compat)
			{
				PrivateDependencyModuleNames.Add("D3D11RHI");

				// TODO: Is this still required ?
				AddDefinition("PK_PARTICLES_UPDATER_USE_D3D11=1");
				AddDefinition("PK_PARTICLES_UPDATER_USE_D3D12=1");
			}
			if (d3d12Compat)
			{
				if (
#if !UE_5_0_OR_LATER // Support dropped with UE5
					Target.Platform == UnrealTargetPlatform.Win32 ||
#endif // !UE_5_0_OR_LATER // Support dropped with UE5
					Target.Platform == UnrealTargetPlatform.Win64 || isWinUNKNOWN)
					AddEngineThirdPartyPrivateStaticDependencies(Target, "NVAftermath");
				else
					AddDefinition("NV_AFTERMATH=0");
				
#if UE_5_2_OR_LATER
				// D3D12RHIPrivate.h
				if (Target.Platform == UnrealTargetPlatform.Win64 || isWinUNKNOWN)
					PublicDefinitions.Add("INTEL_EXTENSIONS=1");
				else
					PublicDefinitions.Add("INTEL_EXTENSIONS=0");
#endif // UE_5_2_OR_LATER

				// TODO: Is this still required ?
				AddDefinition("PK_PARTICLES_UPDATER_USE_D3D12=1");

				PrivateDependencyModuleNames.Add("D3D12RHI");

				// Needed for RHI buffers creation from native resources
				PublicIncludePaths.Add("Runtime/D3D12RHI/Private");
				if (
#if !UE_5_0_OR_LATER // Support dropped with UE5
					Target.Platform == UnrealTargetPlatform.XboxOne ||
#endif // !UE_5_0_OR_LATER
					isXboxOneUNKNOWN || isUNKNOWN)
				{
					PublicIncludePaths.Add("../Platforms/XboxCommon/Source/Runtime/D3D12RHI/Private");
					if (isXboxOneUNKNOWN)
						PublicIncludePaths.Add("../Platforms/XboxOneUNKNOWN/Source/Runtime/D3D12RHI/Private");
					else if (isUNKNOWN)
						PublicIncludePaths.Add("../Platforms/UNKNOWN/Source/Runtime/D3D12RHI/Private");
				}
				else
					AddEngineThirdPartyPrivateStaticDependencies(Target, "DX12");
			}

#if !UE_5_0_OR_LATER // Support dropped with UE5
			if (Target.Platform == UnrealTargetPlatform.PS4)
			{
				PrivateDependencyModuleNames.Add("PS4RHI");
			}
#endif // !UE_5_0_OR_LATER
			else if (
#if !UE_5_0_OR_LATER // Support dropped with UE5
					Target.Platform == UnrealTargetPlatform.XboxOne ||
#endif // !UE_5_0_OR_LATER
					isXboxOneUNKNOWN || isUNKNOWN)
			{
				PrivateDependencyModuleNames.Add("D3D12RHI");
			}

			Add01DefinitionFromValue("PK_HAS_GPU", hasGPU);
			return true;
		}

#if WITH_FORWARDED_MODULE_RULES_CTOR
		private bool			SetupPlugin(ReadOnlyTargetRules Target)
#else
		private bool			SetupPlugin(TargetInfo Target)
#endif // WITH_FORWARDED_MODULE_RULES_CTOR
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PublicIncludePaths.AddRange(
				new string[] {
					//"Runtime/PopcornFX/Public",
					//"Runtime/PopcornFX/Classes",
				}
				);
			PrivateIncludePaths.AddRange(
				new string[] {
					"PopcornFX/Private",
				}
				);
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
				);
			PrivateIncludePathModuleNames.AddRange(
				new string[] {
					"Settings",
				}
				);
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"InputCore",
					"CoreUObject",
					"Engine",
					"Renderer",
					"RenderCore",
					"RHI",
					"MovieScene",
					"MovieSceneTracks",
#if UE_4_26_OR_LATER // Chaos integration
					"PhysicsCore",
					"Chaos",
#endif

#if UE_5_0_OR_LATER
						"RHICore", // D3D12 GPU sim: include D3D12RHIPrivate.h
#endif // UE_5_0_OR_LATER

					"ClothingSystemRuntimeCommon",
					"Projects", // Since 4.21
				}
				);

#if !UE_5_1_OR_LATER
			bool compileWithPhysX = Target.bCompilePhysX;
#else
			bool compileWithPhysX = false;
#endif // !UE_5_1_OR_LATER
#if !UE_5_0_OR_LATER // Support dropped with UE5
			if (Target.Platform == UnrealTargetPlatform.Lumin)
				compileWithPhysX = false;
#endif // UE_5_0_OR_LATER
			if (compileWithPhysX)
				PrivateDependencyModuleNames.Add("PhysX");
			if (Target.bBuildEditor)
			{
				//AddDefinition("MY_WITH_EDITOR=1");
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"UnrealEd",
						"AssetTools",
						"DirectoryWatcher",
						"PropertyEditor",
						"SceneOutliner",
						"Slate",
						"SlateCore",
						"EditorStyle",
						"EditorWidgets",
						"PlacementMode",
						"MessageLog",
						"LevelEditor",
						"Sequencer",
						"MovieSceneTools",
						"AppFramework",

						"TargetPlatform", // Baking

						"DesktopPlatform", // Profiler HUD ask for a path to save profiler report
					}
					);
			}

			// Raytracing
			var		engineDir = Path.GetFullPath(Target.RelativeEnginePath);
			PrivateIncludePaths.Add(Path.Combine(engineDir, "Shaders/Shared"));

			// UE4.25: No backward compatible files anymore.. keep this just in case
			// SetupBackwardCompatFiles();

			return true;
		}

#if WITH_FORWARDED_MODULE_RULES_CTOR
		public PopcornFX(ReadOnlyTargetRules Target) : base(Target)
#else
		public PopcornFX(TargetInfo Target)
#endif // WITH_FORWARDED_MODULE_RULES_CTOR
		{
			if (SetupPopcornFX(Target))
			{
				SetupPlugin(Target);
			}
		}
	}
}
