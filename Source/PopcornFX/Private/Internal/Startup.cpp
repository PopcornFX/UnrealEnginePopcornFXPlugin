//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "Startup.h"

#include "FileSystemController_UE.h"
#include "ResourceHandlerMesh_UE.h"
#include "ResourceHandlerImage_UE.h"
#include "ResourceHandlerVectorField_UE.h"
#include "PopcornFXSettings.h"
#include "PopcornFXStats.h"

#include "GenericPlatform/GenericPlatformMemory.h"
#include "HAL/PlatformAffinity.h"
#include "Misc/ConfigCacheIni.h"
#include "Render/PopcornFXRendererProperties.h"

#include "PopcornFXSDK.h"

#include <pk_version_base.h>
#include <pk_kernel/include/kr_init.h>
#include <pk_base_object/include/hb_init.h>
#include <pk_engine_utils/include/eu_init.h>
#include <pk_compiler/include/cp_init.h>
#include <pk_imaging/include/im_init.h>
#include <pk_geometrics/include/ge_init.h>
#include <pk_geometrics/include/ge_coordinate_frame.h>
#include <pk_geometrics/include/ge_billboards.h>
#include <pk_particles/include/ps_init.h>
#include <pk_particles_toolbox/include/pt_init.h>
#include <pk_render_helpers/include/rh_init.h>
#include <pk_render_helpers/include/basic_renderer_properties/rh_vertex_animation_renderer_properties.h>
#include <pk_kernel/include/kr_static_config_flags.h>

#include <pk_kernel/include/kr_mem_stats.h>

#include <pk_kernel/include/kr_buffer.h>
#include <pk_kernel/include/kr_assert_internals.h>

#include <pk_kernel/include/kr_thread_pool_details.h>
#include <pk_kernel/include/kr_thread_pool_default.h>

#include <pk_kernel/include/kr_plugins.h>

#include <pk_toolkit/include/pk_toolkit_version.h>

#if WITH_EDITOR
#	include <PK-AssetBakerLib/AssetBaker_Oven.h>
#	include <PK-AssetBakerLib/AssetBaker_Oven_HBO.h>
#endif // WITH_EDITOR

//----------------------------------------------------------------------------

DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXStartup, Log, All);
#define LOCTEXT_NAMESPACE "PopcornFXStartup"

//----------------------------------------------------------------------------

PK_LOG_MODULE_DECLARE();

//----------------------------------------------------------------------------

#ifdef	PK_PLUGINS_STATIC

PK_PLUGIN_DECLARE(CCompilerBackendCPU_VM);

#if (PK_COMPILE_GPU_D3D == 1)
PK_PLUGIN_DECLARE(CCompilerBackendGPU_D3D);
#endif

#	ifdef	PK_DEBUG
#		define	PK_PLUGIN_POSTFIX_BUILD		"_D"
#	else
#		define	PK_PLUGIN_POSTFIX_BUILD		""
#	endif
#	if	defined(PK_WINDOWS)
#		define	PK_PLUGIN_POSTFIX_EXT		".dll"
#	else
#		define	PK_PLUGIN_POSTFIX_EXT		""
#	endif
#endif

//----------------------------------------------------------------------------

void	AddDefaultGlobalListenersOverride(void *userHandle);

//----------------------------------------------------------------------------

#define DECLARE_POPCORNFX_SCOPED_DELEGATE(CallbackName, TriggerFunc)				\
	class POPCORNFX_API	FScoped##CallbackName##Impl									\
	{																				\
	public:																			\
		static void		FireCallback() { TriggerFunc; }								\
	};																				\
																					\
	typedef TScopedCallback<FScoped##CallbackName##Impl> FScoped##CallbackName;

//----------------------------------------------------------------------------

#if (KR_PROFILER_ENABLED != 0)

struct	SUEStatIdKey
{
	const char	*m_StatName = null;

	SUEStatIdKey() {}
	SUEStatIdKey(const char *statName) : m_StatName(statName) {}

	bool	Empty() const { return m_StatName == null; }

	bool	operator == (const SUEStatIdKey &other) const { return m_StatName == other.m_StatName; }
	bool	operator != (const SUEStatIdKey &other) const { return !(*this == other); }
};

struct	SUEStatIdEntry
{
	SUEStatIdKey	m_Key;
	TStatId			m_StatId;

	SUEStatIdEntry() {}
	SUEStatIdEntry(const SUEStatIdKey &key, TStatId value) : m_Key(key), m_StatId(value) {}

	bool	operator == (const SUEStatIdKey &other) const { return m_Key == other; }
	bool	operator != (const SUEStatIdKey &other) const { return !(*this == other); }

	bool	operator == (const SUEStatIdEntry &other) const { return m_Key == other.m_Key; }
	bool	operator != (const SUEStatIdEntry &other) const { return !(*this == other); }
};

