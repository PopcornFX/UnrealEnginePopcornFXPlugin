//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXPlugin.h"

#include "Assets/PopcornFXFile.h"
#include "PopcornFXSettingsEditor.h"
#include "HardwareInfo.h"
#include "Internal/Startup.h"
#include "Internal/PopcornFXProfiler.h"
#include "GPUSim/PopcornFXGPUSimInterfaces.h"
#include "PopcornFXVersionGenerated.h"
#if WITH_EDITOR
#	include "Editor/PopcornFXEffectEditor.h"
#endif

#include "ShaderCore.h"

#include "EngineModule.h"
#include "Engine/Texture.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#if WITH_EDITOR
#	include <pk_kernel/include/kr_resources.h>
#	include <pk_geometrics/include/ge_mesh_resource_handler.h>
#	include <pk_geometrics/include/ge_rectangle_list.h>
#	include <pk_imaging/include/im_resource.h>
#	include <pk_particles/include/ps_font_metrics_resource.h>
#	include <pk_particles/include/ps_vectorfield_resource.h>
#	include <pk_particles/include/ps_project_settings.h>	// for SBuildVersion::AllStaticPlatformBuildTags()
#endif
#include "Misc/CoreDelegates.h"

#include "Interfaces/IPluginManager.h"

#include "PopcornFXSDK.h"
#include <pk_particles/include/ps_system.h>
#include <pk_particles/include/ps_simulation_interface.h>
#include <pk_compiler/include/cp_binders.h> // cpu sim interfaces
#include <pk_toolkit/include/pk_toolkit_version.h>
#include <pk_kernel/include/kr_resources.h>

#if WITH_HAVOK_PHYSICS
#include "HavokDllSync.h"
#endif

//----------------------------------------------------------------------------

IMPLEMENT_MODULE(FPopcornFXPlugin, PopcornFX)

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXPlugin, Log, All);
#define LOCTEXT_NAMESPACE "PopcornFXPlugin"

//----------------------------------------------------------------------------

FPopcornFXPlugin				*FPopcornFXPlugin::s_Self = null;
float							FPopcornFXPlugin::s_GlobalScale = 1.f;
float							FPopcornFXPlugin::s_GlobalScaleRcp = 1.f;
FString							FPopcornFXPlugin::s_PluginVersionString;
FString							FPopcornFXPlugin::s_PopornFXRuntimeVersionString;
uint32							FPopcornFXPlugin::s_PopornFXRuntimeRevID = 0;
uint16							FPopcornFXPlugin::s_PopornFXRuntimeMajMinPatch = 0;

FString							FPopcornFXPlugin::s_URLDocumentation = "https://www.popcornfx.com/docs/popcornfx-v2/plugins/ue4-plugin/";
FString							FPopcornFXPlugin::s_URLPluginWiki = "https://wiki.popcornfx.com/";
FString							FPopcornFXPlugin::s_URLDiscord = "https://discord.gg/4ka27cVrsf";

namespace
{
	s32			g_RenderThreadId = -1;
	FString		g_PopcornFXPluginRoot;
	FString		g_PopcornFXPluginContentDir;

} // namespace

namespace
{
	PopcornFX::TAtomic<s32>	s_TotalParticleCount = 0;
}

//----------------------------------------------------------------------------

//static
PopcornFX::SEngineVersion		FPopcornFXPlugin::PopcornFXBuildVersion()
{
	PopcornFX::SDllVersion		version; // ctor default to build version
	return PopcornFX::SEngineVersion(version.Major, version.Minor, version.Patch, version.RevID);
}

//----------------------------------------------------------------------------

// static
int32	FPopcornFXPlugin::TotalParticleCount()
{
	const s32	c = s_TotalParticleCount.Load();
	PK_ASSERT(c >= 0);
	return c;
}

//----------------------------------------------------------------------------

// static
void	FPopcornFXPlugin::IncTotalParticleCount(s32 incCount)
{
	s_TotalParticleCount.Add(incCount);
	PK_ASSERT(s_TotalParticleCount.Load() >= 0);
}

//----------------------------------------------------------------------------

// static
void	FPopcornFXPlugin::RegisterRenderThreadIFN()
{
	LLM_SCOPE(ELLMTag::Particles);
	PopcornFX::CThreadID		renderThreadId = PopcornFX::CCurrentThread::ThreadID();
	if (!renderThreadId.Valid())
	{
		//PK_ASSERT(g_RenderThreadId < 0);
		if (g_RenderThreadId >= 0)
		{
			PopcornFX::CThreadManager::UnsafeUnregisterUserThread(g_RenderThreadId);
			g_RenderThreadId = -1;
		}
		PK_ASSERT(g_RenderThreadId < 0);
		renderThreadId = PopcornFX::CCurrentThread::RegisterUserThread();
		if (!PK_VERIFY(renderThreadId.Valid()))
			return;
		g_RenderThreadId = renderThreadId;
	}
	PK_ASSERT(g_RenderThreadId >= 0);
	if (!PK_VERIFY(u32(g_RenderThreadId) == u32(renderThreadId)))
		g_RenderThreadId = renderThreadId; // force anyway
}

//----------------------------------------------------------------------------

static PopcornFX::TAtomic<u32>		g_RegisteredUserThreadsCount = 0;
static u32							g_RegisteredUserThreads[PopcornFX::CThreadManager::MaxThreadCount] = {0};

