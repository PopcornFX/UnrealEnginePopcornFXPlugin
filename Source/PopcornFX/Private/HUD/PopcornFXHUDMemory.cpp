//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXHUDMemory.h"

#include "PopcornFXPlugin.h"
#include "Internal/ParticleScene.h"
#include "Internal/PopcornFXProfiler.h"
#include "PopcornFXSceneActor.h"
#include "PopcornFXSceneComponent.h"

#include "EngineUtils.h"
#include "Engine/Engine.h"
#include "Engine/Canvas.h"
#include "Debug/DebugDrawService.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_mem_stats.h>
#include <pk_kernel/include/kr_sort.h>
#include <pk_particles/include/ps_mediums.h>
#include <pk_particles/include/ps_stats.h>
#include <pk_particles/include/ps_storage.h>
#include <pk_particles/include/Storage/MainMemory/storage_ram_allocator.h>
#include <pk_particles/include/Updaters/Auto/updater_auto.h>
#include <pk_particles/include/Storage/MainMemory/storage_ram.h>
#if (PK_GPU_D3D11 == 1)
#	include <pk_particles/include/Updaters/D3D11/updater_d3d11.h>
#endif

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXMemoryHUD"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXMemoryHUD, Log, All);

//----------------------------------------------------------------------------

namespace
{
#if (PKUE_HUD_MEM_NODES != 0)
	class	CMemStatNodeSorter
	{
	public:
		typedef	PopcornFX::TArray<PopcornFX::CMemStatNode>::Iterator	_TypeIt;
		PK_FORCEINLINE static bool	Less(const _TypeIt &it0, const _TypeIt &it1) { return it0->Footprint() > it1->Footprint(); }
		PK_FORCEINLINE static bool	LessOrEqual(const _TypeIt &it0, const _TypeIt &it1) { return it0->Footprint() >= it1->Footprint(); }
		PK_FORCEINLINE static bool	Equal(const _TypeIt &it0, const _TypeIt &it1) { return it0->Footprint() == it1->Footprint(); }
	};
#endif
}

//#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
//static TAutoConsoleVariable<int32> CVarDebugCameraTraceComplex(
//	TEXT("g.DebugCameraTraceComplex"),
//	1,
//	TEXT("Whether DebugCamera should use complex or simple collision for the line trace.\n")
//	TEXT("1: complex collision, 0: simple collision "),
//	ECVF_Cheat);
//#endif

template <typename _Pool>
void	APopcornFXHUDMemory::SPoolStat::AccumPool(_Pool &pool)
{
	PopcornFX::SBufferPool_Stats		stats;
	pool.AccumStats(&stats);
	m_Mem += stats.m_TotalBytes;
	m_MemUsed += stats.m_SoftExactUsedBytes;
	m_Alloc += stats.m_AllocatedBytes;
	m_Release += stats.m_ReleasedBytes;
}

void	APopcornFXHUDMemory::SPoolStat::Accum(const SPoolStat &other)
{
	m_Mem += other.m_Mem;
	m_MemUsed += other.m_MemUsed;
	m_Alloc += other.m_Alloc;
	m_Release += other.m_Release;
}

void	APopcornFXHUDMemory::SPoolStat::ComputePerFrame(float invFrameCount)
{
	m_Mem *= invFrameCount;
	m_MemUsed *= invFrameCount;
	//m_Alloc += other.m_Alloc;
	//m_Release += other.m_Release;
}

APopcornFXHUDMemory::APopcornFXHUDMemory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetHidden(false);
	bShowHUD = true;

	m_MaxFrame = 0;

#if (PKUE_HUD_MEM_NODES != 0)
	m_MemNodesCache = PK_NEW(PopcornFX::TArray<PopcornFX::CMemStatNode>);
#endif
}

APopcornFXHUDMemory::~APopcornFXHUDMemory()
{
#if (PKUE_HUD_MEM_NODES != 0)
	PK_SAFE_DELETE(m_MemNodesCache);
#endif
}

void	APopcornFXHUDMemory::PostActorCreated()
{
	Super::PostActorCreated();
	m_MaxFrame = 0;
	m_Frames.Clear();
}

void	APopcornFXHUDMemory::Destroyed()
{
	Super::Destroyed();
}

