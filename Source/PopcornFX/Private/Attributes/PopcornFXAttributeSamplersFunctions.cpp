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
#include "Misc/App.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSamplersFunctions"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSamplersFunctions, Log, All);

#define FIND_ATTRIBUTE_SAMPLER(SamplerName, SamplerType, OutSampler) \
	for (int32 attri = 0; attri < Emitter->AttributeList.m_SamplerDescs.Num(); ++attri) \
	{ \
		if (Emitter->AttributeList.m_SamplerDescs[attri].m_SamplerName == SamplerName) \
		{ \
			OutSampler = Emitter->AttributeList.m_Samplers[attri].m_Sampler## SamplerType .GetPtrOrNull(); \
		} \
	} \

#define CHECK_VALID_CALL(__ReturnValue) \
	if (Emitter == null) \
		return __ReturnValue; \
\
	const UWorld *world = Emitter->GetWorld(); \
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer))) \
	{ \
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList(); \
		if (!PK_VERIFY(attrList != null)) \
			return __ReturnValue; \
	}

//----------------------------------------------------------------------------

UPopcornFXAttributeSamplersFunctions::UPopcornFXAttributeSamplersFunctions(class FObjectInitializer const &pcip)
:	Super(pcip)
{
}

//---------------------------------------------------------------------------
// 
//	Sampler properties setters
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesAnimTrack &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler anim track properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler anim track properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::AnimTrack)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler anim track properties: sampler '%s' is not a anim track"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerAnimTrack *sampler = static_cast<FPopcornFXAttributeSamplerAnimTrack*>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesCurve &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler curve properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler curve properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Curve)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler curve properties: sampler '%s' is not a curve"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerCurve *sampler = static_cast<FPopcornFXAttributeSamplerCurve *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesGrid &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler grid properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler grid properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Grid)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler grid properties: sampler '%s' is not a grid"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerGrid *sampler = static_cast<FPopcornFXAttributeSamplerGrid *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesImage &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler image properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler image properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Image)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler image properties: sampler '%s' is not a image"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerImage *sampler = static_cast<FPopcornFXAttributeSamplerImage *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesShape &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler shape properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler shape properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Shape)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler shape properties: sampler '%s' is not a shape"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerShape *sampler = static_cast<FPopcornFXAttributeSamplerShape *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesText &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler text properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler text properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Text)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler text properties: sampler '%s' is not a text"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerText *sampler = static_cast<FPopcornFXAttributeSamplerText *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//----------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::SetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, const FPopcornFXAttributeSamplerPropertiesVectorField &InProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler vector field properties: invalid emitter"));
		return false;
	}

	const UWorld	*world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return false;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler vector field properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return false;
		}
		FPopcornFXSamplerDesc	*desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::VectorField)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't set attribute sampler vector field properties: sampler '%s' is not a vector field"), *InAttributeSamplerName);
			return false;
		}
		desc->SetProperties(&InProperties);
		FPopcornFXAttributeSamplerVectorField *sampler = static_cast<FPopcornFXAttributeSamplerVectorField *>(attrList->ResolveAttributeSampler(samplerIdx));
		if (sampler)
			sampler->RefreshFromProperties(&InProperties);
	}
	return true;
}

//---------------------------------------------------------------------------
// 
//	Sampler properties getters
// 
//---------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerAnimTrackProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesAnimTrack &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get anim track attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get anim track attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::AnimTrack)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get anim track attribute sampler properties: sampler '%s' is not a anim track"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesAnimTrack *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesAnimTrack *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//---------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerCurveProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesCurve &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get curve attribute sampler properties: invalid emitter"));
		return;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get curve attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Curve)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get curve attribute sampler properties: sampler '%s' is not a curve"), *InAttributeSamplerName);
			return;
		}
		const FPopcornFXAttributeSamplerPropertiesCurve *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesCurve *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerGridProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesGrid &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get grid attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get grid attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Grid)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get grid attribute sampler properties: sampler '%s' is not a grid"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesGrid *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesGrid *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerImageProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesImage &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get image attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get image attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Image)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get image attribute sampler properties: sampler '%s' is not a image"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesImage *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesImage *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerShapeProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesShape &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get shape attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get shape attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Shape)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get shape attribute sampler properties: sampler '%s' is not a shape"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesShape *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesShape *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//----------------------------------------------------------------------------
void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerTextProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesText &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get text attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get text attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::Text)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get text attribute sampler properties: sampler '%s' is not a text"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesText *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesText *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//----------------------------------------------------------------------------