void	FPopcornFXPlugin::RegisterCurrentThreadAsUserIFN()
{
	LLM_SCOPE(ELLMTag::Particles);
	if (PK_PREDICT_LIKELY(PopcornFX::CCurrentThread::ThreadID().Valid()))
		return;
	PopcornFX::CThreadID	tid = PopcornFX::CCurrentThread::RegisterUserThread();
	if (!PK_VERIFY(tid.Valid()))
		return;
	const u32		i = g_RegisteredUserThreadsCount.FetchInc();
	const u32		maxUserThreads = u32(PK_ARRAY_COUNT(g_RegisteredUserThreads));
	if (!PK_VERIFY_MESSAGE(i < maxUserThreads, "Exeeded the maximum of %d user threads", maxUserThreads))
		return;
	PK_ASSERT(tid > 0); // 0 is main thread
	g_RegisteredUserThreads[i] = u32(tid);
}

//----------------------------------------------------------------------------

// static
bool	FPopcornFXPlugin::IsMainThread()
{
	return PopcornFX::CCurrentThread::IsMainThread();
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
//static
const FString	&FPopcornFXPlugin::PopcornFXPluginRoot()
{
	check(!g_PopcornFXPluginRoot.IsEmpty());
	return g_PopcornFXPluginRoot;
}
//static
const FString	&FPopcornFXPlugin::PopcornFXPluginContentDir()
{
	check(!g_PopcornFXPluginContentDir.IsEmpty());
	return g_PopcornFXPluginContentDir;
}

//----------------------------------------------------------------------------

struct	SBakeContext
{
	class PopcornFX::CResourceHandlerMesh			*m_BakeResourceMeshHandler;
	class PopcornFX::CResourceHandlerImage			*m_BakeResourceImageHandler;
	class PopcornFX::CResourceHandlerRectangleList	*m_BakeResourceRectangleListHandler;
	class PopcornFX::CResourceHandlerFontMetrics	*m_BakeResourceFontMetricsHandler;
	class PopcornFX::CResourceHandlerVectorField	*m_BakeResourceVectorFieldHandler;
	class PopcornFX::IFileSystem					*m_BakeFSController;
	class PopcornFX::CResourceManager				*m_BakeResourceManager;
	class PopcornFX::HBO::CContext					*m_BakeContext;

	SBakeContext()
	: m_BakeResourceMeshHandler(null)
	, m_BakeResourceImageHandler(null)
	, m_BakeResourceRectangleListHandler(null)
	, m_BakeResourceFontMetricsHandler(null)
	, m_BakeResourceVectorFieldHandler(null)
	, m_BakeFSController(null)
	, m_BakeResourceManager(null)
	{
	}

	~SBakeContext()
	{
		if (m_BakeResourceManager != null)
		{
			PK_ASSERT(m_BakeResourceMeshHandler != null);
			PK_ASSERT(m_BakeResourceImageHandler != null);
			PK_ASSERT(m_BakeResourceVectorFieldHandler != null);
			PK_ASSERT(m_BakeResourceFontMetricsHandler != null);
			PK_ASSERT(m_BakeResourceRectangleListHandler != null);

			m_BakeResourceManager->UnregisterHandler<PopcornFX::CResourceMesh>(m_BakeResourceMeshHandler);
			m_BakeResourceManager->UnregisterHandler<PopcornFX::CImage>(m_BakeResourceImageHandler);
			m_BakeResourceManager->UnregisterHandler<PopcornFX::CRectangleList>(m_BakeResourceRectangleListHandler);
			m_BakeResourceManager->UnregisterHandler<PopcornFX::CFontMetrics>(m_BakeResourceFontMetricsHandler);
			m_BakeResourceManager->UnregisterHandler<PopcornFX::CVectorField>(m_BakeResourceVectorFieldHandler);
		}
		PK_SAFE_DELETE(m_BakeResourceMeshHandler);
		PK_SAFE_DELETE(m_BakeResourceImageHandler);
		PK_SAFE_DELETE(m_BakeResourceVectorFieldHandler);
		PK_SAFE_DELETE(m_BakeResourceFontMetricsHandler);
		PK_SAFE_DELETE(m_BakeResourceRectangleListHandler);
		PK_SAFE_DELETE(m_BakeContext);
		PK_SAFE_DELETE(m_BakeFSController);
		PK_SAFE_DELETE(m_BakeResourceManager);
	}

	bool	Init()
	{
		PK_ASSERT(m_BakeResourceMeshHandler == null);
		PK_ASSERT(m_BakeResourceImageHandler == null);
		PK_ASSERT(m_BakeResourceVectorFieldHandler == null);
		PK_ASSERT(m_BakeResourceFontMetricsHandler == null);
		PK_ASSERT(m_BakeResourceRectangleListHandler == null);
		PK_ASSERT(m_BakeFSController == null);
		PK_ASSERT(m_BakeResourceManager == null);

		// Keep this updated with all PopcornFX resource types
		m_BakeResourceMeshHandler = PK_NEW(PopcornFX::CResourceHandlerMesh);
		m_BakeResourceImageHandler = PK_NEW(PopcornFX::CResourceHandlerImage);
		m_BakeResourceRectangleListHandler = PK_NEW(PopcornFX::CResourceHandlerRectangleList);
		m_BakeResourceFontMetricsHandler = PK_NEW(PopcornFX::CResourceHandlerFontMetrics);
		m_BakeResourceVectorFieldHandler = PK_NEW(PopcornFX::CResourceHandlerVectorField);
		if (!PK_VERIFY(m_BakeResourceMeshHandler != null) ||
			!PK_VERIFY(m_BakeResourceImageHandler != null) ||
			!PK_VERIFY(m_BakeResourceRectangleListHandler != null) ||
			!PK_VERIFY(m_BakeResourceFontMetricsHandler != null) ||
			!PK_VERIFY(m_BakeResourceVectorFieldHandler != null))
			return false;

		m_BakeFSController = PopcornFX::File::NewInternalFileSystem();
		if (!PK_VERIFY(m_BakeFSController != null))
			return false;

		m_BakeResourceManager = PK_NEW(PopcornFX::CResourceManager(m_BakeFSController));
		if (!PK_VERIFY(m_BakeResourceManager != null))
			return false;
		m_BakeResourceManager->RegisterHandler<PopcornFX::CResourceMesh>(m_BakeResourceMeshHandler);
		m_BakeResourceManager->RegisterHandler<PopcornFX::CImage>(m_BakeResourceImageHandler);
		m_BakeResourceManager->RegisterHandler<PopcornFX::CRectangleList>(m_BakeResourceRectangleListHandler);
		m_BakeResourceManager->RegisterHandler<PopcornFX::CFontMetrics>(m_BakeResourceFontMetricsHandler);
		m_BakeResourceManager->RegisterHandler<PopcornFX::CVectorField>(m_BakeResourceVectorFieldHandler);

		m_BakeContext = PK_NEW(PopcornFX::HBO::CContext(m_BakeResourceManager));
		if (!PK_VERIFY(m_BakeContext != null))
			return false;
		return true;
	}
};
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
//	Sim interfaces supported by the UE plugin
//
//----------------------------------------------------------------------------

struct	SPopcornFXSimulationInterface
{
	typedef bool(*CbSimInterfaceCompatible)();
	typedef bool(*CbSimInterfaceSetup)(const SPopcornFXSimulationInterface &simDef, PopcornFX::SSimulationInterfaceDefinition &outDef);
	typedef bool(*CbSimInterfaceEmitBuiltin)(PopcornFX::SSimulationInterfaceDefinition &def);
	typedef bool(*CbSimInterfaceLink)(PopcornFX::SSimulationInterfaceDefinition &def);

	bool						m_CoreLibTemplate; // Set to false for paths outside core lib files
	const char					*m_SimInterfacePath; // Only used when baking, not required at runtime. Not ifdefed to keep code clean
	const char					*m_SimInterfaceName;
	CbSimInterfaceCompatible	m_CompatibleCb;
	CbSimInterfaceSetup			m_SetupCb;
	CbSimInterfaceEmitBuiltin	m_EmitBuiltinsCb;
	CbSimInterfaceLink			m_LinkCb;
};

//----------------------------------------------------------------------------
// IntersectScene

bool	_SimInterfaceCompatible_IntersectScene()
{
#if (PK_GPU_D3D11 != 0) // This sim interface is only supported on D3D11 right now
	// Still has to check manually the currently loaded RHI, as PK_GPU_D3D11 can be defined when RHI is D3D12
	const FString	hardwareDetails = FHardwareInfo::GetHardwareDetailsString();
	const bool		D3D11RHI = FModuleManager::Get().IsModuleLoaded("D3D11RHI") && hardwareDetails.Contains("D3D11");

	return D3D11RHI;
#else
	return false;
#endif // (PK_GPU_D3D11 != 0)
}

bool	_SimInterfaceBuildDefinition_IntersectScene(const SPopcornFXSimulationInterface &simDef, PopcornFX::SSimulationInterfaceDefinition& outDef)
{
	PK_ASSERT(outDef.m_Inputs.Empty() && outDef.m_Outputs.Empty());	// double-init !

	outDef.m_FnNameBase = simDef.m_SimInterfaceName;
	outDef.m_Traits = PopcornFX::Compiler::F_StreamedReturnValue | PopcornFX::Compiler::F_Pure;
	outDef.m_Flags = PopcornFX::SSimulationInterfaceDefinition::Flags_Context_Scene; // We need the scene context to use view.project

	// Declare all inputs
	if (!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("Position", PopcornFX::Nodegraph::DataType_Float3, PopcornFX::Compiler::Attribute_Stream)).Valid() ||
		!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("RayDir", PopcornFX::Nodegraph::DataType_Float3, PopcornFX::Compiler::Attribute_Stream)).Valid() ||
		!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("RayLength", PopcornFX::Nodegraph::DataType_Float1, PopcornFX::Compiler::Attribute_Stream)).Valid() ||
		!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("SweepRadius", PopcornFX::Nodegraph::DataType_Float1, PopcornFX::Compiler::Attribute_Stream)).Valid() ||
		!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("Mask", PopcornFX::Nodegraph::DataType_Bool1, PopcornFX::Compiler::Attribute_Stream)).Valid() ||
		!outDef.m_Inputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueIn("CollisionFilter", PopcornFX::Nodegraph::DataType_Int1, PopcornFX::Compiler::Attribute_None)).Valid())
		return false;

	// Declare all outputs
	if (!outDef.m_Outputs.PushBack(PopcornFX::SSimulationInterfaceDefinition::SValueOut("RawISecResult", PopcornFX::Nodegraph::DataType_Float4, PopcornFX::Compiler::Attribute_Stream)).Valid())
		return false;
	return true;
}