void	APopcornFXHUDMemory::DrawVBar(float minX, float maxX, float y, float value, float maxValue, float thickness)
{
	float			sizeX = (maxX - minX) * value * SafeRcp(maxValue);
	float			c = PopcornFX::PKMax(0.f, PopcornFX::PKMin(value * SafeRcp(maxValue), 1.f));
	FLinearColor	color(c, 1.f - c, 0.f, 0.6f);
	{
		FCanvasTileItem	tile(
			FVector2D(minX, y),
			FVector2D(sizeX, thickness),
			color
		);
		tile.BlendMode = SE_BLEND_AlphaBlend;
		Canvas->DrawItem(tile);
	}
	{
		color.A = 0.1f;
		FCanvasTileItem	tile(
			FVector2D(minX + sizeX, y),
			FVector2D((maxX - minX) - sizeX, thickness),
			color
		);
		tile.BlendMode = SE_BLEND_AlphaBlend;
		Canvas->DrawItem(tile);
	}
}

void		APopcornFXHUDMemory::Update(SFrame &frame)
{
	PK_NAMEDSCOPEDPROFILE_C("APopcornFXHUDMemory::Update", POPCORNFX_UE_PROFILER_COLOR);

	frame.Clear();

#ifndef PK_RETAIL
	UWorld				*world = GetWorld();
	if (world == null)
		return;

	TActorIterator<APopcornFXSceneActor>	sceneIt(world);
	for (; sceneIt; ++sceneIt)
	{
		if (sceneIt->PopcornFXSceneComponent == null ||
			sceneIt->PopcornFXSceneComponent->ParticleScene() == null)
			continue;

		if (!sceneIt->PopcornFXSceneComponent->IsActive() ||
			!sceneIt->PopcornFXSceneComponent->IsComponentTickEnabled() ||
			!sceneIt->PopcornFXSceneComponent->bEnableUpdate)
			continue;

		CParticleScene			*scene = sceneIt->PopcornFXSceneComponent->ParticleScene();

		PK_SCOPEDLOCK(scene->m_UpdateLock);
		//scene->Debug_GoreLock_Lock();

		frame.m_TotalParticles += scene->LastUpdatedParticleCount();

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
		PK_FIXME("Async access to RenderThread's VB: baaad");
		//frame.m_VB.AccumPool(scene->VBPool());
		//frame.m_IB.AccumPool(scene->IBPool());
		//frame.m_GPUVB.AccumPool(scene->GPUVBPool());
		//frame.m_GPUIB.AccumPool(scene->GPUIBPool());
#endif
		//scene->Debug_GoreLock_Unlock();
	}

#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
	PopcornFX::CParticlePageAllocator_Stats		pageStats;
	PopcornFX::CParticleStorageManager_MainMemory::GlobalPageAllocator()->GetTotalStats(pageStats);
	PK_ASSERT(pageStats.m_Used >= pageStats.m_Unused);
	PK_ASSERT(pageStats.m_Allocated >= pageStats.m_Freed);
	PK_ASSERT(pageStats.m_AllocCount >= pageStats.m_FreeCount);
	frame.m_PPP_TotalUsed = pageStats.m_Used - pageStats.m_Unused;
	frame.m_PPP_TotalMem = pageStats.m_Allocated - pageStats.m_Freed;
	frame.m_PPP_AllocCount = pageStats.m_AllocCount - pageStats.m_FreeCount;
#endif

#if (PKUE_HUD_MEM_NODES != 0)
	m_MemNodesCache->Clear();
	PopcornFX::CMemStats::CaptureStatNodes(*m_MemNodesCache);
	PopcornFX::QuickSort<CMemStatNodeSorter>(m_MemNodesCache->Begin(), m_MemNodesCache->End());
#endif
#endif // PK_RETAIL
}

