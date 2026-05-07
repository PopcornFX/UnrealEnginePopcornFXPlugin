//--------------------------------------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved. https://www.popcornfx.com/terms-and-conditions/
//--------------------------------------------------------------------------------------------------------

#pragma once

#include "GCObject.h"
#include "SharedPointer.h"

/**
 * Like TStrongObjectPtr, but TStrongObjectPtrViaShared pass themselves a TSharedPtr instead of reallocating new TUniquePtr.
 * So, there is no allocations nor GC references added when it is just passed/copied around.
 * And, it doesn't allocate nor add GC reference until a valid pointer is given.
 *
 * The goal is to allocate and take ref on an UObject in the GameThread, and then pass it around in other thread without
 * touching the GC (where checks can fail if happening while gc collection is running in another thread).
 */

template <typename ObjectType>
class TStrongObjectPtrViaShared
{
public:
	TStrongObjectPtrViaShared()
		: ReferenceCollector() // does NOT touch GC
	{
		static_assert(TPointerIsConvertibleFromTo<ObjectType, const volatile UObject>::Value, "TStrongObjectPtrViaShared can only be constructed with UObject types");
	}

	explicit TStrongObjectPtrViaShared(ObjectType* InObject)
		: ReferenceCollector(MakeShared<FInternalReferenceCollector>(InObject)) // touches GC
	{
		static_assert(TPointerIsConvertibleFromTo<ObjectType, const volatile UObject>::Value, "TStrongObjectPtrViaShared can only be constructed with UObject types");
	}

	TStrongObjectPtrViaShared(const TStrongObjectPtrViaShared& InOther)
		: ReferenceCollector(InOther.ReferenceCollector) // does NOT touch GC
	{
	}

	template <typename OtherObjectType>
	TStrongObjectPtrViaShared(const TStrongObjectPtrViaShared<OtherObjectType>& InOther)
		: ReferenceCollector(InOther.ReferenceCollector)
	{
	}

	FORCEINLINE TStrongObjectPtrViaShared& operator=(const TStrongObjectPtrViaShared& InOther)
	{
		ReferenceCollector = InOther.ReferenceCollector;
		return *this;
	}

	template <typename OtherObjectType>
	FORCEINLINE TStrongObjectPtrViaShared& operator=(const TStrongObjectPtrViaShared<OtherObjectType>& InOther)
	{
		ReferenceCollector = InOther.ReferenceCollector;
		return *this;
	}

	FORCEINLINE ObjectType& operator*() const
	{
		check(IsValid());
		return *Get();
	}

	FORCEINLINE ObjectType* operator->() const
	{
		check(IsValid());
		return Get();
	}

	FORCEINLINE bool IsValid() const
	{
		return Get() != nullptr;
	}

	FORCEINLINE explicit operator bool() const
	{
		return Get() != nullptr;
	}

	FORCEINLINE ObjectType* Get() const
	{
		return ReferenceCollector.IsValid() ? ReferenceCollector->Get() : nullptr;
	}

	FORCEINLINE void Reset(ObjectType* InObject = nullptr)
	{
		if (InObject == nullptr)
			ReferenceCollector.Reset();
		else
			ReferenceCollector = MakeShared<FInternalReferenceCollector>(InObject);
	}

	FORCEINLINE friend uint32 GetTypeHash(const TStrongObjectPtrViaShared& InStrongObjectPtr)
	{
		return GetTypeHash(InStrongObjectPtr.Get());
	}

private:
	class FInternalReferenceCollector : public FGCObject
	{
	public:
		FInternalReferenceCollector(ObjectType* InObject)
			: Object(InObject)
		{
		}

		virtual ~FInternalReferenceCollector()
		{
		}

		FORCEINLINE ObjectType* Get() const
		{
			return Object;
		}

		FORCEINLINE void Set(ObjectType* InObject)
		{
			Object = InObject;
		}

		//~ FGCObject interface
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			Collector.AddReferencedObject(Object);
		}

	private:
		ObjectType* Object;
	};

	// `const` because we must not changed it: potentially shared with other TStrongObjectPtrViaShared
	// (or else we could check the ref count)
	TSharedPtr<const FInternalReferenceCollector>	ReferenceCollector;
};

template <typename LHSObjectType, typename RHSObjectType>
FORCEINLINE bool operator==(const TStrongObjectPtrViaShared<LHSObjectType>& InLHS, RHSObjectType* InRHS)
{
	return InLHS.Get() == InRHS;
}

template <typename LHSObjectType, typename RHSObjectType>
FORCEINLINE bool operator!=(const TStrongObjectPtrViaShared<LHSObjectType>& InLHS, RHSObjectType* InRHS)
{
	return InLHS.Get() != InRHS;
}
