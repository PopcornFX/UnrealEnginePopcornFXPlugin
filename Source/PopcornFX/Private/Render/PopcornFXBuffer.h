//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXMinimal.h"

#include "RHIDefinitions.h"
#include "RenderResource.h"

#include "PopcornFXSDK.h"
#include <pk_render_helpers/include/buffer_pool/rh_bufferpool.h>

//----------------------------------------------------------------------------

class	FPopcornFXVertexBuffer : public FVertexBuffer
{
public:
	enum : uint32 { Alignment = 0x10 };
	enum : uint32
	{
		Flag_UAV				= (1U << 31),
		Flag_SRV				= (1U << 31) - 1,
		Flag_ByteAddressBuffer	= (1U << 31) - 2,
		Flag_StrideMask			= (1U << 31) - 3,
	};

	FPopcornFXVertexBuffer()
		: m_CapacityInBytes(0), m_Flags(0), m_AllocatedCount(0), m_CurrentMap(null)
	{ }

	u32				CapacityInBytes() const { return m_CapacityInBytes; }

	bool			HardResize(u32 sizeInBytes);
	void			Release() { ReleaseResource(); }

	bool			UsedAsUAV() const { return (m_Flags & Flag_UAV) != 0; }
	bool			UsedAsSRV() const { return (m_Flags & Flag_SRV) != 0; }
	bool			UsedAsByteAddressBuffer() const { return (m_Flags & Flag_ByteAddressBuffer) != 0; }
	void			SetResourceUsage(bool uav, bool srv, bool byteAddressBuffer);
	bool			Mappable() const { return !UsedAsUAV(); }
	bool			Mapped() const { return m_CurrentMap != null; }
	void			*RawMap(u32 count, u32 stride);
	void			Unmap();

	void			_SetAllocated(u32 count, u32 stride) { m_AllocatedCount = count; PK_ASSERT(stride < Flag_StrideMask); m_Flags = (m_Flags & ~Flag_StrideMask) | stride; }
	u32				AllocatedStride() const { return m_Flags & Flag_StrideMask; }
	u32				AllocatedCount() const { return m_AllocatedCount; }
	u32				AllocatedSize() const { return AllocatedCount() * AllocatedStride(); }

	template <typename _Type>
	bool			Map(PopcornFX::TMemoryView<_Type> &outMap, u32 count)
	{
		void	*map = RawMap(count, outMap.Stride());
		if (map == null)
		{
			outMap.Clear();
			return false;
		}
		outMap = PopcornFX::TMemoryView<_Type>(reinterpret_cast<_Type*>(map), count);
		return true;
	}

	template <typename _Type, s32 _FootprintInBytes = -1>
	bool			Map(TStridedMemoryView<_Type, _FootprintInBytes> &outMap, u32 count, u32 stride)
	{
		void	*map = RawMap(count, stride);
		if (map == null)
		{
			outMap.Clear();
			return false;
		}
		outMap = TStridedMemoryView<_Type, _FootprintInBytes>(reinterpret_cast<_Type*>(map), count, stride);
		return true;
	}

	template <typename _Type, s32 _FootprintInBytes = -1>
	bool			Map(TStridedMemoryView<_Type, _FootprintInBytes> &outPreSetupedMap)
	{
		PK_ASSERT(outPreSetupedMap.Count() > 0 && outPreSetupedMap.Stride() > 0 && outPreSetupedMap.Data() == null);
		return Map(outPreSetupedMap, outPreSetupedMap.Count(), outPreSetupedMap.Stride());
	}

	const FUnorderedAccessViewRHIRef		&UAV();
	const FShaderResourceViewRHIRef			&SRV();

protected:
	virtual void	ReleaseRHI() override;
	virtual void	InitRHI() override;

private:

private:
	u32								m_CapacityInBytes;
	u32								m_Flags;
	u32								m_AllocatedCount;
	void							*m_CurrentMap;
	FUnorderedAccessViewRHIRef		m_UAV;
	FShaderResourceViewRHIRef		m_SRV;
};

//----------------------------------------------------------------------------