bool	_SimInterfaceEmitBuiltins_IntersectScene(PopcornFX::SSimulationInterfaceDefinition &def)
{
	bool	success = true;
#if (PK_GPU_D3D != 0)
#	if WITH_EDITOR
	const PopcornFX::CStringId	backendName("GPU_D3D");

#if 0 // IntersectScene doesn't rely on view project anymore. Keeping here as reference for future sim interfaces
	const PopcornFX::CStringId		kDependencies[] {
		PopcornFX::CStringId("_view_project_f32x3_s32x1_CX"),
		PopcornFX::CStringId("_view_unproject_f32x3_s32x1_CX")
		};
	success &= PopcornFX::CLinkerGPU::Bind(backendName, PopcornFX::CStringId(def.GetCallNameMangledGPU(0)), &PopcornFXGPU::D3D::EmitBuiltin_IntersectScene, false, kDependencies);
#else
	success &= PopcornFX::CLinkerGPU::Bind(backendName, PopcornFX::CStringId(def.GetCallNameMangledGPU(0)), &PopcornFXGPU::D3D::EmitBuiltin_IntersectScene, false);
#endif

#endif // WITH_EDITOR
#endif // (PK_GPU_D3D != 0)
	return success;
}

bool	_SimInterfaceLink_IntersectScene(PopcornFX::SSimulationInterfaceDefinition &def)
{
	bool	success = true;
	// IntersectScene implementation on CPU is CParticleScene::RaytracePacket
	PopcornFX::Compiler::SBinding	defaultBinding = PopcornFX::CLinkerCPU::GetBinding(PopcornFX::CStringView(), "scene.sweepSphere_float3_float3_float_float_bool_int_SceneCtx");
	success &= PopcornFX::CLinkerCPU::Bind(def.GetCallNameMangledCPU(0), defaultBinding);

#if (PK_GPU_D3D != 0)
	const PopcornFX::CStringId	backendName("GPU_D3D");
	success &= PopcornFX::CLinkerGPU::BindLink(backendName, PopcornFX::CStringId("UE_SceneDepth"), &PopcornFXGPU::D3D::Bind_IntersectScene_SceneDepth);
	success &= PopcornFX::CLinkerGPU::BindLink(backendName, PopcornFX::CStringId("UE_SceneNormal"), &PopcornFXGPU::D3D::Bind_IntersectScene_SceneNormal);
	success &= PopcornFX::CLinkerGPU::BindLink(backendName, PopcornFX::CStringId("UE_ViewConstantBuffer"), &PopcornFXGPU::D3D::Bind_IntersectScene_ViewUniformBuffer);
#endif // (PK_GPU_D3D != 0)
	return success;
}

