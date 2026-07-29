#include "PopcornFXAttributeSamplerVectorFieldAsset.h"

UPopcornFXAttributeSamplerVectorFieldFactory::UPopcornFXAttributeSamplerVectorFieldFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerVectorFieldAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}