class	FPopcornFXIndexBuffer : public FIndexBuffer
{
public:
	enum : uint32 { Alignment = 0x10 };
	enum : uint32
	{
		Flag_Large				= (1U << 0),
		Flag_ByteAddressBuffer	= (1U << 1),
		Flag_SRV				= (1U << 2),
		Flag_UAV				= (1U << 3),
	};

	FPopcornFXIndexBuffer()
		: m_Capacity(0), m_Flags(0), m_AllocatedCount(0), m_CurrentMap(null)
	{ }

	virtual void	ReleaseRHI() override;
	virtual void	InitRHI() override;

	bool			Large() const { return m_Flags & Flag_Large; }
	u32				Stride() const { return Large() ? sizeof(u32) : sizeof(u16); }
	u32				Capacity() const { return m_Capacity; }

	bool			HardResize(u32 count);
	void			Release() { ReleaseResource(); }

	bool			UsedAsUAV() const { return (m_Flags & Flag_UAV) != 0; }
	bool			UsedAsSRV() const { return (m_Flags & Flag_SRV) != 0; }
	bool			UsedAsByteAddressBuffer() const { return (m_Flags & Flag_ByteAddressBuffer) != 0; }
	void			SetResourceUsage(bool uav, bool srv, bool byteAddressBuffer);
	bool			Mappable() const { return !UsedAsUAV(); }
	void			*RawMap(u32 count);
	bool			Mapped() const { return m_CurrentMap != null; }
	void			Unmap();

	void			_SetAllocated(u32 count) { m_AllocatedCount = count; }
	u32				AllocatedCount() const { return m_AllocatedCount; }
	u32				AllocatedStride() const { return Stride(); }
	u32				AllocatedSize() const { return AllocatedCount() * AllocatedStride(); }

	bool			Map(void *&outMap, bool &outLarge, u32 count)
	{
		outLarge = Large();
		void	*map = RawMap(count);
		if (map == null)
			return false;
		outMap = map;
		return true;
	}

	bool			Map(PopcornFX::TMemoryView<u32> &outMap, u32 count)
	{
		PK_ASSERT(Large());
		void	*map = RawMap(count);
		if (map == null)
		{
			outMap.Clear();
			return false;
		}
		outMap = PopcornFX::TMemoryView<u32>(reinterpret_cast<u32*>(map), count);
		return true;
	}
	bool			Map(PopcornFX::TMemoryView<u16> &outMap, u32 count)
	{
		PK_ASSERT(!Large());
		void	*map = RawMap(count);
		if (map == null)
		{
			outMap.Clear();
			return false;
		}
		outMap = PopcornFX::TMemoryView<u16>(reinterpret_cast<u16*>(map), count);
		return true;
	}

	const FUnorderedAccessViewRHIRef		&UAV();
	const FShaderResourceViewRHIRef			&SRV();

private:
	u32				m_Capacity;
	u32				m_Flags;
	u32				m_AllocatedCount;
	void			*m_CurrentMap;
	FUnorderedAccessViewRHIRef		m_UAV;
	FShaderResourceViewRHIRef		m_SRV;
};

//----------------------------------------------------------------------------
//
// BufferPools
//
//----------------------------------------------------------------------------

struct SPopcornFXBufferPoolAllocatorPolicy
{
public:
	enum { kFrameTicksBeforeReuse = 2U };
	enum { kGCTicksBeforeRelease = 4U };
	enum { kLevelCount = 20U };
	enum { kBasePow = 9 };
	u32		LevelToSize(u32 level)
	{
		return 1 << (level + kBasePow);
	}
	u32		SizeToLevel(u32 size)
	{
		return PopcornFX::PKMax(PopcornFX::IntegerTools::Log2(size), u32(kBasePow)) - kBasePow + 1;
	}
	u32		LevelCountToLookahead(u32 level)
	{
		return (kLevelCount - level) * 1 / 4;
		//return 1U + (PKMax(u32(kMaxLevel) - level, 8U) - 8U) / 2U;
	}
};

//----------------------------------------------------------------------------