template<typename _RawHasher>
class	PopcornFX::TTypeHasher<SUEStatIdEntry, _RawHasher>
{
public:
	PK_INLINE static u32	Hash(const SUEStatIdKey &value)
	{
		return _RawHasher::Hash(value.m_StatName, strlen(value.m_StatName));//sizeof(char*)); // hash pointer
	}
	PK_INLINE static u32	Hash(const SUEStatIdEntry &value)
	{
		return Hash(value.m_Key);
	}
};

template<typename _Hasher, typename _Controller>
class	PopcornFX::TFastHashMapTraits<SUEStatIdEntry, _Hasher, _Controller>
{
public:
	typedef SUEStatIdEntry	Type;
	typedef _Hasher			Hasher;
	typedef _Controller		Controller;

	static const Type	Invalid;
	static const Type	Probe;

	PK_FORCEINLINE static bool	Valid(const Type &value) { return !value.m_Key.Empty(); }
	PK_FORCEINLINE static bool	IsProbe(const Type &value) { return value.m_Key == Probe.m_Key; }
};

template<typename _Hasher, typename _Controller>
SUEStatIdEntry	const PopcornFX::TFastHashMapTraits<SUEStatIdEntry, _Hasher, _Controller>::Invalid = SUEStatIdEntry();
template<typename _Hasher, typename _Controller>
SUEStatIdEntry	const PopcornFX::TFastHashMapTraits<SUEStatIdEntry, _Hasher, _Controller>::Probe = SUEStatIdEntry(SUEStatIdKey((const char*)1), TStatId());

struct	SPopcornFXThreadProfileContext
{
	PopcornFX::TFastHashMap<SUEStatIdEntry>	m_StatsMap;
};

thread_local bool	g_PopcornFXProfileRecordReentryGuard = false;
bool				g_IsUETaskGraph = false;

DECLARE_POPCORNFX_SCOPED_DELEGATE(ProfileRecordReentryGuard, g_PopcornFXProfileRecordReentryGuard = false);

#define POPCORNFX_PROFILERECORD_REENTRY_GUARD(__failureExpr)	\
	if (g_PopcornFXProfileRecordReentryGuard)					\
		__failureExpr;											\
	g_PopcornFXProfileRecordReentryGuard = true;				\
	FScopedProfileRecordReentryGuard	_reentryGuard;			\
	_reentryGuard.Request();

#if defined(CPUPROFILERTRACE_ENABLED)
#	define PK_HAS_CPUPROFILERTRACE CPUPROFILERTRACE_ENABLED
#endif // defined(CPUPROFILERTRACE_ENABLED)

#endif // (KR_PROFILER_ENABLED != 0)

#ifndef PK_HAS_CPUPROFILERTRACE
#	define PK_HAS_CPUPROFILERTRACE 0
#endif

//----------------------------------------------------------------------------

namespace
{
#if	(PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)

	PopcornFX::TAtomic<u32>		g_GlobalAssertId = 0;
	void	PrettyFormatAssert_Unsafe(char *buffer, u32 bufferLen, PK_ASSERT_CATCHER_PARAMETERS)
	{
		PK_ASSERT_CATCHER_KILLARGS;
		using namespace PopcornFX;

		const CThreadID		threadId = CCurrentThread::ThreadID();
		const u32			globalAssertId = g_GlobalAssertId.FetchInc();

#if	(PK_CALL_SCOPE_ENABLED != 0)
		PopcornFX::CString	callCtxStr = PopcornFX::CCallContext::ReadStackToString();
		const char			*callCtxStrPtr = callCtxStr.Data();
#else
		const char			*callCtxStrPtr = null;
#endif	//(PK_CALL_SCOPE_ENABLED != 0)

		// pointer compare ok (compiler probably removed duplicated static strings, else we dont care)
		const bool		pexp = (expanded != failed);
		const bool		pmsg = (message != failed && message != expanded);
		const bool		cctx = callCtxStrPtr != null;
		SNativeStringUtils::SPrintf(
			buffer, bufferLen,
			"!! PopcornFX Assertion failed !!"
			"\nFile       : %s(%d)"
			"\nFunction   : %s(...)"
			"%s%s"// Message
			"\nCondition  : %s"
			"%s%s" // Expanded
			"%s%s" // Call context
			"\nThreadID   : %d"
			"\nAssertNum  : %d"
			"\n"
			, file, line
			, function
			, (pmsg	? "\nMessage    : " : ""), (pmsg ? message : "")
			, failed
			, (pexp	? "\nExpanded   : " : ""), (pexp ? expanded : "")
			, (cctx ? "\nCallContext: " : ""), (cctx ? callCtxStrPtr : "")
			, u32(threadId)
			, globalAssertId);
	}

	volatile bool							g_Asserting = false;
	PopcornFX::Threads::CCriticalSection	g_AssertLock;

