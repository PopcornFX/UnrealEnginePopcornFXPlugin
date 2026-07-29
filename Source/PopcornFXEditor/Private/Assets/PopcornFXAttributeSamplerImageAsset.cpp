#include "PopcornFXAttributeSamplerImageAsset.h"

UPopcornFXAttributeSamplerImageFactory::UPopcornFXAttributeSamplerImageFactory(FObjectInitializer const &PCIP)
	: Super(PCIP)
{
	SupportedClass = UPopcornFXAttributeSamplerImageAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