class CBufferPool_VertexBufferPolicy : public PopcornFX::TBaseBufferPoolPolicy<FPopcornFXVertexBuffer, SPopcornFXBufferPoolAllocatorPolicy>
{
public:
	bool		m_BuffersUsedAsUAV = false;
	bool		m_BuffersUsedAsSRV = false;
	bool		m_ByteAddressBuffers = false;
protected:
	bool		BufferAllocate(CBuffer &buffer, u32 sizeInBytes)
	{
		check(IsInRenderingThread());
		buffer.SetResourceUsage(m_BuffersUsedAsUAV, m_BuffersUsedAsSRV, m_ByteAddressBuffers);
		return buffer.HardResize(sizeInBytes);
	}

	void		BufferRelease(CBuffer &buffer)
	{
		check(IsInRenderingThread());
		buffer.Release();
	}
};

//----------------------------------------------------------------------------

class CBufferPool_IndexBufferPolicy : public PopcornFX::TBaseBufferPoolPolicy<FPopcornFXIndexBuffer, SPopcornFXBufferPoolAllocatorPolicy>
{
public:
	bool		m_BuffersUsedAsUAV = false;
	bool		m_BuffersUsedAsSRV = false;
	bool		m_ByteAddressBuffers = false;
protected:
	bool		BufferAllocate(CBuffer &buffer, u32 sizeInBytes)
	{
		check(IsInRenderingThread());
		buffer.SetResourceUsage(m_BuffersUsedAsUAV, m_BuffersUsedAsSRV, m_ByteAddressBuffers);
		return buffer.HardResize(sizeInBytes);
	}

	void		BufferRelease(CBuffer &buffer)
	{
		check(IsInRenderingThread());
		buffer.Release();
	}
};

//----------------------------------------------------------------------------

class CVertexBufferPool : public PopcornFX::TBufferPool < FPopcornFXVertexBuffer, CBufferPool_VertexBufferPolicy >
{
public:
	typedef PopcornFX::TBufferPool < FPopcornFXVertexBuffer, CBufferPool_VertexBufferPolicy > Super;

	template <typename _Type, s32 _FootprintInBytes>
	bool		AllocateAndMapIf(bool doIt, CPooledBuffer &outPBuffer, TStridedMemoryView<_Type, _FootprintInBytes> &outPreSetupedMap)
	{
		outPBuffer.Clear();
		if (doIt)
		{
			PK_ASSERT(outPreSetupedMap.Count() > 0 && outPreSetupedMap.Stride() > 0 && outPreSetupedMap.Data() == null);
			if (!Super::Allocate(outPBuffer, outPreSetupedMap.CoveredBytes()))
				return false;
			outPBuffer->_SetAllocated(outPreSetupedMap.Count(), outPreSetupedMap.Stride());

			if (!outPBuffer->Map(outPreSetupedMap))
				return false;
		}
		else
			outPreSetupedMap = TStridedMemoryView<_Type, _FootprintInBytes>();
		return true;
	}

	bool		AllocateIf(bool doIt, CPooledBuffer &outPBuffer, u32 count, u32 stride, bool clear = true)
	{
		if (clear)
			outPBuffer.Clear();
		if (doIt)
		{
			PK_ASSERT(count > 0);
			if (!Super::Allocate(outPBuffer, count * stride))
				return false;
			outPBuffer->_SetAllocated(count, stride);
		}
		return true;
	}

	bool		Allocate(CPooledBuffer &outPBuffer, u32 count, u32 stride, bool clear = true)
	{
		PK_ASSERT(count > 0);
		if (clear)
			outPBuffer.Clear();
		if (!Super::Allocate(outPBuffer, count * stride))
			return false;
		outPBuffer->_SetAllocated(count, stride);
		return true;
	}
};

//----------------------------------------------------------------------------

class CIndexBufferPool : public PopcornFX::TBufferPool < FPopcornFXIndexBuffer, CBufferPool_IndexBufferPolicy >
{
public:
	typedef PopcornFX::TBufferPool < FPopcornFXIndexBuffer, CBufferPool_IndexBufferPolicy >		Super;