	PopcornFX::Assert::EResult	AssertImpl(PK_ASSERT_CATCHER_PARAMETERS)
	{
		PK_ASSERT_CATCHER_KILLARGS;
		using namespace PopcornFX;

		PK_SCOPEDLOCK(g_AssertLock);

		if (g_Asserting) // assert recursion ! break now
		{
			UE_LOG(LogPopcornFXStartup, Warning, TEXT("!! PopcornFX Assertion Recursion !! %s(%d): '%s' '%s'"), file, line, failed, message);
#if defined(PK_DEBUG)
			PK_BREAKPOINT();
#endif
			return Assert::Result_Skip;
		}
		g_Asserting = true;

		char			_buffer[2048];
		PrettyFormatAssert_Unsafe(_buffer, sizeof(_buffer), PK_ASSERT_CATCHER_ARGUMENTS);
		const char		*perttyMessage = _buffer;
		UE_LOG(LogPopcornFXStartup, Warning, TEXT("%s"), ANSI_TO_TCHAR(perttyMessage));

		if (!IsClassLoaded<UPopcornFXSettings>() ||
			!GetMutableDefault<UPopcornFXSettings>()->EnableAsserts)
		{
			g_Asserting = false;
			return Assert::Result_Skip; // Only log
		}

		Assert::EResult		result = Assert::Result_Skip;

		// FIXME: on macosx, message box is queued on the main thread, we cannot do that ! need to workaround eg: spawn /usr/bin/osascript ?

#if PLATFORM_DESKTOP && (PLATFORM_WINDOWS || PLATFORM_LINUX)
		{
			EAppReturnType::Type	msgBoxRes;
			{
				// UE4 handles all WinProc messages then wait updates tasks before execute them >>> DEAD LOCK
				// GPumpingMessagesOutsideOfMainLoop = true should enqueue messages
				TGuardValue<bool>	pumpMessageGuard(GPumpingMessagesOutsideOfMainLoop, true);
				msgBoxRes = FPlatformMisc::MessageBoxExt(EAppMsgType::CancelRetryContinue, ANSI_TO_TCHAR(perttyMessage), TEXT("PopcornFX Assertion failed"));
			}
			switch (msgBoxRes)
			{
			case EAppReturnType::Cancel:
				result = Assert::Result_Break;
				break;
			case EAppReturnType::Retry:
				result = Assert::Result_Skip;
				break;
			case EAppReturnType::Continue:
				result = Assert::Result_Ignore;
				break;
			default:
				break;
			}
		}

#else // No msgbox avail

#	if defined(PK_DEBUG)
		if (FPlatformMisc::IsDebuggerPresent())
		{
			//
			// !! PopcornFX Assertion Failed !! (and no proper message box system)
			//
			// Look at the Log
			//
			PK_BREAKPOINT();

			// Ignore subsequent calls of this assert
			result = Assert::Result_Ignore;
		}
		else
		{
			// Just skip (dont break) (but still log all asserts)
			result = Assert::Result_Skip;
		}
#	else
		// Ignore subsequent calls of this asserts
		result = Assert::Result_Ignore;
#	endif

#endif

		{
			const TCHAR		*msg = null;
			switch (result)
			{
			case Assert::Result_Break:	msg = TEXT("! About to BREAK on this assert !"); break;
			case Assert::Result_Ignore:	msg = TEXT("Will ignore all subsequent calls of this assert"); break;
			case Assert::Result_Skip:	msg = null; break;
			}
			if (msg != null)
				UE_LOG(LogPopcornFXStartup, Warning, TEXT("Assert action: %s"), msg);
		}

		g_Asserting = false;

		return result;
	}

#endif // (PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)


	bool			StartupPlugins()
	{
		using namespace PopcornFX;

		bool		success = true;

#ifdef	PK_PLUGINS_STATIC
		{
			const char	*backendPath = "Plugins/CBCPU_VM" PK_PLUGIN_POSTFIX_BUILD PK_PLUGIN_POSTFIX_EXT;
			IPluginModule	*backend = StartupPlugin_CCompilerBackendCPU_VM();
			success &= (backend != null && CPluginManager::PluginRegister(backend, true, backendPath));
		}
#	if (PK_COMPILE_GPU_D3D == 1)
	// NOT IMPLEMENTED
		{
			const char	*backendPath = "Plugins/CBGPU_D3D" PK_PLUGIN_POSTFIX_BUILD PK_PLUGIN_POSTFIX_EXT;
			IPluginModule	*backend = StartupPlugin_CCompilerBackendGPU_D3D();
			success &= (backend != null && CPluginManager::PluginRegister(backend, true, backendPath));
		}
#	endif
		return success;
#else
		TODO
#endif
	}

