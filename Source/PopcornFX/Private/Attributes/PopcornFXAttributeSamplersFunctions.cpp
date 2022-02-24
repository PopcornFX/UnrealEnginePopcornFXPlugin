//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSamplersFunctions.h"

#include "Engine/World.h"
#include "PopcornFXPlugin.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeList.h"
#include "PopcornFXSDK.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplersFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplersFunctions, Log, All);

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplersFunctions::UPopcornFXAttributeSamplersFunctions(class FObjectInitializer const &pcip)
:	Super(pcip)
{
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSampler(UPopcornFXEmitterComponent *InSelf, FName InAttributeSamplerName, AActor* InActor, FName InComponentName)
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
#undef LOCTEXT_NAMESPACE
