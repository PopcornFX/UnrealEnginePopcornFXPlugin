//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

FWD_PK_API_BEGIN
template <typename _Type> class TRefPtr;
class CRefCountedObject;
FWD_PK_API_END
// Statement to help the UE Header Parser not crash on FWD_PK_API_...
class	FPopcornFXPlugin;

class POPCORNFX_API CPopcornFXRefPtrWrap_Base
{
public:
	inline CPopcornFXRefPtrWrap_Base() : m_Ptr(nullptr) { }
	inline CPopcornFXRefPtrWrap_Base(PopcornFX::CRefCountedObject *rawPtr) : m_Ptr(nullptr) { Set(rawPtr); }
	inline CPopcornFXRefPtrWrap_Base(const CPopcornFXRefPtrWrap_Base &other) : m_Ptr(nullptr) { Set(other.m_Ptr); }
	inline ~CPopcornFXRefPtrWrap_Base() { Clear(); }
	inline const CPopcornFXRefPtrWrap_Base		&operator = (const CPopcornFXRefPtrWrap_Base &other) { Set(other.m_Ptr); return *this; }
	inline const CPopcornFXRefPtrWrap_Base		&operator = (PopcornFX::CRefCountedObject *rawPtr) { Set(rawPtr); return *this; }
	inline PopcornFX::CRefCountedObject		*GetRaw() const { return m_Ptr; }
	inline void								Swap(CPopcornFXRefPtrWrap_Base &other) { PopcornFX::CRefCountedObject *ptr = m_Ptr; m_Ptr = other.m_Ptr; other.m_Ptr = ptr; }

	// !! NO INLINE, those will do the PopcornFX stuff
	void									Clear();
	void									Set(PopcornFX::CRefCountedObject *rawPtr);

private:
	PopcornFX::CRefCountedObject		*m_Ptr;
};

/**
PopcornFX::TRefPtr<_Type> behavior without _Type declaration required (works even if _Type is only forward declared).
PopcornFX Object Ref Counter are intrusive, so we can safely do that.
*/
template <typename _Type>
class TPopcornFXRefPtrWrap : private CPopcornFXRefPtrWrap_Base
{
public:
	typedef TPopcornFXRefPtrWrap<_Type>		Self;
	inline TPopcornFXRefPtrWrap() { }
	inline TPopcornFXRefPtrWrap(_Type *ptr) : CPopcornFXRefPtrWrap_Base(ptr) { }
	inline _Type		*Get() const { return static_cast<_Type*>(GetRaw()); }
	inline _Type		&operator * () const { check(Get() != nullptr); return *Get(); }
	inline _Type		*operator -> () const { check(Get() != nullptr); return Get(); }
	inline bool			operator == (_Type *ptr) const { return ptr == GetRaw(); }
	inline bool			operator != (_Type *ptr) const { return ptr != GetRaw(); }
	inline bool			operator == (const Self &other) const { return other.GetRaw() == GetRaw(); }
	inline bool			operator != (const Self &other) const { return other.GetRaw() != GetRaw(); }
	inline void			Swap(Self &other) { CPopcornFXRefPtrWrap_Base::Swap(other); }
};
