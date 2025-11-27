//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXRenderUtils.h"
#include "Engine/Engine.h"

#include <pk_render_helpers/include/render_features/rh_features_basic.h>

DEFINE_LOG_CATEGORY_STATIC(LogVertexBillboardingPolicy, Log, All);

//----------------------------------------------------------------------------
//
// FPopcornFXAtlasRectsVertexBuffer
//
//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects)
{
	if (rects.Empty())
	{
		Clear();
		return false;
	}
	if (!_LoadRects(rects))
	{
		Clear();
		return false;
	}
	return true;
}

//----------------------------------------------------------------------------

bool	FPopcornFXAtlasRectsVertexBuffer::_LoadRects(const PopcornFX::TMemoryView<const CFloat4> &rects)
{
	const u32		bytes = rects.CoveredBytes();
	const bool		empty = m_AtlasBuffer_Raw == null;

	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();

	if (empty || bytes > m_AtlasBufferCapacity)
	{
		m_AtlasBufferCapacity = PopcornFX::Mem::Align<0x100>(bytes + 0x10);
		const EBufferUsageFlags	usage = BUF_Static | BUF_ShaderResource;

#if (ENGINE_MINOR_VERSION >= 6)
		FRHIBufferCreateDesc CreateDesc =
			FRHIBufferCreateDesc::Create(TEXT("PopcornFX Atlas buffer"), m_AtlasBufferCapacity, sizeof(CFloat4), usage)
			.SetInitialState(RHIGetDefaultResourceState(usage | EBufferUsageFlags::VertexBuffer, false));
#else
		FRHIResourceCreateInfo	info(TEXT("PopcornFX Atlas buffer"));
#endif // (ENGINE_MINOR_VERSION >= 6)

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		m_AtlasBuffer_Raw = RHICmdList.CreateBuffer(CreateDesc);

		// For back-compat reasons, SRVs of byte-address buffers created via this function ignore the Format, and instead create raw views.
		if (m_AtlasBuffer_Raw && EnumHasAnyFlags(m_AtlasBuffer_Raw->GetDesc().Usage, BUF_ByteAddressBuffer))
		{
			m_AtlasBufferSRV = RHICmdList.CreateShaderResourceView(m_AtlasBuffer_Raw, FRHIViewDesc::CreateBufferSRV()
				.SetType(FRHIViewDesc::EBufferType::Raw)
			);
		}
		else
		{
			m_AtlasBufferSRV = RHICmdList.CreateShaderResourceView(m_AtlasBuffer_Raw, FRHIViewDesc::CreateBufferSRV()
				.SetType(FRHIViewDesc::EBufferType::Typed)
				.SetFormat(EPixelFormat(PF_A32B32G32R32F))
			);
		}

#else
		m_AtlasBuffer_Raw = RHICmdList.CreateVertexBuffer(m_AtlasBufferCapacity, usage, info);
		m_AtlasBufferSRV = RHICmdList.CreateShaderResourceView(m_AtlasBuffer_Raw, sizeof(CFloat4), PF_A32B32G32R32F);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)

		if (!PK_VERIFY(IsValidRef(m_AtlasBuffer_Raw)) ||
			!PK_VERIFY(IsValidRef(m_AtlasBufferSRV)))
			return false;
	}
	else
	{
		PK_ASSERT(m_AtlasBufferSRV != null);
	}

	void	*map = RHICmdList.LockBuffer(m_AtlasBuffer_Raw, 0, bytes, RLM_WriteOnly);
	if (!PK_VERIFY(map != null))
		return false;

	PopcornFX::Mem::Copy(map, rects.Data(), bytes);
	RHICmdList.UnlockBuffer(m_AtlasBuffer_Raw);
	m_AtlasRectsCount = rects.Count();
	return true;
}

//----------------------------------------------------------------------------
//
//	FNullFloat4Buffer
//
//----------------------------------------------------------------------------

void	FNullFloat4Buffer::InitRHI(FRHICommandListBase &RHICmdList)
{
	// Note: buffer is not initialized, only meant to calm down UE not letting bind null descriptors.
	// Draw calls binding this null buffer will access its data.

	const u32	totalByteCount = 4 * sizeof(float); // Whatever
	const EBufferUsageFlags	usage = BUF_Dynamic | BUF_ShaderResource;

#if (ENGINE_MINOR_VERSION >= 6)
	FRHIBufferCreateDesc CreateDesc =
		FRHIBufferCreateDesc::Create(TEXT("PopcornFX Null Float4 buffer"), totalByteCount, 0, usage)
		.SetGPUMask(FRHIGPUMask::All())
		.SetInitialState(ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask)
		.SetClassName(NAME_None)
		.SetOwnerName(NAME_None);
#else
	FRHIResourceCreateInfo	info(TEXT("PopcornFX Null Float4 buffer"));
#endif

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
	VertexBufferRHI = RHICmdList.CreateBuffer(CreateDesc);

	// For back-compat reasons, SRVs of byte-address buffers created via this function ignore the Format, and instead create raw views.
	if (VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer))
	{
		SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, FRHIViewDesc::CreateBufferSRV()
			.SetType(FRHIViewDesc::EBufferType::Raw)
		);
	}
	else
	{
		SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, FRHIViewDesc::CreateBufferSRV()
			.SetType(FRHIViewDesc::EBufferType::Typed)
			.SetFormat(EPixelFormat(PF_A32B32G32R32F))
		);
	}
