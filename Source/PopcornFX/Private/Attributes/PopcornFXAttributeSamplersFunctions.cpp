//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplersFunctions.h"

#include "PopcornFXPlugin.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerVectorField.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXSDK.h"

#include "Engine/World.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplersFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplersFunctions, Log, All);

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplersFunctions::UPopcornFXAttributeSamplersFunctions(class FObjectInitializer const &pcip)
:	Super(pcip)
{
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName, AActor* InActor, FString InComponentName)
{
	if (InSelf == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't SetAttributeSampler: Invalid Self"));
		return false;
	}

	const UWorld	*world = InSelf->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		UPopcornFXAttributeList		*attrList = InSelf->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		attrList->SetAttributeSampler(InAttributeSamplerName, InActor, InComponentName);
	}
	return true;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSampler	*UPopcornFXAttributeSamplersFunctions::GetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	if (InSelf == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't SetAttributeSampler: Invalid Self"));
		return nullptr;
	}
	const UWorld *world = InSelf->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		UPopcornFXAttributeSampler	*sampler = InSelf->GetAttributeSampler(InAttributeSamplerName);
		if (!sampler)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't retrieve attribute sampler '%s'"), *InAttributeSamplerName);
		}
		return sampler;
	}
	return nullptr;
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerAnimTrack *UPopcornFXAttributeSamplersFunctions::GetAnimTrackAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerAnimTrack>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerCurve *UPopcornFXAttributeSamplersFunctions::GetCurveAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerCurve>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerGrid *UPopcornFXAttributeSamplersFunctions::GetGridAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerGrid>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerImage *UPopcornFXAttributeSamplersFunctions::GetImageAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerImage>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerShape *UPopcornFXAttributeSamplersFunctions::GetShapeAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerShape>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerText *UPopcornFXAttributeSamplersFunctions::GetTextAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerText>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplerVectorField *UPopcornFXAttributeSamplersFunctions::GetVectorFieldAttributeSampler(UPopcornFXEmitterComponent *InSelf, FString InAttributeSamplerName)
{
	return Cast<UPopcornFXAttributeSamplerVectorField>(GetAttributeSampler(InSelf, InAttributeSamplerName));
}

//----------------------------------------------------------------------------

UPopcornFXAttributeSampler *UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerCasted(UPopcornFXEmitterComponent *InSelf, TSubclassOf<UPopcornFXAttributeSampler> SamplerClass, FString InAttributeSamplerName)
{
	return GetAttributeSampler(InSelf, InAttributeSamplerName);
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
