//----------------------------------------------------------------------------
// Copyright Persistant Studios, SARL.
// https://popcornfx.com/popcornfx-community-license/
//----------------------------------------------------------------------------

#pragma once

#include "PopcornFXPublic.h"

#include "PopcornFXAttributeSampler.h"

#include "PopcornFXAttributeSamplerText.generated.h"

struct	FAttributeSamplerTextData;


USTRUCT(BlueprintType)
struct POPCORNFX_API FPopcornFXAttributeSamplerPropertiesText : public FPopcornFXAttributeSamplerProperties
{
	GENERATED_USTRUCT_BODY()

	/** The Text to be sampled */
	UPROPERTY(Category = "PopcornFX AttributeSampler", BlueprintReadOnly, EditAnywhere, meta = (Multiline = true))
	FString		Text;
};

/** Can override an Attribute Sampler **Text** by a **FString**. */
UCLASS(EditInlineNew, meta=(BlueprintSpawnableComponent), ClassGroup=PopcornFX)
class POPCORNFX_API UPopcornFXAttributeSamplerText : public UPopcornFXAttributeSampler
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "PopcornFX AttributeSampler")
	FPopcornFXAttributeSamplerPropertiesText	Properties;

	UFUNCTION(BlueprintCallable, Category="PopcornFX|AttributeSampler")
	void		SetText(FString InText);

	// overrides
	void		BeginDestroy() override;
#if WITH_EDITOR
	void		PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

	// UPopcornFXAttributeSampler overrides
	const FPopcornFXAttributeSamplerProperties		*GetProperties() const override { return &Properties; }
#if WITH_EDITOR
	virtual void									CopyPropertiesFrom(const UPopcornFXAttributeSampler *other) override;
	virtual void									SetupDefaults(UPopcornFXEffect *effect, const uint32 samplerIdx, bool updateUnlockedValues) override;
#endif
	virtual bool									ArePropertiesSupported() override;
	virtual bool									ArePropertiesCompatible(UPopcornFXEmitterComponent *emitter, const PopcornFX::CResourceDescriptor *defaultSampler) override;
	virtual PopcornFX::CParticleSamplerDescriptor	*_AttribSampler_SetupSamplerDescriptor(UPopcornFXEmitterComponent *emitter, FPopcornFXSamplerDesc &desc, const PopcornFX::CResourceDescriptor *defaultSampler) override;

private:
	FAttributeSamplerTextData	*m_Data;
};
