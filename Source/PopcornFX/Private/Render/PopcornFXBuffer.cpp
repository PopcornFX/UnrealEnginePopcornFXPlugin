//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXBuffer.h"

#include <pk_kernel/include/kr_profiler.h>

#include "GPUSim/PopcornFXGPUSim.h"

//----------------------------------------------------------------------------
//
// FPopcornFXVertexBuffer
//
//----------------------------------------------------------------------------

void	FPopcornFXVertexBuffer::ReleaseRHI()
{
	PK_ASSERT(!Mapped());
	m_CapacityInBytes = 0;
	m_AllocatedCount = 0;
	FVertexBuffer::ReleaseRHI();
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
void	FPopcornFXVertexBuffer::InitRHI(FRHICommandListBase &RHICmdList)
#else
void	FPopcornFXVertexBuffer::InitRHI()
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
{
	m_UAV = null;
	m_SRV = null;
	if (m_CapacityInBytes > 0)
	{
#if (ENGINE_MAJOR_VERSION == 5)
		EBufferUsageFlags		usage = EBufferUsageFlags::None;
#else
		EBufferUsageFlags		usage = EBufferUsageFlags::BUF_None;
#endif // (ENGINE_MAJOR_VERSION == 5)
		if (UsedAsUAV() || UsedAsSRV())
		{
			usage |= BUF_ShaderResource;
			if (UsedAsByteAddressBuffer())
				usage |= BUF_ByteAddressBuffer;
			if (UsedAsUAV())
				usage |= BUF_UnorderedAccess;
			else
				usage |= BUF_Dynamic;
		}
		else
			usage |= BUF_Dynamic;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		FRHIBufferCreateDesc	bufferDesc =
			FRHIBufferCreateDesc::Create(TEXT("PopcornFX Buffer"), m_CapacityInBytes, 0, usage | EBufferUsageFlags::VertexBuffer)
			.SetInitialState(RHIGetDefaultResourceState(usage | EBufferUsageFlags::VertexBuffer, false));
		VertexBufferRHI = RHICmdList.CreateBuffer(bufferDesc);
#elif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHIResourceCreateInfo	emptyInformations(TEXT("PopcornFX Buffer"));
		VertexBufferRHI = RHICmdList.CreateVertexBuffer(m_CapacityInBytes, usage, emptyInformations);
#else
		VertexBufferRHI = RHICreateVertexBuffer(m_CapacityInBytes, usage, emptyInformations);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		if (!PK_VERIFY(IsValidRef(VertexBufferRHI)))
			m_CapacityInBytes = 0;
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXVertexBuffer::SetResourceUsage(bool uav, bool srv, bool byteAddressBuffer)
{
	if (uav)
		m_Flags |= Flag_UAV;
	else
		m_Flags &= ~Flag_UAV;

	if (srv)
		m_Flags |= Flag_SRV;
	else
		m_Flags &= ~Flag_SRV;

	if (byteAddressBuffer)
		m_Flags |= Flag_ByteAddressBuffer;
	else
		m_Flags &= ~Flag_ByteAddressBuffer;
}

//----------------------------------------------------------------------------

bool	FPopcornFXVertexBuffer::HardResize(u32 sizeInBytes)
{
	if (!PK_VERIFY(!Mapped()))
		return false;
	PK_ASSERT(m_CapacityInBytes == 0);
	const u32		capacity = PopcornFX::Mem::Align<Alignment>(sizeInBytes);
	m_CapacityInBytes = capacity;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (!IsInitialized())
		InitResource(RHICmdList);
	else
		UpdateRHI(RHICmdList);
#else
	if (!IsInitialized())
		InitResource();
	else
		UpdateRHI();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	return m_CapacityInBytes == capacity;
}

//----------------------------------------------------------------------------

void	*FPopcornFXVertexBuffer::RawMap(u32 count, u32 stride)
{
	PK_ASSERT((m_Flags & Flag_StrideMask) == stride);
	PK_ASSERT(count <= m_AllocatedCount);

	if (!PK_VERIFY(Mappable()))
		return null;
	if (!PK_VERIFY(!Mapped()))
		return null;

	const u32		sizeInBytes = count * stride;
	const u32		sizeToMap = PopcornFX::Mem::Align<Alignment>(sizeInBytes);
	if (!PK_VERIFY(sizeToMap > 0 && sizeToMap <= m_CapacityInBytes))
		return null;
	if (!PK_VERIFY(IsValidRef(VertexBufferRHI)))
		return null;
	void	*map;
	{
		PK_NAMEDSCOPEDPROFILE("FPopcornFXVertexBuffer::RawMap RHILockVertexBuffer");
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		map = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(VertexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#elif (ENGINE_MAJOR_VERSION == 5)
		map = RHILockBuffer(VertexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#else
		map = RHILockVertexBuffer(VertexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#endif // (ENGINE_MAJOR_VERSION == 5)
	}
	PK_ASSERT(map != null);
	PK_ASSERT(PopcornFX::Mem::IsAligned<Alignment>(map));
	m_CurrentMap = map;
	return map;
}

//----------------------------------------------------------------------------

void	FPopcornFXVertexBuffer::Unmap()
{
	if (Mappable() && Mapped())
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(VertexBufferRHI);
#elif (ENGINE_MAJOR_VERSION == 5)
		RHIUnlockBuffer(VertexBufferRHI);
#else
		RHIUnlockVertexBuffer(VertexBufferRHI);
#endif // (ENGINE_MAJOR_VERSION == 5)
		m_CurrentMap = null;
	}
}

//----------------------------------------------------------------------------

const FUnorderedAccessViewRHIRef	&FPopcornFXVertexBuffer::UAV()
{
#if (PK_HAS_GPU != 0)
	PK_ASSERT(UsedAsUAV());
	if (!IsValidRef(m_UAV))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		if (VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer))
		{
			m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(VertexBufferRHI, FRHIViewDesc::CreateBufferUAV()
				.SetType(FRHIViewDesc::EBufferType::Raw)
			);
		}
		else
		{
			m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(VertexBufferRHI, FRHIViewDesc::CreateBufferUAV()
				.SetType(FRHIViewDesc::EBufferType::Typed)
				.SetFormat(EPixelFormat(PF_R32_UINT))
			);
		}
#elif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(VertexBufferRHI, PF_R32_UINT);
#else
		m_UAV = RHICreateUnorderedAccessView(VertexBufferRHI, PF_R32_UINT);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}
	PK_ASSERT(IsValidRef(m_UAV));
#else
	PK_ASSERT_NOT_REACHED();
#endif
	return m_UAV;
}

//----------------------------------------------------------------------------

const FShaderResourceViewRHIRef		&FPopcornFXVertexBuffer::SRV()
{
	PK_ASSERT(UsedAsSRV());
	if (!IsValidRef(m_SRV))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		if (VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer))
		{
			m_SRV = FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(VertexBufferRHI, FRHIViewDesc::CreateBufferSRV()
				.SetType(FRHIViewDesc::EBufferType::Raw)
			);
		}
		else
		{
			m_SRV = FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(VertexBufferRHI, FRHIViewDesc::CreateBufferSRV()
				.SetType(FRHIViewDesc::EBufferType::Typed)
				.SetFormat(EPixelFormat(PF_R32_UINT))
			);
		}
#elif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		m_SRV = FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(VertexBufferRHI, sizeof(u32), PF_R32_UINT);
#else
		m_SRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(u32), PF_R32_UINT);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}
	PK_ASSERT(IsValidRef(m_SRV));
	return m_SRV;
}