	void			ShutdownPlugins()
	{
		using namespace PopcornFX;

#ifdef	PK_PLUGINS_STATIC
#if (PK_COMPILE_GPU_D3D == 1)
		{
			IPluginModule	*backend = GetPlugin_CCompilerBackendGPU_D3D();
			(backend != null && CPluginManager::PluginRelease(backend));
			ShutdownPlugin_CCompilerBackendGPU_D3D();
		}
#endif
		{
			IPluginModule	*backend = GetPlugin_CCompilerBackendCPU_VM();
			(backend != null && CPluginManager::PluginRelease(backend));
			ShutdownPlugin_CCompilerBackendCPU_VM();
		}
#else
		TODO
#endif
	}

	PopcornFX::IFileSystem	*PopcornFX_CreateNewFileSystem()
	{
		return PK_NEW(CFileSystemController_UE);
	}

	class	CWorkerThreadPool_UE;

	class	FPopcornFXTask
	{
	public:
		FPopcornFXTask(CWorkerThreadPool_UE *pool, PopcornFX::PAsynchronousJob job)
		:	m_Pool(pool)
		,	m_Task(job) { }

#if (ENGINE_MINOR_VERSION < 25)
		static const TCHAR				*GetTaskName() { return TEXT("PopcornFXTask"); }
#endif // (ENGINE_MINOR_VERSION < 25)
		FORCEINLINE static TStatId		GetStatId() { RETURN_QUICK_DECLARE_CYCLE_STAT(FPopcornFXTask, STATGROUP_TaskGraphTasks); }
		static ENamedThreads::Type		GetDesiredThread() { return ENamedThreads::AnyThread; }
		static ESubsequentsMode::Type	GetSubsequentsMode() { return ESubsequentsMode::TrackSubsequents; }
		void							DoTask(ENamedThreads::Type currentThread, const FGraphEventRef &myCompletionGraphEvent);

	private:
		CWorkerThreadPool_UE			*m_Pool;
		PopcornFX::PAsynchronousJob		m_Task;
	};

	class	CWorkerThreadPool_UE : public PopcornFX::Threads::CAbstractPool
	{
	public:
		CWorkerThreadPool_UE()
		{
			PopcornFX::Mem::Clear(m_ThreadContexts.Data(), m_ThreadContexts.CoveredBytes());
#if (KR_PROFILER_ENABLED != 0)
			PopcornFX::Mem::Clear(m_ThreadProfileContexts.Data(), m_ThreadProfileContexts.CoveredBytes());
#endif // (KR_PROFILER_ENABLED != 0)
		}

		virtual	~CWorkerThreadPool_UE()
		{
#if (KR_PROFILER_ENABLED != 0)
			g_PopcornFXProfileRecordReentryGuard = true; // Unhook until destroy
#endif // (KR_PROFILER_ENABLED != 0)

			Release();
		}

		virtual void	SubmitTask(PopcornFX::Threads::CAbstractTask *task) override
		{
			PK_ASSERT(task != null);
			PopcornFX::CAsynchronousJob	*job = static_cast<PopcornFX::CAsynchronousJob*>(task);
			PK_ASSERT(job != null && job->Ready());

			TGraphTask<FPopcornFXTask>::CreateTask().ConstructAndDispatchWhenReady(this, job);
		}

		void	Release()
		{
			for (u32 i = 0; i < m_ThreadContexts.Count(); ++i)
			{
				if (m_ThreadContexts[i] != null)
				{
					const PopcornFX::CThreadID	tid = i;
					PopcornFX::CThreadManager::UnsafeUnregisterUserThread(tid);
					PK_SAFE_DELETE(m_ThreadContexts[i]);
				}
			}
#if (KR_PROFILER_ENABLED != 0)
			for (u32 i = 0; i < m_ThreadProfileContexts.Count(); ++i)
			{
				PK_SAFE_DELETE(m_ThreadProfileContexts[i]);
			}
#endif // (KR_PROFILER_ENABLED != 0)
		}

		PopcornFX::Threads::SThreadContext	*GetCurrentThreadContext()
		{
			PopcornFX::CThreadID	tid = PopcornFX::CCurrentThread::ThreadID();
			if (!tid.Valid())
				tid = PopcornFX::CCurrentThread::RegisterUserThread();
			if (!tid.Valid())
				return null;

			PK_ASSERT(tid < PopcornFX::CThreadManager::MaxThreadCount);
			if (m_ThreadContexts[tid] == null)
				m_ThreadContexts[tid] = PK_NEW(PopcornFX::Threads::SThreadContext(this, tid));
			return m_ThreadContexts[tid];
		}

#if (KR_PROFILER_ENABLED != 0)
		SPopcornFXThreadProfileContext		*GetCurrentThreadProfileContext()
		{
			PopcornFX::CThreadID	tid = PopcornFX::CCurrentThread::ThreadID();
			if (!tid.Valid())
				return null; // Valid, we can be in PopcornFX::CCurrentThread::RegisterUserThread()

			PK_ASSERT(tid < PopcornFX::CThreadManager::MaxThreadCount);
			if (m_ThreadProfileContexts[tid] == null)
				m_ThreadProfileContexts[tid] = PK_NEW(SPopcornFXThreadProfileContext());
			return m_ThreadProfileContexts[tid];
		}
#endif // (KR_PROFILER_ENABLED != 0)

