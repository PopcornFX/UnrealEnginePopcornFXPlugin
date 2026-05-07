//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "GameFramework/HUD.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/pk_kernel_config.h>
#include <pk_kernel/include/kr_containers.h>
#include <pk_kernel/include/kr_string.h>

#include "PopcornFXHUDMemory.generated.h"

class CParticleScene;

#define	PKUE_HUD_GPU_BUFFER_POOLS		0 // not working yet
#define	PKUE_HUD_PARTICLE_PAGE_POOL		1

#if (KR_MEM_DEFAULT_ALLOCATOR_DEBUG != 0) && (KR_MEM_DEBUG != 0)
#	define	PKUE_HUD_MEM_NODES				1
#else
#	define	PKUE_HUD_MEM_NODES				0
#endif

#if (PKUE_HUD_MEM_NODES != 0)
FWD_PK_API_BEGIN
struct	CMemStatNode;
FWD_PK_API_END
#endif
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

UCLASS(MinimalAPI, Config=Game)
class APopcornFXHUDMemory : public AHUD
{
	GENERATED_UCLASS_BODY()

public:
	~APopcornFXHUDMemory();

	void				DrawMemoryHUD(UCanvas* InCanvas, APlayerController* PC);
	//virtual void		PostRender() override;

	struct SPoolStat
	{
		float			m_Mem = 0;
		float			m_MemUsed = 0;
		float			m_Alloc = 0;
		float			m_Release = 0;

		void			Clear() { PopcornFX::Mem::Clear(*this); }
		template <typename _Pool>
		void			AccumPool(_Pool &pool);
		void			Accum(const SPoolStat &other);
		void			ComputePerFrame(float invFrameCount);
	};

	struct SMemNode
	{
		FString			m_Name;
		float			m_Mem = 0;
		void			Clear() { m_Name.Empty(); m_Mem = 0.f; }
	};

	struct SFrame
	{
		float			m_TotalParticles;

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
		SPoolStat		m_IB;
		SPoolStat		m_VB;
		SPoolStat		m_GPUIB;
		SPoolStat		m_GPUVB;
#endif

#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
		float			m_PPP_TotalMem;
		float			m_PPP_TotalUsed;
		float			m_PPP_AllocCount;
#endif

		void			Clear()
		{
			m_TotalParticles = 0;

#if (PKUE_HUD_GPU_BUFFER_POOLS != 0)
			m_IB.Clear();
			m_VB.Clear();
			m_GPUIB.Clear();
			m_GPUVB.Clear();
#endif
#if (PKUE_HUD_PARTICLE_PAGE_POOL != 0)
			m_PPP_TotalMem = 0;
			m_PPP_TotalUsed = 0;
			m_PPP_AllocCount = 0;
#endif
		}
	};

	virtual void		PostActorCreated() override;
	virtual void		Destroyed() override;

	void				Update(SFrame &frame);
	void				Draw(SFrame &frame);

#if (PKUE_HUD_MEM_NODES != 0)
	PopcornFX::TArray<PopcornFX::CMemStatNode>		*m_MemNodesCache = null;
#endif

	void				DrawVBar(float minX, float maxX, float y, float value, float maxValue, float thickness);

private:
	bool				m_ProfilerStarted = false;

	u32								m_CurrentFrame = 0;
	//u32							m_Count = 0;

	PopcornFX::TArray<SFrame>		m_Frames;
	u32								m_MaxFrame = 0;

	SFrame							m_MergedFrame;
};