void UPopcornFXAttributeSamplersFunctions::GetAttributeSamplerVectorFieldProperties(UPopcornFXEmitterComponent *Emitter, FString InAttributeSamplerName, FPopcornFXAttributeSamplerPropertiesVectorField &OutProperties)
{
	if (Emitter == null)
	{
		UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get vector field attribute sampler properties: invalid emitter"));
		return ;
	}

	const UWorld *world = Emitter->GetWorld();
	if (FApp::CanEverRender() && (world == null || !world->IsNetMode(NM_DedicatedServer)))
	{
		FPopcornFXAttributeList *attrList = Emitter->GetAttributeList();
		if (!PK_VERIFY(attrList != null))
			return ;

		const int32 samplerIdx = attrList->FindSamplerIndex(InAttributeSamplerName);
		if (samplerIdx == -1)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get vector field attribute sampler properties: can't find sampler '%s'"), *InAttributeSamplerName);
			return ;
		}
		FPopcornFXSamplerDesc *desc = attrList->GetSamplerDesc(samplerIdx);
		if (!desc || desc->m_SamplerType != EPopcornFXAttributeSamplerType::VectorField)
		{
			UE_LOG(LogPopcornFXAttributeSamplersFunctions, Warning, TEXT("Couldn't get vector field attribute sampler properties: sampler '%s' is not a vector field"), *InAttributeSamplerName);
			return ;
		}
		const FPopcornFXAttributeSamplerPropertiesVectorField *properties = static_cast<const FPopcornFXAttributeSamplerPropertiesVectorField *>(desc->ResolveAttributeProperties());
		if (properties)
			OutProperties = *properties;
	}
}

//---------------------------------------------------------------------------
// 
//	Image sampler functions
// 
//---------------------------------------------------------------------------

#if 0
void	UPopcornFXAttributeSamplersFunctions::SetImageSamplerTexture(UPopcornFXEmitterComponent *Emitter, FString ImageSamplerName, class UTexture *InTexture)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerImage *samplerImage = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ImageSamplerName, Image, samplerImage);
	if (samplerImage != nullptr)
		samplerImage->SetTexture(InTexture);
}

//---------------------------------------------------------------------------
// 
//	Curve sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetCurveSamplerDimension(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, TEnumAsByte<EAttributeSamplerCurveDimension::Type> InCurveDimension)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerCurve *samplerCurve = nullptr;
	FIND_ATTRIBUTE_SAMPLER(CurveSamplerName, Curve, samplerCurve);
	if (samplerCurve != nullptr)
		samplerCurve->SetCurveDimension(InCurveDimension);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetCurveSamplerCurve(UPopcornFXEmitterComponent *Emitter, FString CurveSamplerName, class UCurveBase *InCurve, bool InIsSecondCurve)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerCurve *samplerCurve = nullptr;
	FIND_ATTRIBUTE_SAMPLER(CurveSamplerName, Curve, samplerCurve);
	if (samplerCurve != nullptr)
		samplerCurve->SetCurve(InCurve, InIsSecondCurve);
}

//---------------------------------------------------------------------------
// 
//	Text sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetTextSamplerText(UPopcornFXEmitterComponent *Emitter, FString TextSamplerName, FString InText)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerText *samplerText = nullptr;
	FIND_ATTRIBUTE_SAMPLER(TextSamplerName, Text, samplerText);
	if (samplerText != nullptr)
		samplerText->SetText(InText);
}

//---------------------------------------------------------------------------
// 
//	Shape sampler functions
// 
//---------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Radius)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetRadius(Radius);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerWeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetWeight(Height);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerBoxDimension(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector BoxDimensions)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetBoxDimension(BoxDimensions);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerInnerRadius(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float InnerRadius)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetInnerRadius(InnerRadius);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerHeight(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, float Height)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetHeight(Height);
}

//----------------------------------------------------------------------------

void	UPopcornFXAttributeSamplersFunctions::SetShapeSamplerScale(UPopcornFXEmitterComponent *Emitter, FString ShapeSamplerName, FVector Scale)
{
	CHECK_VALID_CALL();

	FPopcornFXAttributeSamplerShape *samplerShape = nullptr;
	FIND_ATTRIBUTE_SAMPLER(ShapeSamplerName, Shape, samplerShape);
	if (samplerShape != nullptr)
		samplerShape->SetScale(Scale);
}
#endif // 0

//---------------------------------------------------------------------------
// 
//	Grid Read functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<float> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloatValues(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector2D> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat2Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat3Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FVector4> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridFloat4Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<int> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridIntValues(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntPoint> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt2Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt3Values(samplerGrid, OutValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::ReadGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, TArray<FIntVector4> &OutValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->ReadGridInt4Values(samplerGrid, OutValues);
	return false;
}

//---------------------------------------------------------------------------
// 
//	Grid Write functions
// 
//---------------------------------------------------------------------------

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloatValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<float> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloatValues(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector2D> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat2Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat3Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridFloat4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FVector4> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridFloat4Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridIntValues(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<int> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridIntValues(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt2Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntPoint> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt2Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt3Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt3Values(samplerGrid, InValues);
	return false;
}

bool	UPopcornFXAttributeSamplersFunctions::WriteGridInt4Values(UPopcornFXEmitterComponent *Emitter, FString InGridSamplerName, const TArray<FIntVector4> &InValues)
{
	CHECK_VALID_CALL(false);

	FPopcornFXAttributeSamplerGrid *samplerGrid = nullptr;
	FIND_ATTRIBUTE_SAMPLER(InGridSamplerName, Grid, samplerGrid);
	if (samplerGrid != nullptr)
		return samplerGrid->WriteGridInt4Values(samplerGrid, InValues);
	return false;
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