	private:
		// Too much, actual max count is defined by MAX_THREADS in TaskGraph.cpp
		PopcornFX::TStaticArray<PopcornFX::Threads::SThreadContext*, PopcornFX::CThreadManager::MaxThreadCount>	m_ThreadContexts;

#if (KR_PROFILER_ENABLED != 0)
		PopcornFX::TStaticArray<SPopcornFXThreadProfileContext*, PopcornFX::CThreadManager::MaxThreadCount>		m_ThreadProfileContexts;
#endif // (KR_PROFILER_ENABLED != 0)
	};

	void	FPopcornFXTask::DoTask(ENamedThreads::Type currentThread, const FGraphEventRef &myCompletionGraphEvent)
	{
		PopcornFX::Threads::SThreadContext	*tCtx = m_Pool->GetCurrentThreadContext();
		if (PK_VERIFY(tCtx != null))
		{
#if 0
			const PopcornFX::CPU::CScoped_FpuDisableExceptions	_de;
#endif
			const PopcornFX::CPU::CScoped_FpuEnableFastMode		_efm;

			m_Task->Run(*tCtx);
		}
		else
			PK_ASSERT_NOT_REACHED();
	}

	PopcornFX::Threads::PAbstractPool	_CreateThreadPool_UE_Auto()
	{
		bool	useUETaskGraph = false;
		GConfig->GetBool(TEXT("PopcornFX"), TEXT("bUseUETaskGraph"), useUETaskGraph, GEngineIni);

		const bool	forceUETaskGraph = false;
		if (useUETaskGraph || forceUETaskGraph)
		{
			// Directly use UE4's task graph system
			CWorkerThreadPool_UE	*pool = PK_NEW(CWorkerThreadPool_UE);
			check(pool != null);
#if (KR_PROFILER_ENABLED != 0)
			g_IsUETaskGraph = true;
#endif // (KR_PROFILER_ENABLED != 0)
			return pool;
		}

		// Fallback to default PopcornFX thread pool and worker threads
		const u64				ueImportantAffinity = FPlatformAffinity::GetMainGameMask() | FPlatformAffinity::GetRenderingThreadMask() | FPlatformAffinity::GetRHIThreadMask();
		const u64				ueTasksAffinity = FPlatformAffinity::GetPoolThreadMask() | FPlatformAffinity::GetTaskGraphThreadMask();

		const bool				ueHasAffinities = (ueImportantAffinity != ueTasksAffinity);

		const PopcornFX::CGenericAffinityMask		fullAffinityMask = PopcornFX::CPU::Caps().ProcessAffinity(); // masks all available logical cores
		const u32									coreCount = fullAffinityMask.NumBitsSet();

		PopcornFX::CGenericAffinityMask				allWorkersAffinityMask;
		u32											workerCount;
		if (ueHasAffinities)
		{
			allWorkersAffinityMask.SetAffinityBlock64(0, ueTasksAffinity);
			allWorkersAffinityMask &= fullAffinityMask;

			workerCount = allWorkersAffinityMask.NumBitsSet();
			PK_RELEASE_ASSERT_MESSAGE(workerCount > 1, "Worker Thread affinity might not be right");
		}
		else
		{
			allWorkersAffinityMask = fullAffinityMask;
			// let 2 core unused for main thread, and render thread
			const u32			dontUseCoreCount = 2;
			const u32			minWorkerCount = 1;
			workerCount = PopcornFX::PKMax(dontUseCoreCount + minWorkerCount, coreCount) - dontUseCoreCount;
		}

		PopcornFX::CWorkerThreadPool	*pool = PK_NEW(PopcornFX::CWorkerThreadPool);
		check(pool != null);

		const PopcornFX::CThreadManager::EPriority		workerThreadPrio = PopcornFX::CThreadManager::Priority_High;

		bool			success = true;
		for (u32 i = 0; i < workerCount; ++i)
		{
			success &= PK_VERIFY(pool->AddWorker(workerThreadPrio, &allWorkersAffinityMask) != null);
		}

		if (!PK_VERIFY(success))
		{
			UE_LOG(LogPopcornFXStartup, Fatal, TEXT("Failed to Startup PopcornFX Thread Pool"));
			PK_SAFE_DELETE(pool);
			return null;
		}
		pool->StartWorkers();

		UE_LOG(LogPopcornFXStartup, Log, TEXT("PopcornFX Worker Thread Pool created: %d threads, affinity %llx"), workerCount, (unsigned long long)(allWorkersAffinityMask.AffinityBlock64(0)));
		return pool;
	}

