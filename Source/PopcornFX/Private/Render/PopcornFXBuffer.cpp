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

void	FPopcornFXVertexBuffer::InitRHI(FRHICommandListBase &RHICmdList)
{
	m_UAV = null;
	m_SRV = null;
	if (m_CapacityInBytes > 0)
	{
		EBufferUsageFlags		usage = EBufferUsageFlags::None;
		if (UsedAsUAV() || UsedAsSRV())
		{
			usage |= BUF_ShaderResource;
			if (UsedAsDrawIndirectBuffer())
				usage |= BUF_DrawIndirect;
			if (UsedAsByteAddressBuffer())
				usage |= BUF_ByteAddressBuffer;
			if (UsedAsUAV() && UsedAsSRV())
				usage |= BUF_StructuredBuffer;
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
#else
		FRHIResourceCreateInfo	emptyInformations(TEXT("PopcornFX Buffer"));
		VertexBufferRHI = RHICmdList.CreateVertexBuffer(m_CapacityInBytes, usage, emptyInformations);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)

		if (!PK_VERIFY(IsValidRef(VertexBufferRHI)))
			m_CapacityInBytes = 0;
	}
}

//----------------------------------------------------------------------------

void	FPopcornFXVertexBuffer::SetResourceUsage(bool uav, bool srv, bool byteAddressBuffer, bool drawIndirectBuffer)
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

	if (drawIndirectBuffer)
		m_Flags |= Flag_DrawIndirectBuffer;
	else
		m_Flags &= ~Flag_DrawIndirectBuffer;
}

//----------------------------------------------------------------------------

bool	FPopcornFXVertexBuffer::HardResize(u32 sizeInBytes)
{
	if (!PK_VERIFY(!Mapped()))
		return false;
	PK_ASSERT(m_CapacityInBytes == 0);

	const u32		capacity = PopcornFX::Mem::Align<Alignment>(sizeInBytes);
	m_CapacityInBytes = capacity;

	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (!IsInitialized())
		InitResource(RHICmdList);
	else
		UpdateRHI(RHICmdList);

	return m_CapacityInBytes == capacity;
}

//----------------------------------------------------------------------------

void	*FPopcornFXVertexBuffer::RawMap(u32 count, u32 stride)
{
	const u32	allocatedStride = AllocatedStride();
	PK_ASSERT(stride >= allocatedStride && stride % allocatedStride == 0);
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
		map = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(VertexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
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
		FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(VertexBufferRHI);
		m_CurrentMap = null;
	}
}

//----------------------------------------------------------------------------

void FPopcornFXVertexBuffer::_SetAllocated(u32 count, u32 stride)
{
	m_AllocatedCount = count;
	PK_ASSERT(stride < Flag_StrideMask);
	m_Flags &= ~Flag_StrideMask;
	m_Flags |= Flag_StrideMask & stride;
	m_UAV = null;
	m_SRV = null;
}

//----------------------------------------------------------------------------

const FUnorderedAccessViewRHIRef	&FPopcornFXVertexBuffer::UAV()
{
	if (!IsValidRef(m_UAV))
		m_UAV = CreateSubUAV(AllocatedCount());
	return m_UAV;
}

//----------------------------------------------------------------------------

const FShaderResourceViewRHIRef		&FPopcornFXVertexBuffer::SRV()
{
	if (!IsValidRef(m_SRV))
		m_SRV = CreateSubSRV(AllocatedCount());
	return m_SRV;
}

//----------------------------------------------------------------------------