//----------------------------------------------------------------------------
//
// FPopcornFXIndexBuffer
//
//----------------------------------------------------------------------------

void	FPopcornFXIndexBuffer::ReleaseRHI()
{
	PK_ASSERT(!Mapped());
	m_Capacity = 0;
	m_Flags &= ~Flag_Large;
	m_AllocatedCount = 0;
	FIndexBuffer::ReleaseRHI();
}

//----------------------------------------------------------------------------

#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
void	FPopcornFXIndexBuffer::InitRHI(FRHICommandListBase &RHICmdList)
#else
void	FPopcornFXIndexBuffer::InitRHI()
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
{
	m_UAV = null;
	m_SRV = null;
	PK_TODO("u16 indices for GPU");
	if (UsedAsUAV() || m_Capacity > 0xFFFF)
		m_Flags |= Flag_Large;
	else
		m_Flags &= ~Flag_Large;
	if (m_Capacity > 0)
	{
#if (ENGINE_MAJOR_VERSION == 5)
		EBufferUsageFlags		usage = EBufferUsageFlags::None;
#else
		EBufferUsageFlags		usage = EBufferUsageFlags::BUF_None;
#endif // (ENGINE_MAJOR_VERSION == 5)
		if (UsedAsUAV() || UsedAsSRV())
		{
			usage |= BUF_ShaderResource;
			if (UsedAsByteAddressBuffer())
				usage |= BUF_ByteAddressBuffer;
			if (UsedAsUAV())
				usage |= BUF_UnorderedAccess;
			else
				usage |= BUF_Dynamic;
		}
		else
			usage |= BUF_Dynamic;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		FRHIBufferCreateDesc	bufferDesc =
			FRHIBufferCreateDesc::Create(TEXT("PopcornFX Buffer"), m_Capacity * Stride(), Stride(), usage | EBufferUsageFlags::IndexBuffer)
			.SetInitialState(RHIGetDefaultResourceState(usage | EBufferUsageFlags::IndexBuffer, false));
		IndexBufferRHI = RHICmdList.CreateBuffer(bufferDesc);
#elif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHIResourceCreateInfo	emptyInformations(TEXT("PopcornFX Index Buffer"));
		IndexBufferRHI = RHICmdList.CreateIndexBuffer(Stride(), m_Capacity * Stride(), usage, emptyInformations);
#else
		IndexBufferRHI = RHICreateIndexBuffer(Stride(), m_Capacity * Stride(), usage, emptyInformations);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		if (!PK_VERIFY(IsValidRef(IndexBufferRHI)))
		{
			m_Capacity = 0;
			m_Flags &= ~Flag_Large;
		}
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXIndexBuffer::SetResourceUsage(bool uav, bool srv, bool byteAddressBuffer)
{
	if (uav)
		m_Flags |= Flag_UAV;
	else
		m_Flags &= ~Flag_UAV;

	if (srv)
		m_Flags |= Flag_SRV;
	else
		m_Flags &= ~Flag_SRV;

	if (byteAddressBuffer)
		m_Flags |= Flag_ByteAddressBuffer;
	else
		m_Flags &= ~Flag_ByteAddressBuffer;
}

//----------------------------------------------------------------------------

bool	FPopcornFXIndexBuffer::HardResize(u32 count)
{
	if (IsInitialized() && !PK_VERIFY(Mappable()))
		return false;

	if (!PK_VERIFY(!Mapped()))
		return false;
	const u32		capacity = PopcornFX::Mem::Align<Alignment>(count);
	m_Capacity = capacity;
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (!IsInitialized())
		InitResource(RHICmdList);
	else
		UpdateRHI(RHICmdList);
#else
	if (!IsInitialized())
		InitResource();
	else
		UpdateRHI();
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	return m_Capacity == capacity;
}

//----------------------------------------------------------------------------

void	*FPopcornFXIndexBuffer::RawMap(u32 countToMap)
{
	PK_ASSERT(countToMap <= m_AllocatedCount);

	if (!PK_VERIFY(Mappable()))
		return null;

	if (!PK_VERIFY(!Mapped()))
		return null;
	const u32		sizeToMap = PopcornFX::Mem::Align<Alignment>(countToMap * Stride());
	if (!PK_VERIFY(sizeToMap > 0 && sizeToMap <= m_Capacity * Stride()))
		return null;
	if (!PK_VERIFY(IsValidRef(IndexBufferRHI)))
		return null;
	void	*map;
	{
		PK_NAMEDSCOPEDPROFILE("FPopcornFXIndexBuffer::RawMap RHILockBuffer");
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		map = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(IndexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#elif (ENGINE_MAJOR_VERSION == 5)
		map = RHILockBuffer(IndexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#else
		map = RHILockIndexBuffer(IndexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
#endif // (ENGINE_MAJOR_VERSION == 5)
	}
	PK_ASSERT(map != null);
	PK_ASSERT(PopcornFX::Mem::IsAligned<Alignment>(map));
	m_CurrentMap = map;
	return map;
}

//----------------------------------------------------------------------------

void	FPopcornFXIndexBuffer::Unmap()
{
	if (Mappable() && Mapped())
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(IndexBufferRHI);
#elif (ENGINE_MAJOR_VERSION == 5)
		RHIUnlockBuffer(IndexBufferRHI);
#else
		RHIUnlockIndexBuffer(IndexBufferRHI);
#endif // (ENGINE_MAJOR_VERSION == 5)
		m_CurrentMap = null;
	}
}

//----------------------------------------------------------------------------

const FUnorderedAccessViewRHIRef	&FPopcornFXIndexBuffer::UAV()
{
#if (PK_HAS_GPU != 0)
	PK_ASSERT(UsedAsUAV());
	if (!IsValidRef(m_UAV))
	{
#if (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)
		if (IndexBufferRHI && EnumHasAnyFlags(IndexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer))
		{
			m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(IndexBufferRHI, FRHIViewDesc::CreateBufferUAV()
				.SetType(FRHIViewDesc::EBufferType::Raw)
			);
		}
		else
		{
			m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(IndexBufferRHI, FRHIViewDesc::CreateBufferUAV()
				.SetType(FRHIViewDesc::EBufferType::Typed)
				.SetFormat(EPixelFormat(PF_R32_UINT))
			);
		}
#elif (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
		m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(IndexBufferRHI, PF_R32_UINT);
#else
		m_UAV = RHICreateUnorderedAccessView(IndexBufferRHI, PF_R32_UINT);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 3)
	}
	PK_ASSERT(IsValidRef(m_UAV));
#else
	PK_ASSERT_NOT_REACHED();
#endif
	return m_UAV;
}

//----------------------------------------------------------------------------

const FShaderResourceViewRHIRef		&FPopcornFXIndexBuffer::SRV()
{
	PK_ASSERT(UsedAsSRV());
	if (!IsValidRef(m_SRV))
	{
		// Not supported
		//m_SRV = /*My_*/RHICreateShaderResourceView(IndexBufferRHI, PF_R32_UINT);
	}
	PK_ASSERT(IsValidRef(m_SRV));
	return m_SRV;
}

//----------------------------------------------------------------------------