	void		PopcornFX_AllocFailed(bool *outRetryAlloc, ureg size, u32 alignment, PopcornFX::Mem::EAllocType type)
	{
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("PopcornFX went out of memory for %d bytes !\n"), size);
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("PopcornFX memory stats in bytes: \n"));
		FPlatformMisc::LowLevelOutputDebugStringf(TEXT("    footprint:       %d (overhead: %d)\n"), PopcornFX::CMemStats::RealFootprint(), PopcornFX::CMemStats::Overhead());

		FGenericPlatformMemory::OnOutOfMemory(size, alignment);
	}

	// UE's Malloc will correctly setup system heap size and all (PS4)
	void		*PopcornFX_Alloc(size_t size, PopcornFX::Mem::EAllocType type)
	{
		(void)type;
		LLM_SCOPE(ELLMTag::Particles);
		return FMemory::Malloc(size, DEFAULT_ALIGNMENT); // popcornfx takes care of alignment already
	}

	void		*PopcornFX_Realloc(void *ptr, size_t size, PopcornFX::Mem::EAllocType type)
	{
		(void)type;
		LLM_SCOPE(ELLMTag::Particles);
		return FMemory::Realloc(ptr, size, DEFAULT_ALIGNMENT);
	}

	void		PopcornFX_Free(void *ptr, PopcornFX::Mem::EAllocType type)
	{
		(void)type;
		return FMemory::Free(ptr);
	}

#if (KR_PROFILER_ENABLED != 0)
	bool		PopcornFX_ProfileRecordEventStart(void *arg, const PopcornFX::Profiler::SNodeDescriptor *nodeDescriptor)
	{
		check(g_IsUETaskGraph);

		if (nodeDescriptor == null ||
			nodeDescriptor->m_Name == null)
			return false; // wat

		POPCORNFX_PROFILERECORD_REENTRY_GUARD(return false);

		CWorkerThreadPool_UE			*threadPool = static_cast<CWorkerThreadPool_UE*>(PopcornFX::Scheduler::ThreadPool().Get());
		PK_ASSERT(threadPool != null);
		SPopcornFXThreadProfileContext	*ctx = threadPool->GetCurrentThreadProfileContext();
		bool	success = ctx != null; // Can record
		if (success)
		{
			PK_ASSERT(nodeDescriptor != null);
			const SUEStatIdKey	key = SUEStatIdKey(nodeDescriptor->m_Name);
			PopcornFX::CGuid	statId = ctx->m_StatsMap.IndexOf(key);
			if (!statId.Valid())
			{
				const FString	statName = ANSI_TO_TCHAR(nodeDescriptor->m_Name);
				const TStatId	newStatId = FDynamicStats::CreateStatId<STAT_GROUP_TO_FStatGroup(STATGROUP_PopcornFX_Profile)>(statName);

				statId = ctx->m_StatsMap.Insert(SUEStatIdEntry(SUEStatIdKey(nodeDescriptor->m_Name), newStatId));
			}

			success = statId.Valid();
			if (success)
			{
				const TStatId	&dynamicStatId = ctx->m_StatsMap.At(statId).m_StatId;

#if PK_HAS_CPUPROFILERTRACE
#	if (ENGINE_MINOR_VERSION >= 26)
				if (GCycleStatsShouldEmitNamedEvents > 0 && UE_TRACE_CHANNELEXPR_IS_ENABLED(CpuChannel))
					FCpuProfilerTrace::OutputBeginDynamicEvent(dynamicStatId.GetStatDescriptionANSI());
#	elif (ENGINE_MINOR_VERSION < 25)
				if (GCycleStatsShouldEmitNamedEvents > 0)
					FCpuProfilerTrace::OutputBeginEvent(dynamicStatId.GetTraceCpuProfilerSpecId());
#	endif // (ENGINE_MINOR_VERSION >= 26)
#endif // PK_HAS_CPUPROFILERTRACE

#if (ENGINE_MINOR_VERSION < 25)
				if (FThreadStats::IsCollectingData())
#endif // (ENGINE_MINOR_VERSION < 25)
				{
					if (GCycleStatsShouldEmitNamedEvents > 0)
					{
#if	PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
						FPlatformMisc::BeginNamedEvent(FColor(0), dynamicStatId.GetStatDescriptionANSI());
#else
						FPlatformMisc::BeginNamedEvent(FColor(0), dynamicStatId.GetStatDescriptionWIDE());
#endif // PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
					}
				}

#if (ENGINE_MINOR_VERSION >= 25)
				const FMinimalName	statMinimalName = dynamicStatId.GetMinimalName(EMemoryOrder::Relaxed);
				const FName			statName = MinimalNameToName(statMinimalName);
				FThreadStats::AddMessage(statName, EStatOperation::CycleScopeStart);
#endif // (ENGINE_MINOR_VERSION >= 25)
			}
		}

		return success;
	}

	void		PopcornFX_ProfileRecordEventEnd(void *arg, const PopcornFX::Profiler::SNodeDescriptor *nodeDescriptor)
	{
		check(g_IsUETaskGraph);

		if (nodeDescriptor == null ||
			nodeDescriptor->m_Name == null)
			return; // wat

		POPCORNFX_PROFILERECORD_REENTRY_GUARD(return);

		CWorkerThreadPool_UE			*threadPool = static_cast<CWorkerThreadPool_UE*>(PopcornFX::Scheduler::ThreadPool().Get());
		PK_ASSERT(threadPool != null);
		SPopcornFXThreadProfileContext	*ctx = threadPool->GetCurrentThreadProfileContext();
		if (ctx != null) // Can record
		{
			PK_ASSERT(nodeDescriptor != null);
			const SUEStatIdKey		key = SUEStatIdKey(nodeDescriptor->m_Name);
			const PopcornFX::CGuid	statId = ctx->m_StatsMap.IndexOf(key);
			if (statId.Valid())
			{
				if (GCycleStatsShouldEmitNamedEvents > 0)
				{
#	if PK_HAS_CPUPROFILERTRACE
#		if (ENGINE_MINOR_VERSION >= 26)
					if (UE_TRACE_CHANNELEXPR_IS_ENABLED(CpuChannel))
#		endif //  (ENGINE_MINOR_VERSION >= 26)
					FCpuProfilerTrace::OutputEndEvent();
#	endif // (ENGINE_MINOR_VERSION < 25)

#if (ENGINE_MINOR_VERSION < 25)
					if (FThreadStats::IsCollectingData())
#endif // (ENGINE_MINOR_VERSION < 25)
					{
						FPlatformMisc::EndNamedEvent();
					}
				}
#if (ENGINE_MINOR_VERSION >= 25)
				const TStatId&		dynamicStatId = ctx->m_StatsMap.At(statId).m_StatId;
				const FMinimalName	statMinimalName = dynamicStatId.GetMinimalName(EMemoryOrder::Relaxed);
				const FName			statName = MinimalNameToName(statMinimalName);
				FThreadStats::AddMessage(statName, EStatOperation::CycleScopeEnd);
#endif // (ENGINE_MINOR_VERSION >= 25)
			}
		}
	}

	void		PopcornFX_ProfileRecordMemoryTransaction(void *arg, sreg bytes)
	{
		// No
	}

	void		PopcornFX_ProfileRecordThreadDependency(void *arg, PopcornFX::CThreadID other, u32 dFlags)
	{
		// No
	}
