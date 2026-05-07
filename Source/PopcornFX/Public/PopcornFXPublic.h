//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Runtime/Launch/Resources/Version.h"

// 4.15
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"

#include "PopcornFXVersionGenerated.h"
#include "PopcornFXFwdDeclSDK.h"

// LWC types
#if (ENGINE_MAJOR_VERSION == 4)
typedef FVector			FVector3f;
typedef FVector4		FVector4f;
typedef FVector2D		FVector2f;
typedef FVector			FVector3f;
typedef FMatrix			FMatrix44f;
typedef FQuat			FQuat4f;
typedef FBox			FBox3f;

typedef VectorRegister	VectorRegister4f;
#endif
