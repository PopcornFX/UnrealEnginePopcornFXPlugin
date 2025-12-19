//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------
#pragma once

#include "Render/MaterialDesc.h"

//----------------------------------------------------------------------------

// Maximum number of draw requests batched for GPU particles in a single billboarding batch policy (1 draw request = 1 particle renderer being drawn)
#define PK_GPU_MAX_DRAW_REQUEST_COUNT	0x100

//----------------------------------------------------------------------------
//
//	Atlas buffer (holds CRectangleList rects)
//
//----------------------------------------------------------------------------

struct	FPopcornFXAtlasRectsVertexBuffer
{
	//FStructuredBufferRHIRef			m_AtlasBuffer_Structured;
	FBufferRHIRef					m_AtlasBuffer_Raw;
	FShaderResourceViewRHIRef		m_AtlasBufferSRV;
	u32								m_AtlasRectsCount = 0;
	u32								m_AtlasBufferCapacity = 0;

	bool		Loaded() const { return m_AtlasRectsCount > 0; }
	void		Clear()
	{
		//m_AtlasBuffer_Structured = null;
		m_AtlasBuffer_Raw = null;
		m_AtlasBufferSRV = null;
		m_AtlasRectsCount = 0;
		m_AtlasBufferCapacity = 0;
	}
	bool		LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects);

private:
	bool		_LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects);
};

//----------------------------------------------------------------------------
//
//	DrawIndexedInstancedIndirect (ie. https://docs.microsoft.com/en-us/windows/win32/api/d3d11/ns-d3d11-d3d11_draw_indexed_instanced_indirect_args)
//
//----------------------------------------------------------------------------

struct	FDrawIndexedIndirectArgs
{
	u32	m_IndexCount;				// The number of indices read from the index buffer for each instance.
	u32	m_InstanceCount;			// The number of instances to draw.
	u32	m_IndexOffset;				// The location of the first index read by the GPU from the index buffer.
	s32	m_VertexOffset;				// A value added to each index before reading a vertex from the vertex buffer.
	u32	m_InstanceOffset;			// A value added to each index before reading per-instance data from a vertex buffer.
	static const u32 kCount = 5;	// The number of arguments in the struct.
};

//----------------------------------------------------------------------------
//
//	Null Float4 buffer
//
//----------------------------------------------------------------------------

class	FNullFloat4Buffer : public FVertexBuffer
{
public:
	virtual void	InitRHI(FRHICommandListBase &RHICmdList) override;
	virtual void	ReleaseRHI() override;

	FShaderResourceViewRHIRef	SRV;
};

extern TGlobalResource<FNullFloat4Buffer>	GPopcornFXNullFloat4Buffer;

//----------------------------------------------------------------------------
//
// ExecuteOn Render/RHI Thread (taken from XRBase plugin)
//
//----------------------------------------------------------------------------

/**
* Utility function for easily submitting TFunction to be run on the render thread. Must be invoked from the game thread.
* If rendering does not use a separate thread, the TFunction will be executed immediately, otherwise it will be added to the render thread task queue.
* @param Function - the Function to be invoked on the render thread.
* @return true if the function was queued, false if rendering does not use a separate thread, in which case, the function has already been executed.
*/
void PKExecuteOnRenderThread_DoNotWait(const TFunction<void()>& Function);

/**
* Utility function for easily submitting TFunction to be run on the render thread. Must be invoked from the game thread.
* If rendering does not use a separate thread, the TFunction will be executed immediately, otherwise it will be added to the render thread task queue.
* @param Function - the Function to be invoked on the render thread. When executed, it will get the current FRHICommandList instance passed in as its sole argument.
* @return true if the function was queued, false if rendering does not use a separate thread, in which case, the function has already been executed.
*/
void PKExecuteOnRenderThread_DoNotWait(const TFunction<void(FRHICommandListImmediate&)>& Function);

/**
* Utility function for easily running a TFunctionRef on the render thread. Must be invoked from the game thread.
* If rendering does not use a separate thread, the TFunction will be executed immediately, otherwise it will be added to the render thread task queue.
* This method will flush rendering commands meaning that the function will be executed before PKExecuteOnRenderThread returns.
* @param Function - the Function to be invoked on the render thread.
* @return true if the function was queued, false if rendering does not use a separate thread, in which case, the function has already been executed.
*/
void PKExecuteOnRenderThread(const TFunctionRef<void()>& Function);

/**
* Utility function for easily running a TFunctionRef on the render thread. Must be invoked from the game thread.
* If rendering does not use a separate thread, the TFunction will be executed immediately, otherwise it will be added to the render thread task queue.
* This method will flush rendering commands meaning that the function will be executed before PKExecuteOnRenderThread returns.
* @param Function - the Function to be invoked on the render thread. When executed, it will get the current FRHICommandList instance passed in as its sole argument.
* @return true if the function was queued, false if rendering does not use a separate thread, in which case, the function has already been executed.
*/
void PKExecuteOnRenderThread(const TFunctionRef<void(FRHICommandListImmediate&)>& Function);

/**
* Utility function for easily submitting TFunction to be run on the RHI thread. Must be invoked from the render thread.
* If RHI does not run on a separate thread, the TFunction will be executed immediately, otherwise it will be added to the RHI thread command list.
* @param Function - the Function to be invoked on the RHI thread.
* @return true if the function was queued, false if RHI does not use a separate thread, or if it's bypassed, in which case, the function has already been executed.
*/
bool PKExecuteOnRHIThread_DoNotWait(const TFunction<void()>& Function);

/**
* Utility function for easily submitting TFunction to be run on the RHI thread. Must be invoked from the render thread.
* If RHI does not run on a separate thread, the TFunction will be executed immediately, otherwise it will be added to the RHI thread command list.
* @param Function - the Function to be invoked on the RHI thread. When executed, it will get the current FRHICommandList instance passed in as its sole argument.
* @return true if the function was queued, false if RHI does not use a separate thread, or if it's bypassed, in which case, the function has already been executed.
*/
bool PKExecuteOnRHIThread_DoNotWait(const TFunction<void(FRHICommandListImmediate&)>& Function);

/**
* Utility function for easily running a TFunctionRef on the RHI thread. Must be invoked from the render thread.
* If RHI does not run on a separate thread, the TFunction will be executed on the current thread.
* This method will flush the RHI command list meaning that the function will be executed before PKExecuteOnRHIThread returns.
* @param Function - the Function to be invoked on the RHI thread.
*/
void PKExecuteOnRHIThread(const TFunctionRef<void()>& Function);

/**
* Utility function for easily running a TFunctionRef on the RHI thread. Must be invoked from the render thread.
* If RHI does not run on a separate thread, the TFunction will be executed on the current thread.
* This method will flush the RHI command list meaning that the function will be executed before PKExecuteOnRHIThread returns.
* @param Function - the Function to be invoked on the RHI thread. When executed, it will get the current FRHICommandList instance passed in as its sole argument.
*/
void PKExecuteOnRHIThread(const TFunctionRef<void(FRHICommandListImmediate&)>& Function);