#endif // (KR_PROFILER_ENABLED != 0)

} // namespace

bool	PopcornFXStartup()
{
	using namespace PopcornFX;

	SDllVersion	engineVersion;
	bool		success = false;

	if (engineVersion.Major != PK_VERSION_MAJOR || engineVersion.Minor != PK_VERSION_MINOR)
	{
		UE_LOG(LogPopcornFXStartup, Error, TEXT("PopcornFX Runtime version missmatch: PopcornFX Runtime is v%d.%d, but Plugin has been build with v%d.%d headers !"), engineVersion.Major, engineVersion.Minor, PK_VERSION_MAJOR, PK_VERSION_MINOR);
		return false;
	}

	CPKKernel::Config	kernelConfiguration;

	// Setup FileSystem
#if	(PK_ASSERTS_IN_DEBUG != 0) || (PK_ASSERTS_IN_RELEASE != 0)
	kernelConfiguration.m_AssertCatcher = &AssertImpl;
#endif
	kernelConfiguration.m_AddDefaultLogListeners = &AddDefaultGlobalListenersOverride;
	kernelConfiguration.m_NewFileSystem = PopcornFX_CreateNewFileSystem;

	kernelConfiguration.m_CreateThreadPool = &_CreateThreadPool_UE_Auto;

	kernelConfiguration.m_DefaultAllocator_Alloc = &PopcornFX_Alloc;
	kernelConfiguration.m_DefaultAllocator_Realloc = &PopcornFX_Realloc;
	kernelConfiguration.m_DefaultAllocator_Free = &PopcornFX_Free;
	kernelConfiguration.m_DefaultAllocator_OutOfMemory = &PopcornFX_AllocFailed;

#if (KR_PROFILER_ENABLED != 0)
	bool	useUETaskGraph = false;
	bool	recordProfileMarkers = false;
	GConfig->GetBool(TEXT("PopcornFX"), TEXT("bUseUETaskGraph"), useUETaskGraph, GEngineIni);
	GConfig->GetBool(TEXT("PopcornFX"), TEXT("bRecordProfileMarkers"), recordProfileMarkers, GEngineIni);

	if (!useUETaskGraph)
		recordProfileMarkers = false;
	if (recordProfileMarkers)
	{
		kernelConfiguration.m_ProfilerRecordArg = null;
		kernelConfiguration.m_ProfilerRecordEventStart = &PopcornFX_ProfileRecordEventStart;
		kernelConfiguration.m_ProfilerRecordEventEnd = &PopcornFX_ProfileRecordEventEnd;
		kernelConfiguration.m_ProfilerRecordMemoryTransaction = &PopcornFX_ProfileRecordMemoryTransaction;
		kernelConfiguration.m_ProfilerRecordThreadDependency = &PopcornFX_ProfileRecordThreadDependency;
	}
#endif // (KR_PROFILER_ENABLED != 0)

	success = true;
	success &= CPKKernel::Startup(engineVersion, kernelConfiguration);
	success &= CPKBaseObject::Startup(engineVersion, CPKBaseObject::Config());
	success &= CPKEngineUtils::Startup(engineVersion, CPKEngineUtils::Config());
	success &= CPKCompiler::Startup(engineVersion, CPKCompiler::Config());
	success &= CPKImaging::Startup(engineVersion, CPKImaging::Config());
	success &= CPKGeometrics::Startup(engineVersion, CPKGeometrics::Config());
	success &= CPKParticles::Startup(engineVersion, CPKParticles::Config());
	success &= ParticleToolbox::Startup();
	success &= CPKRenderHelpers::Startup(engineVersion, CPKRenderHelpers::Config());
	success &= PK_VERIFY(Kernel::CheckStaticConfigFlags(Kernel::g_BaseStaticConfig, SKernelConfigFlags()));
	if (!success)
	{
		UE_LOG(LogPopcornFXStartup, Error, TEXT("PopcornFX runtime failed to startup"));
		PopcornFXShutdown();
		return false;
	}

#if (KR_PROFILER_ENABLED != 0)
	if (recordProfileMarkers)
	{
		PopcornFX::Profiler::MainEngineProfiler()->Activate(true); // !NOT! compatible with .pkpr export
	}
#endif // (KR_PROFILER_ENABLED != 0)

	if (!StartupPlugins())
	{
		UE_LOG(LogPopcornFXStartup, Error, TEXT("PopcornFX runtime plugins failed to startup"));
		PopcornFXShutdown();
		return false;
	}

	PK_LOG_MODULE_INIT_START;
	PK_LOG_MODULE_INIT_END;

	CResourceHandlerMesh_UE::Startup();
	CResourceHandlerImage_UE::Startup();
	CResourceHandlerVectorField_UE::Startup();

	VertexAnimationRendererProperties::Startup();
	UE4RendererProperties::Startup();

#if WITH_EDITOR
	PopcornFX::COvenBakeConfig_Base::RegisterHandler();
	PopcornFX::COvenBakeConfig_HBO::RegisterHandler();
	PopcornFX::COvenBakeConfig_Particle::RegisterHandler();
	PopcornFX::COvenBakeConfig_ParticleCompiler::RegisterHandler();
#endif // WITH_EDITOR

	CCoordinateFrame::SetGlobalFrame(ECoordinateFrame::Frame_LeftHand_Z_Up);


	// Register load tags
	success &= PopcornFX::CParticleManager::AddGlobalConstant(PopcornFX::CParticleManager::SElement("build.tags.low", 20));
	success &= PopcornFX::CParticleManager::AddGlobalConstant(PopcornFX::CParticleManager::SElement("build.tags.medium", 21));
	success &= PopcornFX::CParticleManager::AddGlobalConstant(PopcornFX::CParticleManager::SElement("build.tags.high", 22));
	success &= PopcornFX::CParticleManager::AddGlobalConstant(PopcornFX::CParticleManager::SElement("build.tags.epic", 23));
	success &= PopcornFX::CParticleManager::AddGlobalConstant(PopcornFX::CParticleManager::SElement("build.tags.cinematic", 24));

#if UE_BUILD_DEBUG || WITH_EDITOR
	PopcornFX::CLog::SetGlobalLogLevel(CLog::Level_Debug);
#endif

	return success;
}