//----------------------------------------------------------------------------

static const	SPopcornFXSimulationInterface	kSimInterfaces[1] = {
		{ true, "PopcornFXCore/Templates/Dynamics.pkfx", "IntersectScene", &_SimInterfaceCompatible_IntersectScene, &_SimInterfaceBuildDefinition_IntersectScene, &_SimInterfaceEmitBuiltins_IntersectScene, &_SimInterfaceLink_IntersectScene }
	};

//----------------------------------------------------------------------------

FPopcornFXPlugin::FPopcornFXPlugin()
	: m_RootPackLoaded(false)
	, m_LaunchedPopcornFX(false)
	, m_Settings(null)
#if WITH_EDITORONLY_DATA
	, m_SettingsEditor(null)
	, m_PackIsUpToDate(false)
	, m_BakeContext(null)
#endif // WITH_EDITORONLY_DATA
#if 0
	, m_SceneDepthTexture(null)
	, m_SceneNormalTexture(null)
#endif
	, m_Profiler(null)
{
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::StartupModule()
{
	const FString	pluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("PopcornFX"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/Plugin/PopcornFX"), pluginShaderDir);

	s_Self = this;

	s_PluginVersionString = FString::Printf(TEXT("%d.%d.%d%s"), POPCORNFX_PLUGIN_VERSION_MAJOR, POPCORNFX_PLUGIN_VERSION_MINOR, POPCORNFX_PLUGIN_VERSION_PATCH, TEXT(POPCORNFX_PLUGIN_VERSION_TAG));
	PopcornFX::SDllVersion		pkfxVersion;
	s_PopornFXRuntimeVersionString = FString::Printf(TEXT("%d.%d.%d-%d%s"), pkfxVersion.Major, pkfxVersion.Minor, pkfxVersion.Patch, pkfxVersion.RevID, pkfxVersion.Debug ? TEXT("-DEBUG") : TEXT(""));
	s_PopornFXRuntimeRevID = pkfxVersion.RevID;

	{
		static const uint32			kMajBits = 2;
		static const uint32			kMinBits = 6;
		static const uint32			kPatchBits = 8;
		static_assert(kMajBits + kMinBits + kPatchBits == sizeof(s_PopornFXRuntimeMajMinPatch) * 8, "Invalid ver");
		check(pkfxVersion.Major > 0 && (pkfxVersion.Major - 1) < (1U << kMajBits));
		check(pkfxVersion.Minor < (1U << kMinBits));
		check(pkfxVersion.Patch < (1U << kPatchBits));
		s_PopornFXRuntimeMajMinPatch = pkfxVersion.Patch | (pkfxVersion.Minor << (kPatchBits)) | ((pkfxVersion.Major - 1) << (kPatchBits + kMinBits));
	}

	UE_LOG(LogPopcornFXPlugin, Log, TEXT("Starting up PopcornFX Plugin %s (PopcornFX Runtime %s) ..."), *s_PluginVersionString, *s_PopornFXRuntimeVersionString);

#if WITH_EDITOR
	{
		TSharedPtr<IPlugin>	pluginPopcornFX = IPluginManager::Get().FindPlugin(TEXT("PopcornFX"));
		if (!pluginPopcornFX.IsValid())
		{
			UE_LOG(LogPopcornFXPlugin, Fatal, TEXT("Could not find PopcornFX Plugin !"));
			return;
		}
		g_PopcornFXPluginRoot = pluginPopcornFX->GetBaseDir();
		check(!g_PopcornFXPluginRoot.IsEmpty());
		g_PopcornFXPluginContentDir = pluginPopcornFX->GetContentDir();
		check(!g_PopcornFXPluginContentDir.IsEmpty());
	}
#endif // WITH_EDITOR

	if (!m_LaunchedPopcornFX)
	{
		LLM_SCOPE(ELLMTag::Particles);
		m_LaunchedPopcornFX = PopcornFXStartup();
		if (!m_LaunchedPopcornFX)
		{
			UE_LOG(LogPopcornFXPlugin, Error, TEXT("Couldn't startup PopcornFX"));
		}
		FCoreDelegates::OnPostEngineInit.AddRaw(this, &FPopcornFXPlugin::OnPostEngineInit);
	}

#if WITH_EDITOR

#if (ENGINE_MAJOR_VERSION == 5)
	m_OnObjectSavedDelegateHandle = FCoreUObjectDelegates::OnObjectPreSave.AddRaw(this, &FPopcornFXPlugin::_OnObjectSaved);
#else
	m_OnObjectSavedDelegateHandle = FCoreUObjectDelegates::OnObjectSaved.AddRaw(this, &FPopcornFXPlugin::_OnObjectSaved);
#endif // (ENGINE_MAJOR_VERSION == 5)

#endif

#if WITH_HAVOK_PHYSICS
	HAVOK_REGISTER_SYNC();
#endif
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::OnPostEngineInit()
{
#if 0
	_LinkSimInterfaces();
#endif
}

//----------------------------------------------------------------------------

bool	FPopcornFXPlugin::_LinkSimInterfaces()
{
	for (const SPopcornFXSimulationInterface &simInterface : kSimInterfaces)
	{
		if (!simInterface.m_CompatibleCb())
			continue;
		// As soon as a sim interface is bound, it needs to be implemented on both backends
		// For now, keep the emit/link cb being null checks, if this changes
#if 0
#if !WITH_EDITOR
		if (simInterface.m_LinkCb == null)
			continue;
#endif // !WITH_EDITOR
#endif
		PopcornFX::SSimulationInterfaceDefinition	def;

		if (!PK_VERIFY(simInterface.m_SetupCb(simInterface, def)) ||
#if 1//WITH_EDITOR
			(simInterface.m_EmitBuiltinsCb != null && !PK_VERIFY(simInterface.m_EmitBuiltinsCb(def))) || // some sim interfaces might have no builtins to emit (ie. CPU VM)
#endif // WITH_EDITOR
			(simInterface.m_LinkCb != null && !PK_VERIFY(simInterface.m_LinkCb(def)))) // some sim interfaces might have no data to link (ie. GPU builtins)
		{
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

#if WITH_EDITOR

#	if (ENGINE_MAJOR_VERSION == 5)
	FCoreUObjectDelegates::OnObjectPreSave.Remove(m_OnObjectSavedDelegateHandle);
#	else
	FCoreUObjectDelegates::OnObjectSaved.Remove(m_OnObjectSavedDelegateHandle);
#	endif // (ENGINE_MAJOR_VERSION == 5)

	m_OnObjectSavedDelegateHandle.Reset();
#endif

	PK_ASSERT(s_TotalParticleCount.Load() >= 0);

#if POPCORNFX_PROFILER_ENABLED
	PK_SAFE_DELETE(m_Profiler);
#endif // POPCORNFX_PROFILER_ENABLED

	PK_ASSERT(g_RenderThreadId != 0);
	if (g_RenderThreadId > 0)
		PopcornFX::CThreadManager::UnsafeUnregisterUserThread(g_RenderThreadId);

	const u32	registeredUserThreadsCount = g_RegisteredUserThreadsCount.Load();
	g_RegisteredUserThreadsCount.Store(0);
	for (u32 i = 0; i < registeredUserThreadsCount; ++i)
		PopcornFX::CThreadManager::UnsafeUnregisterUserThread(g_RegisteredUserThreads[i]);

	// shutdown PopcornFX
	if (m_LaunchedPopcornFX)
	{
		m_LaunchedPopcornFX = false; // avoid doing things during shutdown

#if WITH_EDITOR
		PK_SAFE_DELETE(m_BakeContext);
#endif // WITH_EDITOR

		if (m_RootPackLoaded)
		{
			PK_ASSERT(PopcornFX::File::DefaultFileSystem() != null);
			PopcornFX::File::DefaultFileSystem()->UnmountAllPacks();
			m_RootPackLoaded = false;
			m_FilePack = null;
			m_FilePackPath.Empty();
		}
		PopcornFXShutdown();
	}
#if WITH_EDITOR
	m_SettingsEditor = null;
#endif // WITH_EDITOR
	m_Settings = null;
	s_Self = null;
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::PreUnloadCallback()
{
#if WITH_HAVOK_PHYSICS
	HAVOK_UNREGISTER_SYNC();
#endif // WITH_HAVOK_PHYSICS
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

const UPopcornFXSettingsEditor	*FPopcornFXPlugin::SettingsEditor()
{
	if (!PK_VERIFY(LoadSettingsAndPackIFN()))
		return null;
	return m_SettingsEditor;
}

//----------------------------------------------------------------------------

PopcornFX::HBO::CContext	*FPopcornFXPlugin::BakeContext()
{
	PK_ASSERT(m_LaunchedPopcornFX);
	if (m_BakeContext == null)
	{
		m_BakeContext = PK_NEW(SBakeContext);
		if (!PK_VERIFY(m_BakeContext != null) ||
			!m_BakeContext->Init())
			return null;

		LoadSettingsAndPackIFN();
	}
	return m_BakeContext->m_BakeContext;
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::_UpdateSimInterfaceBindings(const FString &libraryDir)
{
#if 0
	PopcornFX::CSimulationInterfaceMapper *simInterfaceMapper = PopcornFX::CSimulationInterfaceMapper::DefaultMapper();

	if (PK_VERIFY(simInterfaceMapper != null))
	{
		simInterfaceMapper->Clear();

		const PopcornFX::CString	pkLibDir = ToPk(libraryDir);

		for (const SPopcornFXSimulationInterface &simInterface : kSimInterfaces)
		{
			if (!simInterface.m_CompatibleCb())
				continue;
			PopcornFX::SSimulationInterfaceDefinition	def;
			if (PK_VERIFY(simInterface.m_SetupCb(simInterface, def)))
			{
				const PopcornFX::CString			simInterfacePath = simInterface.m_CoreLibTemplate ? pkLibDir / simInterface.m_SimInterfacePath : simInterface.m_SimInterfacePath;
				const PopcornFX::CStringUnicode		simInterfaceNameUnicode = PopcornFX::CStringUnicode::FromAscii(simInterface.m_SimInterfaceName);

				// Check binding, error when mistmatch
				if (!PK_VERIFY(PopcornFX::CSimulationInterfaceMapper::CheckBinding(simInterfacePath, simInterfaceNameUnicode, def, BakeContext())))
					UE_LOG(LogPopcornFXPlugin, Error, TEXT("Sim interface '%s' located in '%s' doesn't match definition or doesn't exist anymore."), simInterface.m_SimInterfaceName, simInterface.m_SimInterfacePath);

				if (!simInterfaceMapper->Bind(simInterfacePath, simInterfaceNameUnicode, def))
					UE_LOG(LogPopcornFXPlugin, Error, TEXT("Failed binding sim interface '%s' located in '%s'"), simInterface.m_SimInterfaceName, simInterface.m_SimInterfacePath);
			}
		}
	}
#endif
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

const UPopcornFXSettings	*FPopcornFXPlugin::Settings()
{
	if (!PK_VERIFY(LoadSettingsAndPackIFN()))
		return null;
	return m_Settings;
}

//----------------------------------------------------------------------------

PopcornFX::PFilePack	FPopcornFXPlugin::FilePack()
{
	PK_VERIFY(LoadSettingsAndPackIFN());
	return m_FilePack;
}

//----------------------------------------------------------------------------

#if WITH_EDITOR
void	FPopcornFXPlugin::RefreshFromEditorSettings()
{
	UPopcornFXSettingsEditor	*editorSettings = GetMutableDefault<UPopcornFXSettingsEditor>();
	check(editorSettings);
	m_SettingsEditor = editorSettings;

	if (m_BakeContext != null &&
		m_BakeContext->m_BakeContext != null &&
		m_BakeContext->m_BakeFSController != null)
	{
		m_BakeContext->m_BakeFSController->UnmountAllPacks();
		if (PK_VERIFY(m_BakeContext->m_BakeFSController->MountPack(ToPk(m_SettingsEditor->AbsSourcePackRootDir)) != null))
		{
			if (editorSettings->bSourcePackFound)
			{
				// Only update bindings when pack is found. we don't want to spam errors for users not having the source pack available (which is legit).
				// No source pack -> no effects import
				{
					// New project, lib dir might have changed, we need to update the sim interface bindings
					FString	libraryDir = editorSettings->AbsSourcePackLibraryDir;
					if (PK_VERIFY(FPaths::MakePathRelativeTo(libraryDir, *editorSettings->AbsSourcePackRootDir)))
						_UpdateSimInterfaceBindings(libraryDir);
					else
						UE_LOG(LogPopcornFXPlugin, Error, TEXT("Couldn't make LibraryDir relative path from source pack root directory. Sim interfaces will not be bound"));
				}
			}
		}
	}
}
#endif	// WITH_EDITOR

//----------------------------------------------------------------------------

bool	FPopcornFXPlugin::LoadSettingsAndPackIFN()
{
	if (!m_LaunchedPopcornFX) // could happen on shutdown
		return false;

#if WITH_EDITOR
	// Lazy init IFN
	BakeContext();
#endif

	if (m_Settings != null)
	{
#if WITH_EDITOR
		PK_ASSERT(m_SettingsEditor != null);
#endif
		PK_ASSERT(!m_RootPackLoaded || m_FilePack != null);
		return m_RootPackLoaded;
	}
#if WITH_EDITOR
	RefreshFromEditorSettings();
	PK_ASSERT(m_SettingsEditor != null);
#endif
	m_Settings = GetMutableDefault<UPopcornFXSettings>();
	check(m_Settings);

	s_GlobalScale = m_Settings->GlobalScale;
	s_GlobalScaleRcp = 1.f / s_GlobalScale;

	// Do not hot-reload Pack: Dangerous
	// Wait for next restart
	if (!m_RootPackLoaded)
	{
		PK_ASSERT(m_FilePack == null);

		const PopcornFX::CString	packPath = ToPk(m_Settings->PackMountPoint);
		PK_ASSERT(PopcornFX::File::DefaultFileSystem() != null);
		PopcornFX::PFilePack		pack = PopcornFX::File::DefaultFileSystem()->MountPack(packPath);
		if (!PK_VERIFY(pack != null))
			return false;

		m_FilePack = pack;
		m_FilePackPath = ToUE(m_FilePack->Path());
		m_FilePackPath /= "";
		PK_ASSERT(m_FilePackPath[m_FilePackPath.Len() - 1] == '/');
		m_RootPackLoaded = true;
	}
#if WITH_EDITOR
	m_PackIsUpToDate = (m_Settings->PackMountPoint / "") == m_FilePackPath;
	if (!m_PackIsUpToDate)
	{
		UE_LOG(LogPopcornFXPlugin, Warning, TEXT("PackMountPoint has changed, needs to restart editor !"));
	}
#endif

	PK_ASSERT(m_RootPackLoaded);
	PK_ASSERT(m_FilePack != null);
	return m_RootPackLoaded;
}

//----------------------------------------------------------------------------

FString		FPopcornFXPlugin::BuildPathFromPkPath(const char *pkPath, bool prependPackPath)
{
	return BuildPathFromPkPath(PopcornFX::CString(pkPath), prependPackPath);
}

//----------------------------------------------------------------------------

FString		FPopcornFXPlugin::BuildPathFromPkPath(const PopcornFX::CString &pkPath, bool prependPackPath)
{
	PK_NAMEDSCOPEDPROFILE_C("FPopcornFXPlugin::BuildPathFromPkPath", POPCORNFX_UE_PROFILER_COLOR);

	if (!LoadSettingsAndPackIFN() ||
		!PK_VERIFY(FilePack() != null))
		return FString();

	PopcornFX::CString	path = pkPath;
	if (path.Empty())
		return FString();
	if (prependPackPath)
	{
		if (PK_VERIFY(FilePack() != null))
		{
			if (path[0] != '/')
				path.Prepend("/");
			path.Prepend(FilePack()->Path());
		}
	}

	// remove extension
	PopcornFX::CFilePath::StripExtensionInPlace(path);

	// Replace '.'
	path = path.Replace('.', '_');
	PK_ASSERT(!path.Contains("//"));

	// re-append filename : "Toto/filename.filename"
	const PopcornFX::CGuid	lastSlash = path.FindLastOf('/');
	path += ".";
	if (lastSlash.Valid())
		path.Append(path.Data() + lastSlash + 1, path.Length() - lastSlash - 2 /*the '/' and '.'*/);
	else
		path.Append(path.Data(), path.Length() - 1 /*the '.'*/);

	return ToUE(path);
}

//----------------------------------------------------------------------------

namespace
{
	void		_SetupFile(const UPopcornFXFile *file, const PopcornFX::PCBaseObjectFile pkFile)
	{
		if (PK_VERIFY(pkFile != null))
		{
			PK_ASSERT(pkFile->InternalUserData() == null || pkFile->InternalUserData() == file);
			pkFile->SetInternalUserData(const_cast<UPopcornFXFile*>(file));
		}
	}
}

//----------------------------------------------------------------------------

PopcornFX::PBaseObjectFile		FPopcornFXPlugin::FindPkFile(const UPopcornFXFile *file)
{
	check(PopcornFX::HBO::g_Context != null);
	check(file->IsBaseObject());

	const PopcornFX::CStringView	pkFilePath = PopcornFX::CStringView::FromNullTerminatedString(file->PkPath());

	if (!pkFilePath.Empty())
	{
		PopcornFX::PBaseObjectFile	pkFile = PopcornFX::HBO::g_Context->FindFile(file->PkPath());
		if (pkFile != null) // find file only
			_SetupFile(file, pkFile);
		return pkFile;
	}
	return null; // Can occur at file UPopcornFXFile unload, if file import/bake failed
}

//----------------------------------------------------------------------------

PopcornFX::PBaseObjectFile		FPopcornFXPlugin::LoadPkFile(const UPopcornFXFile *file, bool reload)
{
	LLM_SCOPE(ELLMTag::Particles);
	check(PopcornFX::HBO::g_Context != null);
	check(file->IsBaseObject());

	if (reload)
		UnloadPkFile(file);

	const PopcornFX::CString	pkPath = file->PkPath();
	if (pkPath.Empty())
		return null;

	PopcornFX::PBaseObjectFile	pkFile = PopcornFX::HBO::g_Context->LoadFile(pkPath, reload);
	if (pkFile != null)
		_SetupFile(file, pkFile);
	return pkFile;
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::UnloadPkFile(const UPopcornFXFile *file)
{
	LLM_SCOPE(ELLMTag::Particles);
	PopcornFX::PBaseObjectFile	pkFile = FPopcornFXPlugin::Get().FindPkFile(file);

	if (pkFile != null)
	{
		pkFile->Unload();

		PK_ASSERT(pkFile->InternalUserData() == null || pkFile->InternalUserData() == file);
		pkFile->SetInternalUserData(null);
	}
}

//----------------------------------------------------------------------------

UPopcornFXFile		*FPopcornFXPlugin::GetPopcornFXFile(const PopcornFX::CBaseObjectFile *boFile)
{
	LLM_SCOPE(ELLMTag::Particles);
	if (boFile == null)
		return null;

	if (boFile->InternalUserData() == null)
	{
		UObject			*object = LoadUObjectFromPkPath(boFile->Path(), false);
		if (object == null)
			return null;
		UPopcornFXFile	*file = Cast<UPopcornFXFile>(object);
		if (!PK_VERIFY(file != null))
			return null;
		_SetupFile(file, boFile);
		return file;
	}

	UPopcornFXFile		*file = reinterpret_cast<UPopcornFXFile*>(boFile->InternalUserData());

	PK_ASSERT(Cast<UPopcornFXFile>(LoadUObjectFromPkPath(boFile->Path(), false)) == file);
	check(file == null || file->IsBaseObject());
	return file;
}

//----------------------------------------------------------------------------

UObject	*FPopcornFXPlugin::LoadUObjectFromPkPath(const PopcornFX::CString &pkPath, bool pathNotVirtual)
{
	LLM_SCOPE(ELLMTag::Particles);
	if (!m_LaunchedPopcornFX) // could happen at shutdown
		return null;

	if (!PK_VERIFY(!IsGarbageCollecting() && !GIsSavingPackage))
		return null;

#if WITH_EDITOR
	// Editor only (baking code is the only place where this occurs)
	PopcornFX::CStringView	ext = PopcornFX::CFilePath::ExtractExtension(PopcornFX::CStringView(pkPath));
	if (ext == "pkcf")
		return null;
#endif // WITH_EDITOR

	// about BuildPathFromPkPath(..., __ bool prependPackPath __):
	//	if pathNotVirtual: // real path
	//		already prepended with "/Game/"
	//	else // virtual path
	//		no "/Game/", just "Particles/": so prendpend "/Game/"

	FString	p = BuildPathFromPkPath(pkPath, !pathNotVirtual);
	if (p.IsEmpty())
		return null;

	UObject	*anyObject = ::FindObject<UObject>(null, *p);
	if (anyObject == null)
		anyObject = ::LoadObject<UObject>(null, *p);
	if (anyObject != null)
	{
		anyObject->ConditionalPostLoad();
		anyObject->ConditionalPostLoadSubobjects();
	}
	return anyObject;
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::NotifyObjectChanged(UObject *object)
{
	if (m_FilePack == null)
		return;

	if (!object->IsA<UTexture>() &&
		!object->IsA<UStaticMesh>() &&
		!object->IsA<USkeletalMesh>() &&
		true)
	{
		if (object->IsA<UPopcornFXFile>())
		{
			UPopcornFXFile		*file = Cast<UPopcornFXFile>(object);
			PK_ASSERT(file != null);
			if (file->PkExtension() != TEXT("pkat")) // do not trigger for base object
				return;
		}
		else
			return;
	}

	PK_ASSERT(!m_FilePackPath.IsEmpty());
	PK_ASSERT(m_FilePackPath[m_FilePackPath.Len() - 1] == '/');

	const FString		objectPath = object->GetPathName();
	if (!objectPath.StartsWith(m_FilePackPath))
		return;
	const FString				virtualPath = objectPath.Right(objectPath.Len() - m_FilePackPath.Len());
	const PopcornFX::CString	virtualPathPk(ToPk(virtualPath));
	PopcornFX::Resource::DefaultManager()->NotifyResourceChanged(PopcornFX::CFilePackPath(m_FilePack, virtualPathPk));
}

//----------------------------------------------------------------------------

bool	FPopcornFXPlugin::HandleSettingsModified()
{
#if WITH_EDITOR
	m_SettingsEditor = null; // LoadSettingsAndPackIFN force reload settings
#endif // WITH_EDITOR
	m_Settings = null; // LoadSettingsAndPackIFN force reload settings

	if (!LoadSettingsAndPackIFN())
		return false;

	if (m_Settings != null)
		m_OnSettingsChanged.Broadcast();

#if WITH_EDITOR
	return m_SettingsEditor != null && m_Settings != null;
#else
	return m_Settings != null;
#endif // WITH_EDITOR
}

//----------------------------------------------------------------------------
#if WITH_EDITOR

TSharedRef<class IPopcornFXEffectEditor>	FPopcornFXPlugin::CreateEffectEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, class UPopcornFXEffect* Effect)
{
	TSharedRef<FPopcornFXEffectEditor> EffectEditor(new FPopcornFXEffectEditor());
	EffectEditor->InitEffectEditor(Mode, InitToolkitHost, Effect);
	return EffectEditor;
}

#endif // WITH_EDITOR
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
#if WITH_EDITOR
namespace
{
	bool		g_SetAskImportAssetDependencies_YesAll = false;
	bool		g_SetAskBuildMeshData_YesAll = false;
}

void	FPopcornFXPlugin::SetAskImportAssetDependencies_YesAll(bool yesAll) { g_SetAskImportAssetDependencies_YesAll = yesAll; }
bool	FPopcornFXPlugin::AskImportAssetDependencies_YesAll() { return g_SetAskImportAssetDependencies_YesAll; }
void	FPopcornFXPlugin::SetAskBuildMeshData_YesAll(bool yesAll) { g_SetAskBuildMeshData_YesAll = yesAll; }
bool	FPopcornFXPlugin::AskBuildMeshData_YesAll() { return g_SetAskBuildMeshData_YesAll; }
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

#if WITH_EDITOR
#if (ENGINE_MAJOR_VERSION == 5)
void	FPopcornFXPlugin::_OnObjectSaved(UObject *object, FObjectPreSaveContext context)
#else
void	FPopcornFXPlugin::_OnObjectSaved(UObject *object)
#endif // (ENGINE_MAJOR_VERSION == 5)
{
	NotifyObjectChanged(object);
}
#endif // WITH_EDITOR

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

CPopcornFXProfiler		*FPopcornFXPlugin::Profiler()
{
#if POPCORNFX_PROFILER_ENABLED
	PK_ASSERT(IsMainThread());
	if (m_Profiler == null)
		m_Profiler = PK_NEW(CPopcornFXProfiler);
	return m_Profiler;
#else
	return null;
#endif // POPCORNFX_PROFILER_ENABLED
}

//----------------------------------------------------------------------------

void	FPopcornFXPlugin::ActivateEffectsProfiler(bool activate)
{
#if POPCORNFX_PROFILER_ENABLED
	PK_ASSERT(IsMainThread());
	m_EffectsProfilerActive = activate;
#endif // POPCORNFX_PROFILER_ENABLED
}

//----------------------------------------------------------------------------

bool	FPopcornFXPlugin::EffectsProfilerActive() const
{
#if POPCORNFX_PROFILER_ENABLED
	PK_ASSERT(IsMainThread());
	return m_EffectsProfilerActive;
#else
	return false;
#endif // POPCORNFX_PROFILER_ENABLED
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
