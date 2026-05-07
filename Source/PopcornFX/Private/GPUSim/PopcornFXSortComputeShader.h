//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#if (PK_HAS_GPU != 0)

#include "PopcornFXMinimal.h"

#include "Render/PopcornFXBuffer.h"

#include "GlobalShader.h"
#include "ShaderParameterUtils.h"

#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

struct FPopcornFXSortComputeShader_GenKeys_Params
{
	CFloat4		m_SortOrigin;

	u32			m_Count = 0;
	u32			m_IndexStart = 0;
	u32			m_IndexStep = 0;
	u32			m_InputOffset = 0;

	FShaderResourceViewRHIRef	m_InputPositions;
	u32							m_InputPositionsOffset = 0;
};

//----------------------------------------------------------------------------
//
// FPopcornFXSortComputeShader_Sorter
//
//----------------------------------------------------------------------------

struct FPopcornFXSortComputeShader_Sorter
{
public:
	void		Clear();

	bool		Prepare(CVertexBufferPool &vpool, u32 totalCount);
	bool		Ready() const { return m_TotalCount > 0; }

	void		DispatchGenIndiceBatch(FRHICommandList& RHICmdList, const FPopcornFXSortComputeShader_GenKeys_Params &genParams);
	void		DispatchSort(FRHICommandList& RHICmdList);

	const FShaderResourceViewRHIRef		&SortedValuesSRV();

private:
	u32		m_TotalCount = 0;

	u32		m_CurrGenOutputOffset = 0;

	u32						m_CurrBuff = 0;
	CPooledVertexBuffer		m_Keys[2];
	CPooledVertexBuffer		m_Values[2];
	CPooledVertexBuffer		m_Cache;
};

//----------------------------------------------------------------------------

#endif // (PK_HAS_GPU != 0)
