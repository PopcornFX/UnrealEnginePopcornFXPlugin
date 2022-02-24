//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "BBView.h"
#include "PrimitiveSceneProxy.h"

#if RHI_RAYTRACING
#	include "RayTracingInstance.h"
#endif // RHI_RAYTRACING

// TODO: Will be replaced by view user datas
FWD_PK_API_BEGIN
class	CRendererSubView;
FWD_PK_API_END

// Context passed around calls to the policy
struct	SRenderContext
{
#if (PK_HAS_GPU != 0)
	// Represents the compute API, if _Unsupported it means we have not that API supported as a GPU sim backend
	enum	ERHIAPI
	{
		D3D11,
		D3D12,

		_Unsupported
	};
#endif // (PK_HAS_GPU != 0)

public:
	class CRenderBatchManager			*m_RenderBatchManager;
	class PopcornFX::CRendererSubView	*m_RendererSubView;

#if (PK_HAS_GPU != 0)
	ERHIAPI								m_API;
#endif // (PK_HAS_GPU != 0)

	void		SetWorld(UWorld *world) { PK_ASSERT(IsInGameThread()); m_World = world; }
	UWorld		*GetWorld() const { PK_ASSERT(IsInGameThread()); return m_World; }
private:
	UWorld		*m_World;
};

struct	SCollectedDrawCalls
{
	PopcornFX::TArray<FSimpleLightPerViewEntry>	m_LightPositions;
	PopcornFX::TArray<FSimpleLightEntry>		m_LightDatas;

	void	Clear()
	{
		m_LightPositions.Clear();
		m_LightDatas.Clear();
	}
};

class	CPopcornFXRenderTypes
{
public:
	typedef SRenderContext			CRenderContext;
	typedef SCollectedDrawCalls		CFrameOutputData;
	typedef CBBView					CViewUserData;

	enum { kMaxQueuedCollectedFrame = 4U };
};
