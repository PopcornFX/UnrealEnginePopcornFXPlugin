//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL. All Rights Reserved.
// https://www.popcornfx.com/terms-and-conditions/
//----------------------------------------------------------------------------

#pragma once

class	CPopcornFXEffect
{
public:
	CPopcornFXEffect(UPopcornFXEffect *effect)
	:	m_Owner(effect)
	{
	}

	bool									RegisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID);
	void									UnregisterEventCallback(const PopcornFX::FastDelegate<PopcornFX::CParticleEffect::EventCallback> &callback, const PopcornFX::CStringId &eventNameID);
	bool									HasRegisteredEvents() const { return !m_RegisteredEventNameIDs.Empty(); }

	PopcornFX::PCParticleEffect				ParticleEffect();
	PopcornFX::PCParticleAttributeList		AttributeList(); // LoadEffectIFN
	PopcornFX::PCParticleAttributeList		AttributeListIFP() const;
	PopcornFX::PCParticleEffect				ParticleEffectIFP() const;
	PopcornFX::PParticleEffect				Unsafe_ParticleEffect() const;

private:
	PopcornFX::PCParticleEffect				m_ParticleEffect;
	PopcornFX::TArray<PopcornFX::CStringId>	m_RegisteredEventNameIDs;

	friend class UPopcornFXEffect;
	UPopcornFXEffect						*m_Owner; // No refptr. CPopcornFXEffect is always accessed through the owner. It can't be invalid
};