void		APopcornFXHUDMemory::DrawMemoryHUD(UCanvas* InCanvas, APlayerController* PC)
{
	PK_NAMEDSCOPEDPROFILE_C("APopcornFXHUDMemory", POPCORNFX_UE_PROFILER_COLOR);

	//Super::PostRender();

	//if (bShowHUD == 0)
	//	return;

	Canvas = InCanvas;
	if (!Canvas)
		return;

	//if (!PC)
	//	return;

	const u32		frameCount = 16;
	if (m_Frames.Count() != frameCount)
	{
		u32		oldCount = m_Frames.Count();
		if (!m_Frames.Resize(frameCount))
			return;
		for (u32 i = oldCount; i < m_Frames.Count(); ++i)
		{
			m_Frames[i].Clear();
		}
	}

	m_CurrentFrame = (m_CurrentFrame + 1) % m_Frames.Count();

	Update(m_Frames[m_CurrentFrame]);

	m_MaxFrame = PopcornFX::PKMax(m_MaxFrame, m_CurrentFrame + 1);

	m_MergedFrame.Clear();

	for (u32 framei = 0; framei < m_MaxFrame; ++framei)
	{
		const SFrame	&srcFrame = m_Frames[framei];

		m_MergedFrame.m_TotalParticles += srcFrame.m_TotalParticles;

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
		m_MergedFrame.m_VB.Accum(srcFrame.m_VB);
		m_MergedFrame.m_IB.Accum(srcFrame.m_IB);
		m_MergedFrame.m_GPUVB.Accum(srcFrame.m_GPUVB);
		m_MergedFrame.m_GPUIB.Accum(srcFrame.m_GPUIB);
#endif

#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
		m_MergedFrame.m_PPP_TotalMem += srcFrame.m_PPP_TotalMem;
		m_MergedFrame.m_PPP_TotalUsed += srcFrame.m_PPP_TotalUsed;
		m_MergedFrame.m_PPP_AllocCount += srcFrame.m_PPP_AllocCount;
#endif
	}

	const float			invMaxFrame = 1.0 / m_MaxFrame;

	m_MergedFrame.m_TotalParticles *= invMaxFrame;

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
	m_MergedFrame.m_VB.ComputePerFrame(invMaxFrame);
	m_MergedFrame.m_IB.ComputePerFrame(invMaxFrame);
	m_MergedFrame.m_GPUVB.ComputePerFrame(invMaxFrame);
	m_MergedFrame.m_GPUIB.ComputePerFrame(invMaxFrame);
#endif

#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
	m_MergedFrame.m_PPP_TotalMem *= invMaxFrame;
	m_MergedFrame.m_PPP_TotalUsed *= invMaxFrame;
	m_MergedFrame.m_PPP_AllocCount *= invMaxFrame;
#endif

	Draw(m_MergedFrame);
}

