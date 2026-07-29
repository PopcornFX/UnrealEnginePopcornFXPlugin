//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeList.h"
#include "PopcornFXEmitterComponent.h"
#include "PopcornFXAttributeSamplerShape.h"
#include "PopcornFXAttributeSamplerImage.h"
#include "PopcornFXAttributeSamplerGrid.h"
#include "PopcornFXAttributeSamplerText.h"
#include "PopcornFXAttributeSamplerCurve.h"
#include "PopcornFXAttributeSamplerCurveDynamic.h"
#include "PopcornFXAttributeSamplerAnimTrack.h"
#include "PopcornFXAttributeSamplerVectorField.h"

#include "PopcornFXSDK.h"

#include "Components/BillboardComponent.h"
#include "Engine/Texture2D.h"

//----------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "PopcornFXAttributeSampler"
DEFINE_LOG_CATEGORY_STATIC(LogPopcornFXAttributeSampler, Log, All);

//----------------------------------------------------------------------------
//
// FPopcornFXAttributeSampler
//
//----------------------------------------------------------------------------

PopcornFX::CParticleSamplerDescriptor *FPopcornFXAttributeSampler::_AttribSampler_SetupSampler(UPopcornFXEmitterComponent *emitter, const FString &samplerName, FPopcornFXAttributeSamplerProperties *properties, const PopcornFX::CResourceDescriptor *defaultSampler)
{
	if (properties == nullptr)
		return nullptr;
#if WITH_EDITOR
	properties->m_UnsupportedProperties.Empty();
	emitter->m_IncompatibleProperties.FindOrAdd(properties).m_Properties.Empty();
#endif

	if (!properties->ArePropertiesSupported(emitter, samplerName))
	{
#if WITH_EDITOR
		emitter->m_IncompatibleProperties.FindOrAdd(properties).m_Properties = properties->m_UnsupportedProperties;
#endif
		return null;
	}
	if (!properties->ArePropertiesCompatible(emitter, samplerName, defaultSampler))
	{
		return null;
	}

	PopcornFX::CParticleSamplerDescriptor *samplerDesc = _AttribSampler_SetupSamplerDescriptor(emitter, properties, defaultSampler);
	if (samplerDesc == null)
	{
		return null;
	}

#if WITH_EDITOR
	properties->m_UnsupportedProperties.Empty();
	emitter->m_IncompatibleProperties.FindOrAdd(properties).m_Properties.Empty();
#endif
	return samplerDesc;
}

//----------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