	bool		AllocateAndMapIf(bool doIt, CPooledBuffer &outPBuffer, void *&outMap, bool &outLargeIndices, u32 indexCount)
	{
		outPBuffer.Clear();
		outLargeIndices = false;
		outMap = null;
		if (doIt)
		{
			if (!Super::Allocate(outPBuffer, indexCount))
				return false;
			outPBuffer->_SetAllocated(indexCount);
			if (!outPBuffer->Map(outMap, outLargeIndices, indexCount))
				return false;
		}
		return true;
	}

	bool		AllocateIf(bool doIt, CPooledBuffer &outPBuffer, bool &outLargeIndices, u32 indexCount, bool clear = true)
	{
		if (clear)
			outPBuffer.Clear();
		outLargeIndices = false;
		if (doIt)
		{
			if (!Super::Allocate(outPBuffer, indexCount))
				return false;
			outPBuffer->_SetAllocated(indexCount);
			outLargeIndices = outPBuffer->Large();
		}
		return true;
	}


	bool		Allocate(CPooledBuffer &outPBuffer, bool &outLargeIndices, u32 indexCount, bool clear = true)
	{
		if (clear)
			outPBuffer.Clear();
		outLargeIndices = false;
		if (!Super::Allocate(outPBuffer, indexCount))
			return false;
		outPBuffer->_SetAllocated(indexCount);
		outLargeIndices = outPBuffer->Large();
		return true;
	}
};

//----------------------------------------------------------------------------

class CPooledVertexBuffer : public CVertexBufferPool::CPooledBuffer
{
public:
	void		UnmapAndClear()
	{
		if (Valid())
		{
			if (Buffer()->Mapped())
				Buffer()->Unmap();
			Clear();
		}
	}
	void		Unmap()
	{
		if (Valid())
		{
			if (Buffer()->Mapped())
				Buffer()->Unmap();
		}
	}
};

class CPooledIndexBuffer : public CIndexBufferPool::CPooledBuffer
{
public:
	void		UnmapAndClear()
	{
		if (Valid())
		{
			if (Buffer()->Mapped())
				Buffer()->Unmap();
			Clear();
		}
	}
	void		Unmap()
	{
		if (Valid())
		{
			if (Buffer()->Mapped())
				Buffer()->Unmap();
		}
	}
};

//----------------------------------------------------------------------------
//
// Generic Buffer View
//
//----------------------------------------------------------------------------

class	FPopcornFXVertexBufferView
{
protected:
	FVertexBuffer			*m_Buffer = null;
	uint32					m_Offset = 0;
	uint32					m_Stride = 0;
	CPooledVertexBuffer		m_ForRefCount;

public:
	bool				Valid() const { return !Empty(); }
	bool				Empty() const { return m_Buffer == null; }

	uint32				Offset() const { return m_Offset; }
	uint32				Stride() const { return m_Stride; }
	const FVertexBuffer		*VertexBufferPtr() const { return m_Buffer; }
	FVertexBuffer			*VertexBufferPtr() { return m_Buffer; }

	void		Clear()
	{
		if (Empty())
		{
			PK_ASSERT(m_Buffer == null);
			PK_ASSERT(m_Offset == 0);
			PK_ASSERT(m_Stride == 0);
			PK_ASSERT(!m_ForRefCount.Valid());
			return;
		}
		m_Buffer = null;
		m_Offset = 0;
		m_Stride = 0;
		m_ForRefCount.Clear();
	}

	void		Setup(FVertexBuffer *buffer, uint32 offset, uint32 stride)
	{
		Clear();
		PK_ASSERT(buffer != null);
		m_Buffer = buffer;
		PK_ASSERT(m_Buffer->IsInitialized());
		m_Offset = offset;
		m_Stride = stride;
	}

	void		Setup(const FPopcornFXVertexBufferView &other)
	{
		*this = other;
	}

	void		Setup(const CPooledVertexBuffer &buffer, uint32 offset = 0)
	{
		Clear();
		if (buffer.Valid())
		{
			m_ForRefCount = buffer;
			m_Buffer = buffer.Buffer();
			PK_ASSERT(m_Buffer->IsInitialized());
			m_Offset = offset;
			m_Stride = buffer->AllocatedStride();
		}
	}

};

inline bool		IsValidRef(const FPopcornFXVertexBufferView &buffer)
{
	return buffer.Valid();
}