FUnorderedAccessViewRHIRef	FPopcornFXVertexBuffer::CreateSubUAV(u32 elementCount, u32 startIdx, u32 strideInBytes)
{
#if (PK_HAS_GPU != 0)
	PK_ASSERT(UsedAsUAV());

	FUnorderedAccessViewRHIRef	uav = null;
	if (strideInBytes == 0)
		strideInBytes = AllocatedStride();

	const bool	isRaw			= VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer);
	const bool	isStructured	= VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_UnorderedAccess);
	uav = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(VertexBufferRHI, FRHIViewDesc::CreateBufferUAV()
		.SetType(isRaw ? FRHIViewDesc::EBufferType::Raw : (isStructured ? FRHIViewDesc::EBufferType::Structured : FRHIViewDesc::EBufferType::Typed))
		.SetStride(isStructured ? strideInBytes : 0)
		.SetFormat(!isRaw && !isStructured ? PF_R32_UINT : PF_Unknown)
		.SetNumElements(elementCount)
		.SetOffsetInBytes(startIdx * strideInBytes)
	);

	PK_ASSERT(IsValidRef(uav));
#else
	PK_ASSERT_NOT_REACHED();
#endif
	return uav;
}

//----------------------------------------------------------------------------

FShaderResourceViewRHIRef	FPopcornFXVertexBuffer::CreateSubSRV(u32 elementCount, u32 startIdx, u32 strideInBytes)
{
	PK_ASSERT(UsedAsSRV());

	FShaderResourceViewRHIRef	srv = null;
	if (strideInBytes == 0)
		strideInBytes = AllocatedStride();

	const bool	isRaw			= VertexBufferRHI && EnumHasAllFlags(VertexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer);
	const bool	isStructured	= VertexBufferRHI && EnumHasAnyFlags(VertexBufferRHI->GetDesc().Usage, BUF_UnorderedAccess);
	srv = FRHICommandListExecutor::GetImmediateCommandList().CreateShaderResourceView(VertexBufferRHI, FRHIViewDesc::CreateBufferSRV()
		.SetType(isRaw ? FRHIViewDesc::EBufferType::Raw : (isStructured ? FRHIViewDesc::EBufferType::Structured : FRHIViewDesc::EBufferType::Typed))
		.SetStride(isStructured ? strideInBytes : 0)
		.SetFormat(!isRaw && !isStructured ? PF_R32_UINT : PF_Unknown)
		.SetNumElements(elementCount)
		.SetOffsetInBytes(startIdx * strideInBytes)
	);

	PK_ASSERT(IsValidRef(srv));
	return srv;
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

void	FPopcornFXIndexBuffer::InitRHI(FRHICommandListBase &RHICmdList)
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
		EBufferUsageFlags		usage = EBufferUsageFlags::None;
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
#else
		FRHIResourceCreateInfo	emptyInformations(TEXT("PopcornFX Index Buffer"));
		IndexBufferRHI = RHICmdList.CreateIndexBuffer(Stride(), m_Capacity * Stride(), usage, emptyInformations);
#endif // (ENGINE_MAJOR_VERSION == 5) && (ENGINE_MINOR_VERSION >= 6)

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

	FRHICommandListImmediate	&RHICmdList = FRHICommandListExecutor::GetImmediateCommandList();
	if (!IsInitialized())
		InitResource(RHICmdList);
	else
		UpdateRHI(RHICmdList);

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
		map = FRHICommandListExecutor::GetImmediateCommandList().LockBuffer(IndexBufferRHI, 0, sizeToMap, RLM_WriteOnly);
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
		FRHICommandListExecutor::GetImmediateCommandList().UnlockBuffer(IndexBufferRHI);
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
		const bool	isRaw = IndexBufferRHI && EnumHasAnyFlags(IndexBufferRHI->GetDesc().Usage, BUF_ByteAddressBuffer);
		m_UAV = FRHICommandListExecutor::GetImmediateCommandList().CreateUnorderedAccessView(IndexBufferRHI, FRHIViewDesc::CreateBufferUAV()
			.SetType(isRaw ? FRHIViewDesc::EBufferType::Raw : FRHIViewDesc::EBufferType::Typed)
			.SetFormat(isRaw ? PF_Unknown : PF_R32_UINT)
		);
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