void		APopcornFXHUDMemory::Draw(SFrame &frame)
{
	PK_NAMEDSCOPEDPROFILE_C("APopcornFXHUDMemory::Draw", POPCORNFX_UE_PROFILER_COLOR);

	UFont				*font = GEngine->GetSmallFont();
	FFontRenderInfo		fri = Canvas->CreateFontRenderInfo(true, true);
	FFontRenderInfo		friNoShadow = Canvas->CreateFontRenderInfo(true, false);

	//Canvas->SetDrawColor(64, 64, 255, 255);
	Canvas->SetDrawColor(255, 255, 255, 255);

	FString		title = TEXT("PopcornFX Memory Stats");
	float _xl, _yl;
	Canvas->StrLen(font, title, _xl, _yl);
	const float xl = _xl;
	const float yl = _yl;

	const UPopcornFXSettings	*settings = FPopcornFXPlugin::Get().Settings();
	check(settings);
	const float		screenRatio = settings->HUD_DisplayScreenRatio;
	const float		hideBelowPerc = settings->HUD_HideNodesBelowPercent;

	//const float		canvasW = Canvas->SizeX;
	const float		canvasH = Canvas->SizeY;

	float			y = canvasH * screenRatio;
	const float		maxY = canvasH - y;
	float			x = y;

	Canvas->DrawText(font, title, x, y, 1.f, 1.f, fri);
	y += yl;

	Canvas->DrawText(font,
		FString::Printf(TEXT("(Averaged over the last %d frames)"), m_MaxFrame),
		x, y, 1.f, 1.f, fri);
	y += yl;

	//y += yl;

	const float			totalMem = PopcornFX::CMemStats::RealFootprint();
	Canvas->DrawText(font,
		FString::Printf(TEXT("PopcornFX total memory used: %5.1f%s"), HumanReadF(totalMem), HumanReadS(totalMem)),
		x, y, 1.f, 1.f, fri);
	y += yl;

	{
		const double		totalPCount = frame.m_TotalParticles;
		//Canvas->SetDrawColor(200, 200, 128, 255);
		Canvas->DrawText(font,
			FString::Printf(TEXT("Particle count: %6.2f%s"), HumanReadF(totalPCount, 1000), HumanReadS(totalPCount, 1000)),
			x, y, 1.f, 1.f, fri);
		y += yl;
	}

#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
	Canvas->DrawText(font,
		FString::Printf(
			TEXT("Particle page memory used/total: %5.1f%s/%5.1f%s (%5.1f%%) (%4.0f allocs used)"),
			HumanReadF(frame.m_PPP_TotalUsed), HumanReadS(frame.m_PPP_TotalUsed),
			//HumanReadF(0), HumanReadS(0),
			HumanReadF(frame.m_PPP_TotalMem), HumanReadS(frame.m_PPP_TotalMem),
			100.f * frame.m_PPP_TotalUsed * SafeRcp(frame.m_PPP_TotalMem),
			frame.m_PPP_AllocCount
			),
		x, y, 1.f, 1.f, fri);
	y += yl;
#endif

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
	float		poolX1 = 130;
	float		poolX2 = poolX1 + 100;

#define DRAW_POOL(__name, __pool)													\
		Canvas->DrawText(font, __name,												\
			x, y, 1.f, 1.f, fri);													\
		Canvas->DrawText(font,														\
			FString::Printf(TEXT("%5.1f%s %5.1f%%"),								\
				HumanReadF((__pool).m_Mem), HumanReadS((__pool).m_Mem),				\
				(__pool).m_MemUsed * 100.f * SafeRcp((__pool).m_Mem)				\
			),																		\
			x + poolX1, y, 1.f, 1.f, fri);											\
		Canvas->DrawText(font,														\
			FString::Printf(TEXT(", %5.1f%s alloc, %5.1f%s release"),				\
				HumanReadF((__pool).m_Alloc), HumanReadS((__pool).m_Alloc),			\
				HumanReadF((__pool).m_Release), HumanReadS((__pool).m_Release)		\
			),																		\
			x + poolX2, y, 1.f, 1.f, fri);

	DRAW_POOL(TEXT("Render CPU Pool VB:"), frame.m_VB);
	y += yl;
	DRAW_POOL(TEXT("Render CPU Pool IB:"), frame.m_IB);
	y += yl;
	DRAW_POOL(TEXT("Render GPU Pool VB:"), frame.m_GPUVB);
	y += yl;
	DRAW_POOL(TEXT("Render GPU Pool IB:"), frame.m_GPUIB);
	y += yl;

	y += yl;
#endif

#if (PKUE_HUD_MEM_NODES != 0)
	const float		maxSlotListY = maxY - 2.f * yl;
	if (y <= maxSlotListY)
	{

		Canvas->DrawText(font,
			TEXT("Detailed PopcornFX memory nodes current memory usage (instant, not averaged):"),
			x, y, 1.f, 1.f, fri);
		y += yl;

		const float		offperc = 0.f;
		const float		offmem = offperc + 60.f;
		const float		offname = offmem + 60.f;

		Canvas->DrawText(font,
			TEXT("%Mem"),
			x + offperc, y, 1.f, 1.f, fri);
		Canvas->DrawText(font,
			TEXT("Mem"),
			x + offmem, y, 1.f, 1.f, fri);
		Canvas->DrawText(font,
			TEXT("Memory node name"),
			x + offname, y, 1.f, 1.f, fri);
		y += yl;

		float			dispMem = 0.f;

		const PopcornFX::TArray<PopcornFX::CMemStatNode>		&memNodes = *m_MemNodesCache;
		const u32			memNodeCount = memNodes.Count();
		for (u32 i = 0; i < memNodeCount; ++i)
		{
			if (y > maxSlotListY)
				break;

			const PopcornFX::CMemStatNode		&node = memNodes[i];
			const FString	&name = node.m_Name;
			const float		mem = node.Footprint();
			const float		perc = 100.f * mem / totalMem;

			if (perc < hideBelowPerc)
				break;

			dispMem += mem;

			DrawVBar(x + offperc, x + offmem - 1, y, perc, 100.f, yl);
			{
				Canvas->DrawText(font,
					FString::Printf(TEXT("%4.1f"), perc),
					x + offperc, y, 1.f, 1.f, friNoShadow);
				Canvas->DrawText(font,
					FString::Printf(TEXT("%5.1f%s"), HumanReadF(mem), HumanReadS(mem)),
					x + offmem, y, 1.f, 1.f, fri);
				Canvas->DrawText(font,
					name,
					x + offname, y, 1.f, 1.f, fri);
			}
			y += yl;
		}

		if (y <= maxY)
		{
			const FString	name = TEXT("Memory displayed here");
			const float		mem = dispMem;
			const float		perc = 100.f * mem / totalMem;

			DrawVBar(x + offperc, x + offmem - 1, y, perc, 100.f, yl);
			{
				Canvas->DrawText(font,
					FString::Printf(TEXT("%4.1f"), perc),
					x + offperc, y, 1.f, 1.f, friNoShadow);
				Canvas->DrawText(font,
					FString::Printf(TEXT("%5.1f%s"), HumanReadF(mem), HumanReadS(mem)),
					x + offmem, y, 1.f, 1.f, fri);
				Canvas->DrawText(font,
					name,
					x + offname, y, 1.f, 1.f, fri);
			}
			y += yl;
		}

		y += yl;
	}
#else
	if (y <= canvasH)
	{
		Canvas->DrawText(font,
			TEXT("Detailed memory nodes statistics not available (enabled by default only for Debug/DebugGame Builds)"),
			x, y, 1.f, 1.f, fri);
		y += yl;
		y += yl;
	}
#endif
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

static void		PopcornFXToggleMemoryHUD(const TArray<FString>& args, UWorld *InWorld)
{
	if (!InWorld)
	{
		UE_LOG(LogPopcornFXMemoryHUD, Warning, TEXT("ToggleMemoryHUD: no World"));
		return;
	}
	bool		hasForce = false;
	bool		forceOn = false;
	if (args.Num() >= 1)
	{
		hasForce = true;
		if (args[0] == "1")
			forceOn = true;
	}

	static FDelegateHandle	s_drawDebugDelegateHandle;
	APopcornFXHUDMemory		*HUD = NULL;
	for (TActorIterator<APopcornFXHUDMemory> It(InWorld); It; ++It)
	{
		HUD = *It;
		break;
	}
	if (hasForce && forceOn == (HUD != null))
		return;
	if (HUD == null)
	{
		HUD = InWorld->SpawnActor<APopcornFXHUDMemory>();
		FDebugDrawDelegate DrawDebugDelegate = FDebugDrawDelegate::CreateUObject(HUD, &APopcornFXHUDMemory::DrawMemoryHUD);
		s_drawDebugDelegateHandle = UDebugDrawService::Register(TEXT("OnScreenDebug"), DrawDebugDelegate);
		UE_LOG(LogPopcornFXMemoryHUD, Log, TEXT("ToggleMemoryHUD: HUD added to %s"), *InWorld->GetFullName());
	}
	else
	{
		UDebugDrawService::Unregister(s_drawDebugDelegateHandle);
		HUD->Destroy();
		UE_LOG(LogPopcornFXMemoryHUD, Log, TEXT("ToggleMemoryHUD: HUD removed from %s"), *InWorld->GetFullName());
	}
}

FAutoConsoleCommandWithWorldAndArgs		PopcornFXMemoryHUDCommand(
	TEXT("PopcornFX.ToggleMemoryHUD"),
	TEXT("PopcornFX.ToggleMemoryHUD [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(PopcornFXToggleMemoryHUD)
);

// for backward compat
FAutoConsoleCommandWithWorldAndArgs		PopcornFXDebugHUDCommand(
	TEXT("PopcornFX.ToggleDebugHUD"),
	TEXT("PopcornFX.ToggleDebugHUD [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(PopcornFXToggleMemoryHUD)
);

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

//----------------------------------------------------------------------------
#undef LOCTEXT_NAMESPACE
