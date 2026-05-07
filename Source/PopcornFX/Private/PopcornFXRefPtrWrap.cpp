//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXRefPtrWrap.h"

#include "PopcornFXSDK.h"
#include <pk_kernel/include/kr_refptr.h>
#include <pk_kernel/include/kr_mem_utils.h>

//----------------------------------------------------------------------------

void	CPopcornFXRefPtrWrap_Base::Set(PopcornFX::CRefCountedObject *rawPtr)
{
	Clear();
	if (rawPtr != null)
	{
		//m_RefTrackingSlot = const_cast<_WriteableType*>(m_Ptr)->AddReference(ONLY_IN_UBER_REFPTR_DEBUG(true));
		//PK_ASSERT(PK_MEM_LOOKS_VALID(rawPtr));
		m_Ptr = rawPtr;
		const_cast<PopcornFX::CRefCountedObject*>(m_Ptr)->AddReference();
	}
}

//----------------------------------------------------------------------------

void	CPopcornFXRefPtrWrap_Base::Clear()
{
	if (m_Ptr != null)
	{
		//PK_ASSERT(PK_MEM_LOOKS_VALID(m_Ptr));
		//const_cast<_WriteableType*>(m_Ptr)->RemoveReference(m_Ptr ONLY_IN_UBER_REFPTR_DEBUG(COMMA m_RefTrackingSlot));
		//ONLY_IN_UBER_REFPTR_DEBUG(m_RefTrackingSlot = u32(-1));
		const_cast<PopcornFX::CRefCountedObject*>(m_Ptr)->RemoveReference(m_Ptr);
		m_Ptr = null;
	}
}

//----------------------------------------------------------------------------