#else
	VertexBufferRHI = RHICmdList.CreateBuffer(totalByteCount, usage, 0, ERHIAccess::VertexOrIndexBuffer | ERHIAccess::SRVMask, info);
	SRV = RHICmdList.CreateShaderResourceView(VertexBufferRHI, sizeof(CFloat4), PF_A32B32G32R32F);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
}

//----------------------------------------------------------------------------

void	FNullFloat4Buffer::ReleaseRHI()
{
	SRV.SafeRelease();
	FVertexBuffer::ReleaseRHI();
}

//----------------------------------------------------------------------------

TGlobalResource<FNullFloat4Buffer>	GPopcornFXNullFloat4Buffer;

//----------------------------------------------------------------------------
//
// ExecuteOn Render/RHI Thread (taken from XRBase plugin)
//
//----------------------------------------------------------------------------

void ExecuteOnRenderThread_DoNotWait(const TFunction<void()>& Function)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(ExecuteOnRenderThread)([Function](FRHICommandListImmediate& /*RHICmdList*/)
		{
			Function();
		});
}

void ExecuteOnRenderThread_DoNotWait(const TFunction<void(FRHICommandListImmediate&)>& Function)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(ExecuteOnRenderThread)([Function](FRHICommandListImmediate& RHICmdList)
		{
			Function(RHICmdList);
		});
}

void ExecuteOnRenderThread(const TFunctionRef<void()>& Function)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(ExecuteOnRenderThread)([Function](FRHICommandListImmediate& /*RHICmdList*/)
		{
			Function();
		});
	FlushRenderingCommands();
}

void ExecuteOnRenderThread(const TFunctionRef<void(FRHICommandListImmediate&)>& Function)
{
	check(IsInGameThread());

	ENQUEUE_RENDER_COMMAND(ExecuteOnRenderThread)([Function](FRHICommandListImmediate& RHICmdList)
		{
			Function(RHICmdList);
		});
	FlushRenderingCommands();
}

namespace
{
	FORCEINLINE void Invoke_Impl(const TFunction<void()>& Function, FRHICommandListImmediate& RHICmdList)
	{
		Function();
	}
	FORCEINLINE void Invoke_Impl(const TFunctionRef<void()>& Function, FRHICommandListImmediate& RHICmdList)
	{
		Function();
	}
	FORCEINLINE void Invoke_Impl(const TFunction<void(FRHICommandListImmediate&)>& Function, FRHICommandListImmediate& RHICmdList)
	{
		Function(RHICmdList);
	}
	FORCEINLINE void Invoke_Impl(const TFunctionRef<void(FRHICommandListImmediate&)>& Function, FRHICommandListImmediate& RHICmdList)
	{
		Function(RHICmdList);
	}

	template <typename T>
	struct FFunctionWrapperRHICommand final : public FRHICommand<FFunctionWrapperRHICommand<T>>
	{
		T Function;

		FFunctionWrapperRHICommand(const T& InFunction)
			: Function(InFunction)
		{
		}

		void Execute(FRHICommandListBase& RHICmdList)
		{
			Invoke_Impl(Function, *static_cast<FRHICommandListImmediate*>(&RHICmdList));
		}
	};

	template<typename T>
	FORCEINLINE bool ExecuteOnRHIThread_Impl(const T& Function, bool bFlush)
	{
		check(IsInRenderingThread() || IsInRHIThread());

		FRHICommandListImmediate& RHICmdList = GetImmediateCommandList_ForRenderCommand();

		if (!IsInRHIThread() && !RHICmdList.Bypass())
		{
			ALLOC_COMMAND_CL(RHICmdList, FFunctionWrapperRHICommand<T>)(Function);
			if (bFlush)
				RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
			return true;
		}
		else
		{
			Invoke_Impl(Function, RHICmdList);
			return false;
		}
	}
}

bool ExecuteOnRHIThread_DoNotWait(const TFunction<void()>& Function)
{
	return ExecuteOnRHIThread_Impl(Function, false);
}

bool ExecuteOnRHIThread_DoNotWait(const TFunction<void(FRHICommandListImmediate&)>& Function)
{
	return ExecuteOnRHIThread_Impl(Function, false);
}

void ExecuteOnRHIThread(const TFunctionRef<void()>& Function)
{
	ExecuteOnRHIThread_Impl(Function, true);
}

void ExecuteOnRHIThread(const TFunctionRef<void(FRHICommandListImmediate&)>& Function)
{
	ExecuteOnRHIThread_Impl(Function, true);
}