//----------------------------------------------------------------------------

void	PopcornFXShutdown()
{
	using namespace PopcornFX;

	PK_LOG_MODULE_RELEASE_START;
	PK_LOG_MODULE_RELEASE_END;


#if WITH_EDITOR
	PopcornFX::COvenBakeConfig_ParticleCompiler::UnregisterHandler();
	PopcornFX::COvenBakeConfig_Particle::UnregisterHandler();
	PopcornFX::COvenBakeConfig_HBO::UnregisterHandler();
	PopcornFX::COvenBakeConfig_Base::UnregisterHandler();
#endif // WITH_EDITOR

	ShutdownPlugins();

	UE4RendererProperties::Shutdown();
	VertexAnimationRendererProperties::Shutdown();

	CPKRenderHelpers::Shutdown();
	ParticleToolbox::Shutdown();
	CPKParticles::Shutdown();
	CPKGeometrics::Shutdown();
	CPKImaging::Shutdown();
	CPKCompiler::Shutdown();
	CPKEngineUtils::Shutdown();
	CPKBaseObject::Shutdown();

	CResourceHandlerVectorField_UE::Shutdown();
	CResourceHandlerImage_UE::Shutdown();
	CResourceHandlerMesh_UE::Shutdown();
	CPKKernel::Shutdown();
}

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
