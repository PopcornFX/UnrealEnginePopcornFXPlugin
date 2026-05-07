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

#include <pk_render_helpers/include/frame_collector/rh_frame_data.h>

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

struct	SUERenderContext : public PopcornFX::SRenderContext
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
	class CRenderBatchManager			*m_RenderBatchManager = null;
	class PopcornFX::CRendererSubView	*m_RendererSubView = null;
	SCollectedDrawCalls					*m_CollectedDrawCalls = null;

#if (PK_HAS_GPU != 0)
	ERHIAPI								m_API;
#endif // (PK_HAS_GPU != 0)

	void		SetWorld(UWorld *world) { PK_ASSERT(IsInGameThread()); m_World = world; }
	UWorld		*GetWorld() const { PK_ASSERT(IsInGameThread()); return m_World; }

private:
	UWorld		*m_World = null;
};

//----------------------------------------------------------------------------

struct	SStreamOffset
{
	u32					m_OffsetInBytes;
	u32					m_InputId; // Optional input id
	bool				m_ValidOffset;
	bool				m_ValidInputId;

	SStreamOffset()
	{
		Reset();
	}

	void				Reset()
	{
		m_ValidOffset = false;
		m_ValidInputId = false;
		m_OffsetInBytes = 0;
		m_InputId = 0;
	}

	s32				OffsetForShaderConstant() const { return m_ValidOffset ? m_OffsetInBytes / sizeof(float) : -1; }
	u32				InputId() const { PK_ASSERT(m_ValidOffset && m_ValidInputId); return m_InputId; }
	bool			Valid() const { return m_ValidOffset; }
	operator u32() const { PK_VERIFY(m_ValidOffset); return m_OffsetInBytes; }

	SStreamOffset	& operator = (u32 offsetInBytes)
	{
		m_OffsetInBytes = offsetInBytes;
		m_ValidOffset = true;
		m_ValidInputId = false;
		m_InputId = 0;
		return *this;
	}

	void			Setup(u32 offsetInBytes, u32 inputId)
	{
		m_OffsetInBytes = offsetInBytes;
		m_InputId = inputId;
		m_ValidOffset = true;
		m_ValidInputId = true;
	}
};
